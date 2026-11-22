#include "entities/Player.h"
#include "world/Arena.h"
#include "audio/SoundManager.h"
#include "combat/CombatSystem.h"
#include <iostream>

Player::Player(sf::Vector2f spawnPos)
    : position(spawnPos),
      _animation(resolveAssetPath("assets/sprites/player/mainHeroTest.png"), true, spawnPos),
      _rollAnimation(resolveAssetPath("assets/sprites/player/heroRoll.png"), false, spawnPos),
      _lightAttackAnim(resolveAssetPath("assets/sprites/player/heroAttackLight.png"), false, spawnPos),
      _heavyAttackAnim(resolveAssetPath("assets/sprites/player/heroAttackHeavy.png"), false, spawnPos),
      _uppercutAnim(resolveAssetPath("assets/sprites/player/heroUppercut.png"), false, spawnPos),
      _sweepAnim(resolveAssetPath("assets/sprites/player/heroSweep.png"), false, spawnPos),
      _dashAttackAnim(resolveAssetPath("assets/sprites/player/heroDashAttack.png"), false, spawnPos),
      _throwAnim(resolveAssetPath("assets/sprites/player/heroThrow.png"), false, spawnPos),
      _airAttackAnim(resolveAssetPath("assets/sprites/player/heroAirAttack.png"), false, spawnPos),
      _diveSlamAnim(resolveAssetPath("assets/sprites/player/heroDiveSlam.png"), false, spawnPos),
      _turnAttackAnim(resolveAssetPath("assets/sprites/player/heroTurnAttack.png"), false, spawnPos)
{
}

Animation* Player::getAttackAnimation(AttackAnimType animType) {
    switch (animType) {
        case AttackAnimType::LIGHT:       return &_lightAttackAnim;
        case AttackAnimType::HEAVY:       return &_heavyAttackAnim;
        case AttackAnimType::UPPERCUT:    return &_uppercutAnim;
        case AttackAnimType::SWEEP:       return &_sweepAnim;
        case AttackAnimType::DASH_ATTACK: return &_dashAttackAnim;
        case AttackAnimType::THROW:       return &_throwAnim;
        case AttackAnimType::AIR_ATTACK:  return &_airAttackAnim;
        case AttackAnimType::DIVE_SLAM:   return &_diveSlamAnim;
        case AttackAnimType::TURN_ATTACK: return &_turnAttackAnim;
        default:                          return nullptr;
    }
}

const Animation* Player::getAttackAnimation(AttackAnimType animType) const {
    switch (animType) {
        case AttackAnimType::LIGHT:       return &_lightAttackAnim;
        case AttackAnimType::HEAVY:       return &_heavyAttackAnim;
        case AttackAnimType::UPPERCUT:    return &_uppercutAnim;
        case AttackAnimType::SWEEP:       return &_sweepAnim;
        case AttackAnimType::DASH_ATTACK: return &_dashAttackAnim;
        case AttackAnimType::THROW:       return &_throwAnim;
        case AttackAnimType::AIR_ATTACK:  return &_airAttackAnim;
        case AttackAnimType::DIVE_SLAM:   return &_diveSlamAnim;
        case AttackAnimType::TURN_ATTACK: return &_turnAttackAnim;
        default:                          return nullptr;
    }
}

void Player::triggerAttackAnimation(AttackAnimType animType, float duration) {
    _currentAttackAnim = animType;
    _attackAnimDuration = std::max(0.01f, duration);
    _attackAnimTimer = _attackAnimDuration;
    Animation* anim = getAttackAnimation(animType);
    if (anim) {
        anim->setCurrentFrame(0);
        anim->setPosition(position);
        anim->setFacing(isFacingRight());
    }
}

void Player::cancelAttackAnimation() {
    _currentAttackAnim = AttackAnimType::NONE;
    _attackAnimTimer = 0.0f;
    _attackAnimDuration = 0.0f;
}

