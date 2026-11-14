#include "entities/Player.h"
#include "world/Arena.h"
#include <iostream>

Player::Player(sf::Vector2f spawnPos)
    : position(spawnPos), _animation("assets/sprites/mainHeroTest.png", true, spawnPos)
{
}

void Player::reset(sf::Vector2f spawnPos) {
    position = spawnPos;
    velocity = {0.0f, 0.0f};
    _isOnGround = false;
    _isAirSuspended = false;
    _coyoteTime = 0.0f;
    _jumpBuffer = 0.0f;
    _dashTimer = 0.0f;
    _dashWindowTimer = 0.0f;
    _dashCooldown = 0.0f;
    _iframeTimer = 0.0f;
    _isSpotDodging = false;
    _spotDodgeTimer = 0.0f;
    _spotDodgeCooldown = 0.0f;
    _perfectDodgeWindowTimer = 0.0f;
    _perfectDodgeEmpoweredTimer = 0.0f;
    _isCounterStance = false;
    _counterWindowTimer = 0.0f;
    _counterCooldown = 0.0f;
    _counterWhiffStunActive = false;
    _stunTimer = 0.0f;
    _facingLocked = false;
    _turnDelayTimer = 0.0f;
    _simulatedVerticalDir = 0.0f;
    _simulatedMoveDir = 0.0f;
    hp = maxHp;
    displayedHp = maxHp;
    isDead = false;
    diveSlamBounce = false;
    diveSlamDir = 1.0f;
    _dashGhosts.clear();
    _animation.setColor(sf::Color::White);
    _animation.setPosition(position);
}

bool Player::takeDamage(float amount, float knockX, float knockY, float stunTime) {
    if (isDead) return false;
    hp -= amount;
    if (hp <= 0.0f) {
        hp = 0.0f;
        isDead = true;
    }
    velocity.x = knockX;
    velocity.y = knockY;
    _stunTimer = stunTime;
    _isOnGround = false;
    _isAirSuspended = false;
    return true;
}

void Player::lunge(float distance) {
    float dir = isFacingRight() ? 1.0f : -1.0f;
    position.x += dir * distance;
}

void Player::triggerCounterStance() {
    if (isDead || isStunned() || _isCounterStance || _dashTimer > 0.0f || _isSpotDodging || _counterCooldown > 0.0f) return;
    const auto& pcfg = PlayerConfig::get();
    _isCounterStance = true;
    _counterWindowTimer = pcfg.counterWindow;
    _counterCooldown = pcfg.counterCooldown;
    velocity.x *= 0.1f;
}

void Player::endCounterSuccess() {
    _isCounterStance = false;
    _counterWindowTimer = 0.0f;
}

void Player::handleInput(const std::optional<sf::Event>& event) {
    if (!event) return;
    if (isStunned()) return;

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::C) {
            _collisionEnabled = !_collisionEnabled;
            std::cout << "Коллизия игрока: " << (_collisionEnabled ? "ВКЛ" : "ВЫКЛ") << std::endl;
        }

        // Прыжок
        if (keyPressed->code == sf::Keyboard::Key::Space || 
            keyPressed->code == sf::Keyboard::Key::W || 
            keyPressed->code == sf::Keyboard::Key::Up) {
            _jumpBuffer = 0.12f;
        }

        // Стойка контр-атаки / парирования на кнопку [Q]
        if (keyPressed->code == sf::Keyboard::Key::Q) {
            triggerCounterStance();
        }

        // Рывок / Уворот на Shift
        if (keyPressed->code == sf::Keyboard::Key::LShift || 
            keyPressed->code == sf::Keyboard::Key::RShift) {
            if (_dashCooldown <= 0.0f && _dashTimer <= 0.0f && _spotDodgeCooldown <= 0.0f && !_isSpotDodging) {
                float dir = _simulatedMoveDir;
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
                    dir -= 1.0f;
                }
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
                    dir += 1.0f;
                }

                // В воздухе при отсутствии зажатых клавиш делаем рывок вперед
                if (dir == 0.0f && !_isOnGround) {
                    dir = isFacingRight() ? 1.0f : -1.0f;
                }

                if (dir != 0.0f) {
                    // 1. Направленный рывок (Directional Dash)
                    const auto& pcfg = PlayerConfig::get();
                    _dashDirX = (dir > 0.0f) ? 1.0f : -1.0f;
                    _dashTimer = pcfg.dashDuration;
                    _dashWindowTimer = pcfg.dashDuration + 0.10f; // Короткое окно атаки в дэше
                    _iframeTimer = pcfg.iframeDuration;
                    _dashCooldown = pcfg.dashCooldown;
                    velocity.y = 0.0f;
                    _ghostSpawnTimer = 0.0f;
                    _animation.setFacing(_dashDirX > 0.0f);
                } else {
                    // 2. Уворот на месте при отсутствии движения (Spot Dodge на земле)
                    const auto& pcfg = PlayerConfig::get();
                    _isSpotDodging = true;
                    _spotDodgeTimer = pcfg.spotDodgeDuration;
                    _spotDodgeCooldown = pcfg.spotDodgeCooldown;
                    _dashCooldown = pcfg.spotDodgeCooldown;
                    _iframeTimer = pcfg.spotDodgeDuration;
                    _perfectDodgeWindowTimer = pcfg.perfectDodgeWindow; // Окно идеального уворота в первые доли секунды
                    _dashWindowTimer = 0.20f; // Короткое окно для атаки из уворота
                    velocity.x = 0.0f;

                    // Оставляем призрачный фантом на месте уворота
                    _dashGhosts.emplace_back(_animation.getCurrentSprite(), 220.0f, 0.22f);
                    _dashGhosts.back().sprite.setColor(sf::Color(140, 220, 255, 210));
                }
            }
        }
    }
}