void Player::syncAllAnimations(const sf::Color& color) {
    bool faceRight = isFacingRight();
    Animation* allAnims[] = {
        &_animation, &_rollAnimation,
        &_lightAttackAnim, &_heavyAttackAnim, &_uppercutAnim,
        &_sweepAnim, &_dashAttackAnim, &_throwAnim,
        &_airAttackAnim, &_diveSlamAnim, &_turnAttackAnim
    };
    for (auto* a : allAnims) {
        a->setPosition(position);
        a->setFacing(faceRight);
        a->setColor(color);
    }
}

void Player::reset(sf::Vector2f spawnPos, bool withWeapon) {
    position = spawnPos;
    velocity = {0.0f, 0.0f};
    _isOnGround = false;
    _forceAirborneForTest = false;
    _isAirSuspended = false;
    _isJuggledByEnemy = false;
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
    _hasWeapon = withWeapon; // С самого начала персонаж появляется с булавой в руках (по умолчанию withWeapon=true)
    _hasWallBounced = false;
    hp = maxHp;
    displayedHp = maxHp;
    isDead = false;
    healFlasks = 4;
    maxHealFlasks = 4;
    diveSlamBounce = false;
    diveSlamDir = 1.0f;
    _pendingLunge = 0.0f;
    _isAirGrabLocked = false;
    _dashGhosts.clear();
    cancelAttackAnimation();
    syncAllAnimations(sf::Color::White);
    _animation.setCurrentFrame(0);
    _rollAnimation.setCurrentFrame(0);
}

bool Player::takeDamage(float amount, float knockX, float knockY, float stunTime,
                        HitLevel hitLevel, bool isBackHit, const sf::Vector2f& attackerPos) {
    if (isDead) return false;

    // СИСТЕМА ЗАЩИТЫ / БЛОКА (снижение входящего урона на 80%)
    // Проверка попадания в спину
    bool defenderFacingRight = isFacingRight();
    bool hitFromBehind = isBackHit;
    if (attackerPos != sf::Vector2f(0.0f, 0.0f)) {
        if (defenderFacingRight && attackerPos.x < position.x - 5.0f) {
            hitFromBehind = true;
        } else if (!defenderFacingRight && attackerPos.x > position.x + 5.0f) {
            hitFromBehind = true;
        }
    }

    // Проверка эффективности блока (блок работает строго на земле!)
    bool blockDefended = false;
    if (_blockType != BlockType::NONE && _isOnGround && !hitFromBehind) {
        if (hitLevel == HitLevel::MID) {
            blockDefended = true; // Нейтральные атаки блокируются и верхним, и нижним блоком
        } else if (hitLevel == HitLevel::OVERHEAD) {
            // Оверхэды (атаки сверху) блокируются только верхним блоком! Пробивают нижний блок
            if (_blockType == BlockType::HIGH) {
                blockDefended = true;
            }
        } else if (hitLevel == HitLevel::LOW) {
            // Нижние удары блокируются только нижним блоком! Пробивают верхний блок
            if (_blockType == BlockType::LOW) {
                blockDefended = true;
            }
        }
    }

    if (blockDefended) {
        float originalAmount = amount;
        damageGuard(originalAmount * 4.0f);
        amount *= 0.20f;
        hp -= amount;
        if (hp <= 0.0f) {
            hp = 0.0f;
            isDead = true;
        }
        velocity.x = knockX * 0.25f; // Небольшой откат по земле
        velocity.y = 0.0f;
        _blockStunTimer = 0.14f;     // Стан-лок защищающегося от каждого заблокированного удара
        return true;
    }

    if (_stunTimer <= 0.0f) {
        _hasWallBounced = false;
    }

    cancelAttackAnimation();
    hp -= amount;
    if (hp <= 0.0f) {
        hp = 0.0f;
        isDead = true;
    }
    velocity.x = knockX;
    velocity.y = knockY;
    _stunTimer = stunTime;
    _isJuggledByEnemy = (knockY < 0.0f);
    if (knockY < 0.0f) {
        _isOnGround = false;
    }
    _isAirSuspended = false;
    return true;
}

void Player::turnAround() {
    bool newFacing = !_animation.isFacingRight();
    _animation.setFacing(newFacing);
    _rollAnimation.setFacing(newFacing);
    Animation* allAnims[] = {
        &_lightAttackAnim, &_heavyAttackAnim, &_uppercutAnim,
        &_sweepAnim, &_dashAttackAnim, &_throwAnim,
        &_airAttackAnim, &_diveSlamAnim, &_turnAttackAnim
    };
    for (auto* a : allAnims) {
        a->setFacing(newFacing);
    }
}

void Player::lunge(float distance) {
    float dir = isFacingRight() ? 1.0f : -1.0f;
    _pendingLunge += dir * distance;
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

bool Player::useHealFlask() {
    if (isDead || isStunned() || _isAirGrabLocked) return false;
    if (healFlasks <= 0) return false;
    if (hp >= maxHp) return false; // Уже полное здоровье, не тратим хилку

    healFlasks--;
    hp = maxHp;
    displayedHp = maxHp;

    if (CombatSystem::getActive()) {
        CombatSystem::getActive()->spawnHealEffect(position, healFlasks);
    }
    return true;
}

void Player::handleInput(const std::optional<sf::Event>& event) {
    if (!event || isDead) return;
    if (isStunned() || _isAirGrabLocked) return;

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::C) {
            _collisionEnabled = !_collisionEnabled;
            std::cout << "Коллизия игрока: " << (_collisionEnabled ? "ВКЛ" : "ВЫКЛ") << std::endl;
        }

        // Прыжок (строго на клавишу Пробел / Space)
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            _jumpBuffer = 0.12f;
        }

        // Взаимодействие с физическими дверьми на кнопку [E]
        if (keyPressed->code == sf::Keyboard::Key::E) {
            if (auto* act = Arena::getActive()) {
                act->interactDoorNear(position, 96.0f);
            }
        }

        // Использование хилки (восстановление до макс ХП) на клавишу [R]
        if (keyPressed->code == sf::Keyboard::Key::R) {
            useHealFlask();
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
                    _perfectDodgeWindowTimer = pcfg.perfectDodgeWindow; // Окно идеального уворота при рывке сквозь атаку
                    _dashCooldown = pcfg.dashCooldown;
                    velocity.y = 0.0f;
                    _ghostSpawnTimer = 0.0f;
                    _animation.setFacing(_dashDirX > 0.0f);
                    _rollAnimation.setFacing(_dashDirX > 0.0f);
                    _rollAnimation.setCurrentFrame(0);
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
            bool steppedUp = false;
            if (isX && _isOnGround) {
                constexpr float maxStepUp = 32.0f; // Плавный подъем на полублоки и наклонные лестницы
                for (float up = 1.0f; up <= maxStepUp; up += 1.0f) {
                    if (!arena.overlapsSolid(testX, testY - up, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                        currentCoord = nextCoord;
                        position.y -= up;
                        remaining -= std::abs(step);
                        steppedUp = true;
                        break;
                    }
                }
            }
            if (steppedUp) continue;

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
    if (isDead) {
        _isAirSuspended = false;
        _dashTimer = 0.0f;
        _isSpotDodging = false;
        _isCounterStance = false;
        _jumpBuffer = 0.0f;
        velocity.x *= std::exp(-8.0f * dt);
        if (std::abs(velocity.x) < 0.1f) velocity.x = 0.0f;

        const auto& pcfg = PlayerConfig::get();
        if (!_isOnGround) {
            velocity.y += pcfg.gravity * dt;
            if (velocity.y > pcfg.maxFallSpeed) velocity.y = pcfg.maxFallSpeed;
        }

        float deltaX = velocity.x * dt;
        float deltaY = velocity.y * dt;
        if (_collisionEnabled) {
            moveAxis(position.x, deltaX, true, position.y, arena);
            bool hitY = moveAxis(position.y, deltaY, false, position.x, arena);
            if (hitY) {
                _isOnGround = true;
                velocity.y = 0.0f;
            }
        }
        _animation.setPosition(position);
        _rollAnimation.setPosition(position);
        return;
    }

    float moveX = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || _simulatedMoveDir < -0.1f) {
        moveX -= 1.0f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || _simulatedMoveDir > 0.1f) {
        moveX += 1.0f;
    }

    // Обработка блока (F) и таймера блок-стана
    if (_blockStunTimer > 0.0f) {
        _blockStunTimer -= dt;
        moveX = 0.0f; // Персонаж находится в коротком блок-стане от каждого заблокированного удара
    }

    // Взаимодействие с дверьми на клавишу [E]
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E)) {
        if (auto* act = Arena::getActive()) {
            act->interactDoorNear(position, 96.0f);
        }
    }

    // Шкала прочности блока (Guard / Poise):
    if (guardBrokenTimer > 0.0f) {
        guardBrokenTimer -= dt;
        _blockType = BlockType::NONE;
        if (guardBrokenTimer <= 0.0f) {
            guardHp = maxGuardHp * 0.5f; // Восстанавливаем 50% прочности после выхода из стана
        }
    } else {
        if (_blockType == BlockType::NONE && guardHp < maxGuardHp) {
            if (guardRegenDelay > 0.0f) {
                guardRegenDelay -= dt;
            } else {
                guardHp = std::min(maxGuardHp, guardHp + 35.0f * dt);
            }
        }
    }

    bool wantsBlock = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F);
    if (_isOnGround && wantsBlock && _stunTimer <= 0.0f && _dashTimer <= 0.0f && !_isCounterStance && !_isSpotDodging && !_isAirGrabLocked && !isGuardBroken()) {
        if (isHoldingDown()) {
            _blockType = BlockType::LOW;
        } else {
            _blockType = BlockType::HIGH;
        }
        moveX *= 0.35f; // Замедленное передвижение с поднятым блоком
    } else {
        _blockType = BlockType::NONE;
    }

    if (_isOnGround) {
        _coyoteTime = 0.10f;
    } else if (_coyoteTime > 0.0f) {
        _coyoteTime -= dt;
    }

    if (_jumpBuffer > 0.0f) {
        _jumpBuffer -= dt;
    }

    if (_attackAnimTimer > 0.0f) {
        _attackAnimTimer -= dt;
        if (_attackAnimTimer <= 0.0f) {
            _currentAttackAnim = AttackAnimType::NONE;
            _attackAnimDuration = 0.0f;
        }
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

    if (_isOnGround || velocity.y >= 0.0f) {
        _isJuggledByEnemy = false;
    }

    // Гравитация (отключается в рывке и во время атак в воздухе)
    if (!_isOnGround && _dashTimer <= 0.0f && !_isAirSuspended) {
        float appliedGravity = pcfg.gravity;
        if (_stunTimer > 0.0f && _isJuggledByEnemy && velocity.y < 0.0f) {
            appliedGravity = pcfg.gravity * 0.18f; // Мягкая гравитация жонглирования в стане ТОЛЬКО при подбросе врагом
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

        // Проверка пробивания закрытой двери кувырком
        if (auto* act = Arena::getActive()) {
            sf::FloatRect rollHitbox = getBounds();
            if (_dashDirX > 0.0f) {
                rollHitbox.size.x += 28.0f;
            } else {
                rollHitbox.position.x -= 28.0f;
                rollHitbox.size.x += 28.0f;
            }
            if (act->checkAndBreakDoorOverlap(rollHitbox, sf::Vector2f(_dashDirX * 600.0f, 0.0f), false)) {
                // Закрытая дверь выбита кувырком! Проносимся сквозь нее
                position.x += _dashDirX * 36.0f;
            }
        }

        // Обновляем текущий кадр кувырка по прогрессу рывка
        float progress = std::clamp(1.0f - (_dashTimer / pcfg.dashDuration), 0.0f, 0.999f);
        int frameIdx = static_cast<int>(progress * _rollAnimation.getImageCount());
        _rollAnimation.setCurrentFrame(frameIdx);

        _ghostSpawnTimer -= dt;
        if (_ghostSpawnTimer <= 0.0f) {
            _ghostSpawnTimer = 0.025f;
            _dashGhosts.emplace_back(_rollAnimation.getCurrentSprite(), 180.0f, 0.15f);
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

    // Защита от застревания в стенах при сильных ударах (De-penetration / Unstuck logic)
    if (_collisionEnabled && arena.overlapsSolid(position.x, position.y, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
        for (float dist = 1.0f; dist <= 64.0f; dist += 1.0f) {
            // В первую очередь выталкиваем ВВЕРХ (из пола или наклонной лестницы), не сдвигая игрока вбок
            if (!arena.overlapsSolid(position.x, position.y - dist, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                position.y -= dist;
                if (velocity.y > 0.0f) velocity.y = 0.0f; // Гасим только скорость падения, сохраняя скорость прыжка!
                break;
            }
            if (!arena.overlapsSolid(position.x - dist, position.y, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                position.x -= dist;
                velocity.x = 0.0f;
                break;
            }
            if (!arena.overlapsSolid(position.x + dist, position.y, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                position.x += dist;
                velocity.x = 0.0f;
                break;
            }
            if (!arena.overlapsSolid(position.x, position.y + dist, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                position.y += dist;
                velocity.y = 0.0f;
                break;
            }
        }
    }

    // Применение выпада атаки (lunge) с полноценной проверкой коллизий стен
    if (_pendingLunge != 0.0f) {
        if (_collisionEnabled) {
            moveAxis(position.x, _pendingLunge, true, position.y, arena);
        } else {
            position.x += _pendingLunge;
        }
        _pendingLunge = 0.0f;
    }

    bool wasGroundedBeforeMove = _isOnGround;
    if (_collisionEnabled) {
        bool hitX = moveAxis(position.x, deltaX, true, position.y, arena);
        if (!hitX && wasGroundedBeforeMove && velocity.y >= 0.0f && std::abs(deltaX) > 0.001f) {
            // Плавный спуск по ступеням ТОЛЬКО при активном горизонтальном беге и если ноги оторвались от поверхности
            if (!arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                constexpr float maxStepDown = 32.0f;
                for (float down = 1.0f; down <= maxStepDown; down += 1.0f) {
                    if (arena.overlapsSolid(position.x, position.y + down, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                        float low = down - 1.0f;
                        float high = down;
                        for (int iter = 0; iter < 4; ++iter) {
                            float mid = (low + high) * 0.5f;
                            if (arena.overlapsSolid(position.x, position.y + mid, HALF_W, TOP_OFFSET, BOTTOM_OFFSET)) {
                                high = mid;
                            } else {
                                low = mid;
                            }
                        }
                        position.y += low;
                        _isOnGround = true;
                        break;
                    }
                }
            }
        }
        if (hitX) {
            if (_dashTimer > 0.0f) {
                if (auto* act = Arena::getActive()) {
                    sf::FloatRect hitBox = getBounds();
                    if (_dashDirX > 0.0f) hitBox.size.x += 28.0f;
                    else { hitBox.position.x -= 28.0f; hitBox.size.x += 28.0f; }
                    if (act->checkAndBreakDoorOverlap(hitBox, sf::Vector2f(_dashDirX * 600.0f, 0.0f), false)) {
                        position.x += _dashDirX * 36.0f;
                    }
                }
            } else if (_stunTimer > 0.0f) {
                if (auto* act = Arena::getActive()) {
                    act->checkAndBreakDoorOverlap(getBounds(), velocity, true);
                }
            }
            float preCollisionSpeedX = std::abs(velocity.x);
            float minSpeed = CombatConfig::getWallBounceMinSpeed();
            if (_stunTimer > 0.0f && preCollisionSpeedX >= minSpeed && !_hasWallBounced && !isDead) {
                _hasWallBounced = true;
                float bounceDmg = CombatConfig::getWallBounceDamage() * 0.75f;
                float forceDamp = CombatConfig::getWallBounceForceDamp();
                float popY = CombatConfig::getWallBouncePopY() * 0.9f;
                float bounceHitstun = CombatConfig::getWallBounceHitstun();

                hp -= bounceDmg;
                if (hp <= 0.0f) {
                    hp = 0.0f;
                    isDead = true;
                }

                float stepSign = (deltaX > 0.0f) ? 1.0f : -1.0f;
                // Сильно сниженная форса (25-30% скорости, макс 220px/s в сторону от стены):
                velocity.x = -stepSign * std::min(220.0f, preCollisionSpeedX * forceDamp);
                velocity.y = popY;
                _stunTimer = std::max(_stunTimer, bounceHitstun);
                _isOnGround = false;

                if (CombatSystem::getActive()) {
                    CombatSystem::getActive()->addSpecialMarker(
                        position + sf::Vector2f(0.0f, -50.0f),
                        "★ WALL SPLAT! (-" + std::to_string(static_cast<int>(bounceDmg)) + " HP) ★",
                        sf::Color(255, 60, 60), 1.15f
                    );
                    CombatSystem::getActive()->triggerShake(0.20f, 6.0f);
                }

                SoundManager::get().play("adw2", 50.0f);

                if (Arena::getActive()) {
                    Arena::getActive()->spawnWallBounceBlood(
                        sf::Vector2f(position.x + stepSign * HALF_W, position.y),
                        sf::Vector2f(-stepSign * 0.8f, -0.6f)
                    );
                }
            } else {
                velocity.x = 0.0f; // При столкновении со стеной скорость отброса гасится, игрок не вдавливается в стену
            }
        }

        bool hitY = moveAxis(position.y, deltaY, false, position.x, arena);

        if (deltaY > 0.0f) {
            if (hitY) {
                // Отскок от пола ТОЛЬКО при ударе врага с дайв-слэмом или при получении урона от слэма
                if (diveSlamBounce) {
                    velocity.y = -280.0f;
                    velocity.x *= 0.65f; // Сохраняем умеренный боковой импульс, не переназначая гигантскую скорость заново
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

        if (_isOnGround && _stunTimer <= 0.0f) {
            _hasWallBounced = false;
        }
    } else {
        position.x += deltaX;
        position.y += deltaY;
    }

    // Ограничение границами арены
    position.x = std::clamp(position.x, HALF_W, arena.getWidth() * TILE_SIZE - HALF_W);
    position.y = std::clamp(position.y, TOP_OFFSET, arena.getHeight() * TILE_SIZE - BOTTOM_OFFSET);

    _animation.setPosition(position);
    _rollAnimation.setPosition(position);

    // Поворот спрайта: запоминаем направление, ориентируемся ТОЛЬКО на движение или рывок!
    // Во время атак и цепочек комбо поворот блокируется (_facingLocked), чтобы ввод направленных
    // атак назад (b1, b2, 11b2) не разворачивал персонажа преждевременно!
    if (isDashing() && _dashDirX != 0.0f) {
        bool faceRight = _dashDirX > 0.0f;
        _animation.setFacing(faceRight);
        _rollAnimation.setFacing(faceRight);
        _turnDelayTimer = 0.0f;
    } else if (!_facingLocked && moveX != 0.0f) {
        bool wantsTurn = (moveX > 0.0f) != isFacingRight();
        if (wantsTurn) {
            _turnDelayTimer += dt;
            if (_turnDelayTimer >= PlayerConfig::get().turnDelay) {
                _animation.setFacing(moveX > 0.0f);
                _rollAnimation.setFacing(moveX > 0.0f);
                _turnDelayTimer = 0.0f;
            }
        } else {
            _turnDelayTimer = 0.0f;
            _animation.setFacing(moveX > 0.0f);
            _rollAnimation.setFacing(moveX > 0.0f);
        }
    } else {
        _turnDelayTimer = 0.0f;
    }

    sf::Color tint = sf::Color::White;
    if (isCounterStanceActive()) {
        tint = sf::Color(100, 240, 255, 255);
    } else if (isStunned()) {
        tint = sf::Color(140, 140, 140, 210);
    } else if (isPerfectDodgeEmpowered()) {
        tint = sf::Color(255, 230, 85, 245);
    } else if (_isSpotDodging) {
        tint = sf::Color(170, 230, 255, 190);
    } else if (isInvulnerable()) {
        tint = sf::Color(130, 215, 255, 220);
    }

    syncAllAnimations(tint);
}

void Player::draw(sf::RenderWindow& window) {
    if (isDead) {
        _animation.setCurrentFrame(0);
        sf::Sprite sp = _animation.getCurrentSprite();
        sp.setColor(sf::Color(180, 50, 50, 200));
        window.draw(sp);
        return;
    }

    for (const auto& ghost : _dashGhosts) {
        window.draw(ghost.sprite);
    }

    float moveX = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) moveX -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) moveX += 1.0f;

    // Лямбда отрисовки булавы (за спиной героя или спереди во время удара)
    auto drawHeldMace = [&](bool behindPlayer) {
        if (!_hasWeapon || isDead) return;
        bool isAttacking = isPlayingAttackAnimation();
        if (behindPlayer == isAttacking) return; // За спиной - только вне замаха

        if (!_heldMaceLoaded) {
            std::string path = resolveAssetPath("assets/sprites/mace_tenderizer.png");
            _heldMaceLoaded = _heldMaceTexture.loadFromFile(path);
        }
        if (_heldMaceLoaded) {
            if (!_heldMaceSprite.has_value()) {
                _heldMaceSprite.emplace(_heldMaceTexture);
            }
            float texW = static_cast<float>(_heldMaceTexture.getSize().x);
            float texH = static_cast<float>(_heldMaceTexture.getSize().y);
            // Размер булавы в игре: ровно в 1.25 раза больше игрока (высота игрока = 64px, 64 * 1.25 = 80px)
            constexpr float playerHeight = static_cast<float>(FRAME_SIZE) * SPRITE_SCALE; // 64.0f
            constexpr float targetMaceH = playerHeight * 1.25f; // 80.0f
            float scaleY = targetMaceH / (texH > 0.0f ? texH : targetMaceH);
            float scaleX = scaleY; // Сохраняем идеальные 1:1 пропорции текстуры без искажения пикселей

            // Точка хвата (grip) на рукояти булавы: 48px при канонической высоте 84px (~57.14%)
            float gripY = texH * (48.0f / 84.0f);
            _heldMaceSprite->setOrigin({texW * 0.5f, gripY});

            float dir = isFacingRight() ? 1.0f : -1.0f;
            // Пологий угол наклона ~15 градусов к горизонту, булава опущена еще ниже к земле
            float rot = isFacingRight() ? -105.0f : 105.0f;
            sf::Vector2f handPos = position + sf::Vector2f(dir * 0.0f, 2.0f);

            if (isAttacking) {
                float progress = std::clamp(1.0f - (_attackAnimTimer / _attackAnimDuration), 0.0f, 1.0f);
                float startRot = isFacingRight() ? -105.0f : 105.0f;
                float swingRange = isFacingRight() ? 150.0f : -150.0f;
                rot = startRot + progress * swingRange;
                handPos += sf::Vector2f(dir * (progress * 18.0f), -progress * 10.0f);
            }

            _heldMaceSprite->setPosition(handPos);
            _heldMaceSprite->setRotation(sf::degrees(rot));
            _heldMaceSprite->setScale({dir * scaleX, scaleY});
            window.draw(*_heldMaceSprite);
        }
    };

    // 1. Отрисовка булавы ЗА СПИНОЙ героя (до тела персонажа)
    drawHeldMace(true);

    if (isDashing()) {
        const auto& pcfg = PlayerConfig::get();
        float progress = std::clamp(1.0f - (_dashTimer / pcfg.dashDuration), 0.0f, 0.999f);
        int frameIdx = static_cast<int>(progress * _rollAnimation.getImageCount());
        _rollAnimation.setCurrentFrame(frameIdx);
        window.draw(_rollAnimation.getCurrentSprite());
    } else if (isPlayingAttackAnimation()) {
        Animation* anim = getAttackAnimation(_currentAttackAnim);
        if (anim) {
            float progress = std::clamp(1.0f - (_attackAnimTimer / _attackAnimDuration), 0.0f, 0.999f);
            int frameIdx = static_cast<int>(progress * anim->getImageCount());
            anim->setCurrentFrame(frameIdx);
            window.draw(anim->getCurrentSprite());
        } else {
            _animation.setCurrentFrame(0);
            window.draw(_animation.getCurrentSprite());
        }
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

    // 2. Отрисовка булавы СФЕРЕДИ (во время замаха атаки)
    drawHeldMace(false);

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

    // Отрисовка защитного барьера блока (F / E: Верхний и Нижний блок, строго на земле)
    if (_blockType != BlockType::NONE && _isOnGround && !isDead) {
        float dir = isFacingRight() ? 1.0f : -1.0f;
        if (_blockType == BlockType::HIGH) {
            // ВЕРХНИЙ БЛОК (стоя): щит на уровне корпуса и головы
            sf::RectangleShape shield({8.0f, 48.0f});
            shield.setOrigin({4.0f, 24.0f});
            shield.setPosition({position.x + dir * 24.0f, position.y - 8.0f});
            if (_blockStunTimer > 0.0f) {
                shield.setFillColor(sf::Color(255, 255, 255, 230));
                shield.setOutlineColor(sf::Color(0, 230, 255, 255));
            } else {
                shield.setFillColor(sf::Color(0, 200, 255, 150));
                shield.setOutlineColor(sf::Color(100, 240, 255, 220));
            }
            shield.setOutlineThickness(2.0f);
            window.draw(shield);
        } else {
            // НИЖНИЙ БЛОК (сидя / держа S): приземистый щит на уровне ног и пола
            sf::RectangleShape shield({10.0f, 32.0f});
            shield.setOrigin({5.0f, 16.0f});
            shield.setPosition({position.x + dir * 24.0f, position.y + 14.0f});
            if (_blockStunTimer > 0.0f) {
                shield.setFillColor(sf::Color(255, 255, 255, 230));
                shield.setOutlineColor(sf::Color(255, 180, 50, 255));
            } else {
                shield.setFillColor(sf::Color(255, 140, 40, 150));
                shield.setOutlineColor(sf::Color(255, 200, 80, 220));
            }
            shield.setOutlineThickness(2.0f);
            window.draw(shield);
        }
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

sf::Vector2f Player::getEffectiveVelocity() const {
    float vx = velocity.x;
    if (_dashTimer > 0.0f) {
        vx = _dashDirX * PlayerConfig::get().dashSpeed;
    } else {
        float moveX = 0.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || _simulatedMoveDir < -0.1f) {
            moveX -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || _simulatedMoveDir > 0.1f) {
            moveX += 1.0f;
        }
        vx += moveX * PlayerConfig::get().moveSpeed;
    }
    return {vx, velocity.y};
}