bool Player::moveAxis(float& currentCoord, float delta, bool isX, float fixedCoord, const Arena& arena) {
    if (delta == 0.0f) return false;

    float stepSign = (delta > 0.0f) ? 1.0f : -1.0f;
    float remaining = std::abs(delta);
    constexpr float maxStep = 1.0f;
    bool collided = false;

    while (remaining > 0.0f) {
        float step = std::min(remaining, maxStep) * stepSign;
        float nextCoord = currentCoord + step;
        float testX = isX ? nextCoord : fixedCoord;
        float testY = isX ? fixedCoord : nextCoord;

        if (arena.overlapsSolid(testX, testY, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
            collided = true;
            float low = 0.0f;
            float high = step;
            for (int iter = 0; iter < 6; ++iter) {
                float mid = (low + high) * 0.5f;
                float tx = isX ? (currentCoord + mid) : fixedCoord;
                float ty = isX ? fixedCoord : (currentCoord + mid);
                if (arena.overlapsSolid(tx, ty, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                    high = mid;
                } else {
                    low = mid;
                }
            }
            currentCoord += low;
            break;
        }

        currentCoord = nextCoord;
        remaining -= std::abs(step);
    }
    return collided;
}

void Player::update(float dt, const Arena& arena, const sf::Vector2f& mouseWorld) {
    float moveX = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
        moveX -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
        moveX += 1.0f;
    }

    if (_isOnGround) {
        _coyoteTime = 0.10f;
    } else if (_coyoteTime > 0.0f) {
        _coyoteTime -= dt;
    }

    if (_jumpBuffer > 0.0f) {
        _jumpBuffer -= dt;
    }

    if (_dashCooldown > 0.0f) _dashCooldown -= dt;
    if (_spotDodgeCooldown > 0.0f) _spotDodgeCooldown -= dt;
    if (_dashWindowTimer > 0.0f) _dashWindowTimer -= dt;
    if (_iframeTimer > 0.0f) _iframeTimer -= dt;
    if (_perfectDodgeWindowTimer > 0.0f) _perfectDodgeWindowTimer -= dt;
    if (_perfectDodgeEmpoweredTimer > 0.0f) _perfectDodgeEmpoweredTimer -= dt;

    const auto& pcfg = PlayerConfig::get();

    if (_isSpotDodging) {
        _spotDodgeTimer -= dt;
        if (_spotDodgeTimer <= 0.0f) {
            _isSpotDodging = false;
        }
        moveX = 0.0f; // При увороте на месте персонаж остается на своей позиции
    }

    if (_counterCooldown > 0.0f) _counterCooldown -= dt;

    if (_isCounterStance) {
        _counterWindowTimer -= dt;
        moveX = 0.0f; // Во время стойки парирования стоим на месте
        velocity.x *= 0.1f;
        if (_counterWindowTimer <= 0.0f) {
            // ПРОМАХ СТОЙКИ ПАРИРОВАНИЯ (WHIFF)!
            // Противник не ударил — игрок получает ДОЛГИЙ СТАН!
            _isCounterStance = false;
            _counterWhiffStunActive = true;
            triggerStun(pcfg.counterWhiffStun);
        }
    }

    if (_stunTimer > 0.0f) {
        _stunTimer -= dt;
        if (_stunTimer <= 0.0f) {
            _stunTimer = 0.0f;
            _counterWhiffStunActive = false;
        }
        moveX = 0.0f; // При стан-локе управление движением заблокировано
    }

    // Прыжок
    if (_jumpBuffer > 0.0f && (_isOnGround || _coyoteTime > 0.0f) && _dashTimer <= 0.0f) {
        velocity.y = -pcfg.jumpSpeed;
        _isOnGround = false;
        _coyoteTime = 0.0f;
        _jumpBuffer = 0.0f;
    }

    // Плавное сглаживание полоски здоровья
    displayedHp += (hp - displayedHp) * std::min(1.0f, dt * 10.0f);

    // Гравитация (отключается в рывке и во время атак в воздухе)
    if (!_isOnGround && _dashTimer <= 0.0f && !_isAirSuspended) {
        float appliedGravity = pcfg.gravity;
        if (_stunTimer > 0.0f && velocity.y < 0.0f) {
            appliedGravity = pcfg.gravity * 0.18f; // Мягкая гравитация жонглирования в стане ТОЛЬКО при подбросе вверх
        } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            appliedGravity *= 1.6f;
        }
        velocity.y += appliedGravity * dt;
        if (velocity.y > pcfg.maxFallSpeed) velocity.y = pcfg.maxFallSpeed;
    } else if (_isAirSuspended) {
        velocity.y = 0.0f; // Зависание в воздухе во время атак
    }

    float deltaX = 0.0f;
    float deltaY = 0.0f;

    if (_dashTimer > 0.0f) {
        _dashTimer -= dt;
        deltaX = _dashDirX * (pcfg.dashSpeed * dt);
        deltaY = 0.0f;
        velocity.y = 0.0f;

        _ghostSpawnTimer -= dt;
        if (_ghostSpawnTimer <= 0.0f) {
            _ghostSpawnTimer = 0.025f;
            _dashGhosts.emplace_back(_animation.getCurrentSprite(), 180.0f, 0.15f);
            _dashGhosts.back().sprite.setColor(sf::Color(120, 200, 255, 180));
        }
    } else {
        deltaX = (moveX * pcfg.moveSpeed + velocity.x) * dt;
        deltaY = velocity.y * dt;

        // Затухание горизонтальной форсы отброса (экспоненциальное трение)
        float friction = _isOnGround ? 10.0f : 4.0f;
        velocity.x *= std::exp(-friction * dt);
        if (std::abs(velocity.x) < 0.1f) velocity.x = 0.0f;
    }

    // Затухание фантомов шлейфа
    for (auto it = _dashGhosts.begin(); it != _dashGhosts.end(); ) {
        it->lifetime -= dt;
        it->alpha -= (180.0f / 0.15f) * dt;
        if (it->lifetime <= 0.0f || it->alpha <= 0.0f) {
            it = _dashGhosts.erase(it);
        } else {
            sf::Color c = it->sprite.getColor();
            c.a = static_cast<std::uint8_t>(std::clamp(it->alpha, 0.0f, 255.0f));
            it->sprite.setColor(c);
            ++it;
        }
    }

    if (_collisionEnabled) {
        moveAxis(position.x, deltaX, true, position.y, arena);

        bool hitY = moveAxis(position.y, deltaY, false, position.x, arena);

        if (deltaY > 0.0f) {
            if (hitY) {
                // Отскок от пола ТОЛЬКО при ударе врага с дайв-слэмом или при получении урона от слэма
                if (diveSlamBounce) {
                    velocity.y = -280.0f;
                    velocity.x = diveSlamDir * 1150.0f;
                    diveSlamBounce = false;
                    _isOnGround = false;
                } else if (_stunTimer > 0.0f && velocity.y > 450.0f) {
                    velocity.y = -280.0f; // Ground Bounce отскок вверх ТОЛЬКО при ударе об пол во время стана от врага
                    velocity.x *= 0.55f;
                    _isOnGround = false;
                } else {
                    _isOnGround = true;
                    velocity.y = 0.0f;
                }
            } else {
                _isOnGround = arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, TOP_OFFSET, BOTTOM_OFFSET);
            }
        } else if (deltaY < 0.0f) {
            if (hitY) {
                velocity.y = 0.0f;
            }
            _isOnGround = false;
        } else {
            _isOnGround = arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, TOP_OFFSET, BOTTOM_OFFSET);
        }
    } else {
        position.x += deltaX;
        position.y += deltaY;
    }

    // Ограничение границами арены
    position.x = std::clamp(position.x, HALF_W, arena.getWidth() * TILE_SIZE - HALF_W);
    position.y = std::clamp(position.y, TOP_OFFSET, arena.getHeight() * TILE_SIZE - BOTTOM_OFFSET);

    _animation.setPosition(position);

    // Поворот спрайта: запоминаем направление, ориентируемся ТОЛЬКО на движение или рывок!
    // Во время атак и цепочек комбо поворот блокируется (_facingLocked), чтобы ввод направленных
    // атак назад (b1, b2, 11b2) не разворачивал персонажа преждевременно!
    if (isDashing() && _dashDirX != 0.0f) {
        _animation.setFacing(_dashDirX > 0.0f);
        _turnDelayTimer = 0.0f;
    } else if (!_facingLocked && moveX != 0.0f) {
        bool wantsTurn = (moveX > 0.0f) != isFacingRight();
        if (wantsTurn) {
            _turnDelayTimer += dt;
            if (_turnDelayTimer >= 0.07f) {
                _animation.setFacing(moveX > 0.0f);
                _turnDelayTimer = 0.0f;
            }
        } else {
            _turnDelayTimer = 0.0f;
            _animation.setFacing(moveX > 0.0f);
        }
    } else {
        _turnDelayTimer = 0.0f;
    }

    if (isCounterStanceActive()) {
        // Яркий неоново-голубой цвет активной стойки контр-атаки / парирования
        _animation.setColor(sf::Color(100, 240, 255, 255));
    } else if (isStunned()) {
        // Серый цвет изнеможения / стан-лока от повторения комбо
        _animation.setColor(sf::Color(140, 140, 140, 210));
    } else if (isPerfectDodgeEmpowered()) {
        // Золотистое сияние готовности к мощной контратаке после идеального уворота!
        _animation.setColor(sf::Color(255, 230, 85, 245));
    } else if (_isSpotDodging) {
        // Призрачно-серебристый цвет во время уворота на месте
        _animation.setColor(sf::Color(170, 230, 255, 190));
    } else if (isInvulnerable()) {
        _animation.setColor(sf::Color(130, 215, 255, 220));
    } else {
        _animation.setColor(sf::Color::White);
    }
}

void Player::draw(sf::RenderWindow& window) {
    for (const auto& ghost : _dashGhosts) {
        window.draw(ghost.sprite);
    }

    float moveX = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) moveX -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) moveX += 1.0f;

    if (isDashing()) {
        _animation.setCurrentFrame(1);
        window.draw(_animation.getCurrentSprite());
    } else if (_isOnGround) {
        if (moveX != 0.0f) {
            _animator.RunAnimation(_animation, window);
        } else {
            _animation.setCurrentFrame(0);
            window.draw(_animation.getCurrentSprite());
        }
    } else {
        _animation.setCurrentFrame(1);
        window.draw(_animation.getCurrentSprite());
    }

    if (isCounterStanceActive()) {
        sf::CircleShape shield(26.0f);
        shield.setOrigin({26.0f, 26.0f});
        shield.setPosition(position);
        shield.setFillColor(sf::Color(0, 210, 255, 60));
        shield.setOutlineThickness(2.5f);
        shield.setOutlineColor(sf::Color(120, 240, 255, 230));
        window.draw(shield);

        sf::RectangleShape parryLine({24.0f, 3.0f});
        parryLine.setOrigin({12.0f, 1.5f});
        parryLine.setPosition({position.x, position.y - TOP_OFFSET - 8.0f});
        parryLine.setFillColor(sf::Color(0, 240, 255));
        window.draw(parryLine);
    }
}

bool Player::isHoldingUp() const {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || 
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || 
           _simulatedVerticalDir < -0.1f;
}

bool Player::isHoldingDown() const {
    return sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || 
           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || 
           _simulatedVerticalDir > 0.1f;
}

bool Player::isHoldingForward() const {
    bool rightPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || 
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || 
                        _simulatedMoveDir > 0.1f;
    bool leftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || 
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || 
                       _simulatedMoveDir < -0.1f;
    return isFacingRight() ? rightPressed : leftPressed;
}

bool Player::isHoldingBackward() const {
    bool rightPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || 
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || 
                        _simulatedMoveDir > 0.1f;
    bool leftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || 
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || 
                       _simulatedMoveDir < -0.1f;
    return isFacingRight() ? leftPressed : rightPressed;
}

