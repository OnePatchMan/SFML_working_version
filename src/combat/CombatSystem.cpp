#include "combat/CombatSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include <iostream>
#include <cmath>
#include <algorithm>

CombatSystem::CombatSystem() {
    std::string fontPath = resolveAssetPath("assets/fonts/font.ttf");
    _fontLoaded = _font.openFromFile(fontPath);
}

void CombatSystem::reset() {
    _comboHistory.clear();
    _comboResetTimer = 0.0f;
    _lastFinishedCombo = "";
    _pendingBaseCombo = "";
    _repeatComboCount = 0;
    _comboRepeatTimer = 0.0f;
    _bufferedAttackInput.reset();
    _bufferedInputTimer = 0.0f;
    _currentAttackType = "";
    _comboDisplayName = "";
    _comboDisplayTimer = 0.0f;
    _isAirAttack = false;
    _airSuspendedRemaining = 0.0f;
    _attackTimer = 0.0f;
    _attackCooldown = 0.0f;
    _currentWhiffStun = 0.40f;
    _hasHitThisSwing = false;
    _lastStunReason = StunReason::NONE;
    _shakeTimer = 0.0f;
    _shakeOffset = {0.0f, 0.0f};
    _damageNumbers.clear();
    _hitParticles.clear();
}

void CombatSystem::handleInput(const std::optional<sf::Event>& event, Player& player, const sf::Vector2f& mouseWorld) {
    if (!event) return;
    if (player.isStunned()) return;

    if (const auto* mbPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mbPressed->button == sf::Mouse::Button::Left) {
            triggerAttack(AttackInput::LIGHT, player, mouseWorld);
        } else if (mbPressed->button == sf::Mouse::Button::Right) {
            triggerAttack(AttackInput::HEAVY, player, mouseWorld);
        }
    }

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::J || keyPressed->code == sf::Keyboard::Key::F) {
            triggerAttack(AttackInput::LIGHT, player, mouseWorld);
        } else if (keyPressed->code == sf::Keyboard::Key::K || keyPressed->code == sf::Keyboard::Key::G) {
            triggerAttack(AttackInput::HEAVY, player, mouseWorld);
        }
    }
}

void CombatSystem::triggerAttack(AttackInput input, Player& player, const sf::Vector2f& mouseWorld) {
    if (player.isStunned()) return;

    // Комбо-отмена (Combo Cancel / Gatling):
    // Если игрок успевает нажать следующий удар в окне комбо (_comboResetTimer > 0)
    // и предыдущий удар ПОПАЛ по врагу (_hasHitThisSwing),
    // то игрок НЕ ЖДЕТ весь откат/стан-лок, а отменяет остаток отката и наносит следующий удар СРАЗУ!
    // При промахе (whiff) комбо-отмена строго заблокирована — персонаж наказывается стан-локом!
    bool canComboCancel = (!_comboHistory.empty() && _comboResetTimer > 0.0f && _hasHitThisSwing);
    if (_attackCooldown > 0.0f && !canComboCancel) {
        // Буферизация ввода следующего удара комбо во время активной фазы замаха (только при подтвержденном попадании)
        if (!_comboHistory.empty() && _comboResetTimer > 0.0f && _attackTimer > 0.0f && _hasHitThisSwing) {
            _bufferedAttackInput = input;
            _bufferedInputTimer = 0.18f;
        }
        return;
    }

    // Отменяем остаток кулдауна при успешном продолжении комбо
    _attackCooldown = 0.0f;
    _bufferedAttackInput.reset();
    _bufferedInputTimer = 0.0f;

    _attackDirection = player.isFacingRight() ? 1 : -1;
    _hasHitThisSwing = false;

    bool inAir = !player.isOnGround();
    bool isDash = player.isDashWindow() || player.isDashing();

    if (player.isDashing()) {
        player.cancelDash();
    }
    player.clearDashWindow();

    _isAirAttack = inAir;

    // Определение направления (directional attacks: f, b, u, dw)
    std::string dir = "";
    if (player.isHoldingDown()) {
        dir = "dw";
    } else if (player.isHoldingUp()) {
        dir = "u";
    } else if (player.isHoldingBackward()) {
        dir = "b";
    } else if (player.isHoldingForward()) {
        dir = "f";
    }

    std::string attackNum = (input == AttackInput::LIGHT) ? "1" : "2";
    std::string attackType = "";

    if (inAir) {
        if (isDash) {
            if (dir.empty() || dir == "f") {
                attackType = "ad" + attackNum;
            } else {
                attackType = "ad" + dir + attackNum;
            }
        } else {
            if (dir.empty()) {
                attackType = "a" + attackNum;
            } else {
                attackType = "a" + dir + attackNum;
            }
        }
    } else {
        if (isDash) {
            if (dir.empty() || dir == "f") {
                attackType = "d" + attackNum;
            } else {
                attackType = "d" + dir + attackNum;
            }
        } else {
            if (dir.empty()) {
                attackType = attackNum;
            } else {
                attackType = dir + attackNum;
            }
        }
    }

    // Комбо-правило для воздуха:
    // Персонаж может нанести a1, затем a1 (связка a1a1), но после нее разрешен ТОЛЬКО
    // завершитель adw2 (Dual Air Dive Slam), либо ничего (любые другие атаки, включая a1, отклоняются).
    if (inAir && _comboResetTimer > 0.0f && _comboHistory.size() >= 2) {
        size_t n = _comboHistory.size();
        bool endsWithA1A1 = (_comboHistory[n - 1] == "a1" || _comboHistory[n - 1] == "af1") &&
                            (_comboHistory[n - 2] == "a1" || _comboHistory[n - 2] == "af1");
        if (endsWithA1A1) {
            bool isAdw2 = (attackType == "adw2" || attackType == "addw2");
            if (!isAdw2) {
                return; // Разрешен ТОЛЬКО adw2 либо ничего!
            }
        }
    }

    _currentAttackType = attackType;

    // Контратака после Идеального Уворота
    if (player.isPerfectDodgeEmpowered()) {
        player.consumePerfectDodgeEmpower();
        _attackTimer = 0.16f;
        _attackCooldown = 0.22f;
        _currentWhiffStun = 0.45f;
        _currentDamage = 6.0f; // Мощный урон контратаки
        _knockX = _attackDirection * 580.0f; // Подброс вверх и вперед по диагонали
        _knockY = -435.0f; // Подброс ровно на высоту прыжка + 13px
        _enemyStunTime = 0.50f;
        _attackColor = sf::Color(255, 220, 60); // Сияющий золотой
        _comboDisplayName = "PERFECT COUNTER-STRIKE!";
        _comboDisplayTimer = 1.3f;
        _comboDisplayColor = sf::Color(255, 220, 60);
        player.lunge(25.0f);
        _comboHistory.push_back("counter");
        _comboResetTimer = 0.42f;
        return;
    }

    if (_comboResetTimer <= 0.0f) {
        _comboHistory.clear();
    }
    _comboHistory.push_back(attackType);
    _comboResetTimer = 0.42f; // Короткое окно продолжения комбо цепочки

    auto tokenMatches = [](const std::string& actual, const std::string& expected) -> bool {
        if (actual == expected) return true;
        if ((actual == "df1" && expected == "d1") || (actual == "d1" && expected == "df1")) return true;
        if ((actual == "df2" && expected == "d2") || (actual == "d2" && expected == "df2")) return true;
        if ((actual == "af1" && expected == "a1") || (actual == "a1" && expected == "af1")) return true;
        if ((actual == "af2" && expected == "a2") || (actual == "a2" && expected == "af2")) return true;
        if (actual == "f1" && expected == "1") return true;
        if ((actual == "addw2" && expected == "adw2") || (actual == "adw2" && expected == "addw2")) return true;
        return false;
    };

    auto endsWith = [this, &tokenMatches](const std::vector<std::string>& seq) -> bool {
        if (_comboHistory.size() < seq.size()) return false;
        size_t start = _comboHistory.size() - seq.size();
        for (size_t i = 0; i < seq.size(); ++i) {
            if (!tokenMatches(_comboHistory[start + i], seq[i])) return false;
        }
        return true;
    };

    // ==========================================
    // ПОИСК КОМБО В РЕЕСТРЕ COMBATCONFIG
    // ==========================================
    const ComboEntry* matchedCombo = nullptr;
    for (const auto& entry : CombatConfig::getCombos()) {
        if (endsWith(entry.sequence)) {
            matchedCombo = &entry;
            break; // Первый совпавший комбо (уникальные завершители идут первыми)
        }
    }

    // Проверка спама одного и того же комбо 2 раза подряд:
    // Если игрок делает одно комбо 2 раза подряд в течение 3 секунд после завершения первого,
    // после 2-го раза он входит в стан-лок!
    bool spamStunTriggered = false;
    if (matchedCombo != nullptr) {
        if (matchedCombo->isFinisher) {
            _pendingBaseCombo = "";
            if (_comboRepeatTimer > 0.0f && matchedCombo->id == _lastFinishedCombo) {
                _repeatComboCount++;
                if (_repeatComboCount >= 2) {
                    spamStunTriggered = true;
                    _repeatComboCount = 0;
                    _lastFinishedCombo = "";
                    _comboRepeatTimer = 0.0f;
                }
            } else {
                _lastFinishedCombo = matchedCombo->id;
                _repeatComboCount = 1;
                _comboRepeatTimer = 3.0f; // Окно 3 секунды после завершения первого комбо
            }
        } else {
            // Базовый промежуточный комбо (11, 1f2, 21, 22)
            _pendingBaseCombo = matchedCombo->id;
        }
    }

    AttackStats stats;
    if (matchedCombo != nullptr) {
        stats = matchedCombo->stats;
    } else {
        stats = CombatConfig::getBaseAttack(attackType);
    }

    _attackTimer = stats.activeTime;
    _attackCooldown = stats.playerRecovery;
    _currentWhiffStun = (stats.whiffStun > 0.0f) ? stats.whiffStun : std::max(0.38f, stats.playerRecovery * 2.3f);
    _currentDamage = stats.damage;
    _knockX = _attackDirection * stats.knockX;
    _knockY = stats.knockY;
    _enemyStunTime = stats.enemyStun;
    _attackColor = stats.color;
    _comboDisplayName = stats.displayName;
    _comboDisplayTimer = (matchedCombo && matchedCombo->isUniqueFinisher) ? 1.3f : 0.8f;
    _comboDisplayColor = stats.color;
    player.lunge(stats.lunge);

    // Если комбо повторено 2 раза подряд — штрафной стан-лок игрока
    if (spamStunTriggered) {
        player.triggerStun(0.75f);
        _attackCooldown = 0.85f;
        _lastStunReason = StunReason::SPAM;
        _comboDisplayName += " [SPAM EXHAUSTION: STUNNED!]";
        _comboDisplayColor = sf::Color(255, 75, 75);
        _comboDisplayTimer = 1.6f;
    }
}

void CombatSystem::update(float dt, Player& player, Enemy& enemy) {
    if (_attackCooldown > 0.0f) {
        _attackCooldown -= dt;
    }

    if (!player.isStunned()) {
        _lastStunReason = StunReason::NONE;
    }

    // Блокируем смену взгляда персонажа от клавиш движения во время активной атаки или серии комбо,
    // чтобы ввод направления назад (A при взгляде вправо) не приводил к случайному развороту персонажа!
    player.setFacingLocked(_attackTimer > 0.0f || (_comboResetTimer > 0.0f && !_comboHistory.empty()));

    // Обработка буферизированного ввода следующего удара комбо
    if (_bufferedInputTimer > 0.0f) {
        _bufferedInputTimer -= dt;
        if (_bufferedInputTimer > 0.0f && _bufferedAttackInput.has_value()) {
            if (_hasHitThisSwing) {
                AttackInput nextInput = _bufferedAttackInput.value();
                _bufferedAttackInput.reset();
                _bufferedInputTimer = 0.0f;
                _attackCooldown = 0.0f;
                triggerAttack(nextInput, player, {0.0f, 0.0f});
            }
        } else {
            _bufferedAttackInput.reset();
            _bufferedInputTimer = 0.0f;
        }
    }

    // Таймер окна учета спама комбо (3 секунды)
    if (_comboRepeatTimer > 0.0f) {
        _comboRepeatTimer -= dt;
        if (_comboRepeatTimer <= 0.0f) {
            _comboRepeatTimer = 0.0f;
            _lastFinishedCombo = "";
            _repeatComboCount = 0;
        }
    }

    if (_comboResetTimer > 0.0f) {
        _comboResetTimer -= dt;
        if (_comboResetTimer <= 0.0f) {
            std::string finishedId = _pendingBaseCombo;
            if (finishedId.empty() && !_comboHistory.empty() && (_comboHistory[0] == "a1" || _comboHistory[0] == "af1")) {
                finishedId = "a1";
            }
            _comboHistory.clear();
            if (!finishedId.empty()) {
                if (_comboRepeatTimer > 0.0f && finishedId == _lastFinishedCombo) {
                    _repeatComboCount++;
                    if (_repeatComboCount >= 2) {
                        player.triggerStun(0.75f);
                        _attackCooldown = 0.85f;
                        _lastStunReason = StunReason::SPAM;
                        _comboDisplayName = "SPAM EXHAUSTION! PLAYER STUNNED (0.75s)!";
                        _comboDisplayColor = sf::Color(255, 75, 75);
                        _comboDisplayTimer = 1.6f;
                        _repeatComboCount = 0;
                        _lastFinishedCombo = "";
                        _comboRepeatTimer = 0.0f;
                    }
                } else {
                    _lastFinishedCombo = finishedId;
                    _repeatComboCount = 1;
                    _comboRepeatTimer = 3.0f; // 3 секунды после завершения первого базового комбо
                }
                _pendingBaseCombo = "";
            }
        }
    }

    if (_comboDisplayTimer > 0.0f) {
        _comboDisplayTimer -= dt;
    }

    // Задержка в воздухе (Air Suspension) активна ТОЛЬКО если атака попала по врагу!
    if (_airSuspendedRemaining > 0.0f) {
        _airSuspendedRemaining -= dt;
        if (_airSuspendedRemaining > 0.0f && !player.isOnGround()) {
            player.setAirSuspended(true);
            player.velocity.y = 0.0f;
        } else {
            player.setAirSuspended(false);
            _airSuspendedRemaining = 0.0f;
        }
    } else {
        player.setAirSuspended(false);
    }

    // Обработка активной фазы удара и хитбокса
    if (_attackTimer > 0.0f) {
        _attackTimer -= dt;

        float hbw = 70.0f;
        float hbh = 46.0f;
        float hby = player.position.y - 20.0f;

        if (_comboDisplayName.find("1-1-b2") != std::string::npos) {
            // Для завершителя 11b2: существенно удлиненный хитбокс захвата спереди,
            // чтобы уверенно доставать противника после отталкивания форсой ударов 1 и 1
            hbw = 125.0f;
            hbh = 54.0f;
            hby = player.position.y - 18.0f;
        } else if (_comboDisplayName.find("UPPERCUT") != std::string::npos) {
            hbw = 70.0f;
            hbh = 70.0f;
            hby = player.position.y - 42.0f;
        } else if (_comboDisplayName.find("SLAM") != std::string::npos || _comboDisplayName.find("THROW") != std::string::npos) {
            hbw = 75.0f;
            hbh = 60.0f;
            hby = player.position.y - 12.0f;
        } else if (_comboDisplayName.find("DIAGONAL") != std::string::npos) {
            hbw = 72.0f;
            hbh = 58.0f;
            hby = player.position.y - 28.0f;
        } else if (_currentAttackType == "b1" || _currentAttackType == "b2") {
            // Для одиночных атак назад (b1, b2) хитбокс охватывает обе стороны от персонажа
            hbw = 96.0f;
            hbh = 50.0f;
            hby = player.position.y - 20.0f;
        }

        float hbx = (_attackDirection > 0) ? (player.position.x + 8.0f) : (player.position.x - 8.0f - hbw);
        if (_comboDisplayName.find("1-1-b2") != std::string::npos) {
            // Захват тянется от тела персонажа (-10px) далеко вперед (+115px),
            // гарантируя поимку отлетевшего от форсы врага
            hbx = (_attackDirection > 0) ? (player.position.x - 10.0f) : (player.position.x + 10.0f - hbw);
        } else if (_currentAttackType == "b1" || _currentAttackType == "b2") {
            hbx = player.position.x - hbw * 0.5f; // Центрирован по персонажу, надежно поражает цель
        }
        _attackHitbox = sf::FloatRect({hbx, hby}, {hbw, hbh});

        if (!_hasHitThisSwing && !enemy.isDead) {
            if (_attackHitbox.findIntersection(enemy.getBounds()).has_value()) {
                _hasHitThisSwing = true;

                if (enemy.isCounterStanceActive()) {
                    // БОТ УСПЕШНО СПАРИРОВАЛ УДАР ИГРОКА (BOT COUNTER-ATTACK)!
                    const auto& ecfg = EnemyConfig::get();
                    enemy.endCounterSuccess();
                    float dir = enemy.isFacingLeft ? -1.0f : 1.0f;
                    player.triggerStun(ecfg.counter.playerStun); // ДОЛГИЙ СТАН ИГРОКА!
                    player.takeDamage(ecfg.counter.damage, dir * ecfg.counter.knockX, ecfg.counter.knockY, ecfg.counter.playerStun);

                    _shakeTimer = 0.22f;
                    _shakeIntensity = 8.0f;
                    _comboDisplayName = "! BOT COUNTER-ATTACK ! PLAYER STUNNED (" + std::to_string(ecfg.counter.playerStun).substr(0, 3) + "s) !";
                    _comboDisplayTimer = 1.6f;
                    _comboDisplayColor = sf::Color(255, 60, 60);

                    // Вспышка искр контратаки
                    for (int i = 0; i < 24; ++i) {
                        HitParticle p;
                        p.position = player.position;
                        float angle = (i * 360.0f / 24.0f) * 3.14159f / 180.0f;
                        float spd = 180.0f + (_rng() % 140);
                        p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                        p.color = sf::Color(255, 70, 70);
                        p.size = 3.5f;
                        p.lifetime = 0.35f;
                        _hitParticles.push_back(p);
                    }

                    _comboHistory.clear();
                    _comboResetTimer = 0.0f;
                    _attackTimer = 0.0f;
                    _attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                } else {
                    // Сначала наносим базовый урон и импульс из таблицы параметров
                    enemy.takeDamage(_currentDamage, _knockX, _knockY, _enemyStunTime);

                // Задерживаем в воздухе ТОЛЬКО ПРИ ПОПАДАНИИ по врагу!
                if (!player.isOnGround()) {
                    _airSuspendedRemaining = 0.22f;
                    player.setAirSuspended(true);
                    player.velocity.y = 0.0f;
                }

                // Особая логика комбо 11b2 (захват и бросок назад на дистанцию прыжка + разворот):
                if (_comboDisplayName.find("1-1-b2") != std::string::npos) {
                    player.turnAround();
                }

                // Особая логика комбо 21db1 (подсечка за спиной и запуск вверх):
                if (_comboDisplayName.find("2-1-db1") != std::string::npos) {
                    player.turnAround();
                    _knockX = (player.isFacingRight() ? 1.0f : -1.0f) * 220.0f;
                }

                // Особая логика комбо a1a1adw2 (падение вместе с врагом в пол и отскок на длину дэша):
                if (_comboDisplayName.find("a1-a1-adw2") != std::string::npos) {
                    _airSuspendedRemaining = 0.0f;
                    player.setAirSuspended(false);
                    player.velocity.y = 1200.0f; // Падают вместе с врагом вниз
                    enemy.velocity.y = 1200.0f;
                    enemy.diveSlamBounce = true;
                    enemy.diveSlamDir = static_cast<float>(_attackDirection);
                }

                // Атака a2 / комбо a1a2 (толчок по прямой на уровень дэша):
                // Выравниваем врага строго по высоте уровня дэша игрока
                if ((_comboDisplayName.find("a2") != std::string::npos || _comboDisplayName.find("a1-a2") != std::string::npos) && 
                    _comboDisplayName.find("SLAM") == std::string::npos && 
                    _comboDisplayName.find("LAUNCH") == std::string::npos) {
                    enemy.position.y = player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H);
                    enemy.velocity.y = 0.0f;
                }

                bool isHeavyOrCombo = (_comboDisplayName.find("COMBO") != std::string::npos) ||
                                      (_currentAttackType == "2") || (_currentAttackType == "d2") || 
                                      (_currentAttackType == "a2") || (_currentAttackType == "ad2");

                _shakeTimer = isHeavyOrCombo ? 0.09f : 0.05f;
                _shakeIntensity = isHeavyOrCombo ? 4.5f : 2.5f;

                // Чистый всплывающий урон (без "CRIT")
                DamageNumber dn;
                dn.position = {enemy.position.x + (static_cast<int>(_rng() % 16) - 8), 
                               enemy.position.y - Enemy::HALF_H - 12.0f};
                dn.text = "-" + std::to_string(static_cast<int>(_currentDamage));
                dn.color = _attackColor;
                _damageNumbers.push_back(dn);

                // Искры попадания
                int pCount = isHeavyOrCombo ? 14 : 7;
                for (int i = 0; i < pCount; ++i) {
                    HitParticle p;
                    p.position = {enemy.position.x + (static_cast<int>(_rng() % 14) - 7), 
                                  enemy.position.y + (static_cast<int>(_rng() % 20) - 10)};
                    float pAngle = ((_rng() % 360) * 3.14159f) / 180.0f;
                    float pSpeed = 90.0f + (_rng() % 180);
                    p.velocity = {std::cos(pAngle) * pSpeed + _attackDirection * 100.0f, 
                                  std::sin(pAngle) * pSpeed - 80.0f};
                    p.color = _attackColor;
                    p.size = 2.5f + (_rng() % 3);
                    _hitParticles.push_back(p);
                }
            }
        }
    }

        // Если время активной фазы замаха истекло:
        if (_attackTimer <= 0.0f) {
            _attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
            if (!_hasHitThisSwing) {
                // WHIFF STUNLOCK: штрафной стан-лок игрока при промахе по врагу
                player.triggerStun(_currentWhiffStun);
                _attackCooldown = _currentWhiffStun;
                _lastStunReason = StunReason::WHIFF;
                _comboHistory.clear();
                _comboResetTimer = 0.0f;
                _bufferedAttackInput.reset();
                _bufferedInputTimer = 0.0f;
                _comboDisplayName = "WHIFF! [STUNLOCK]";
                _comboDisplayColor = sf::Color(255, 110, 110);
                _comboDisplayTimer = 0.65f;
            }
        }
    }

    // -------------------------------------------------------------
    // Проверка попадания атаки противника по игроку (Уворот на месте / Идеальный уворот)
    // -------------------------------------------------------------
    if (enemy.isAttacking() && !enemy.attackHasHit && !enemy.isDead) {
        if (enemy.getAttackHitbox().findIntersection(player.getBounds()).has_value()) {
            if (player.isCounterStanceActive()) {
                // ИГРОК УСПЕШНО СПАРИРОВАЛ УДАР ВРАГА НА КНОПКУ [Q] (COUNTER-ATTACK)!
                player.endCounterSuccess();
                const auto& pcfg = PlayerConfig::get();
                float pDir = player.isFacingRight() ? 1.0f : -1.0f;
                enemy.takeDamage(pcfg.counterDamage, pDir * pcfg.counterKnockX, pcfg.counterKnockY, pcfg.counterEnemyStun);
                enemy.dropCombo();
                enemy.attackHasHit = false;

                _shakeTimer = 0.22f;
                _shakeIntensity = 8.0f;
                _comboDisplayName = "! COUNTER-STRIKE ! ENEMY STUNNED (" + std::to_string(pcfg.counterEnemyStun).substr(0, 3) + "s) !";
                _comboDisplayTimer = 1.6f;
                _comboDisplayColor = sf::Color(0, 240, 255);

                // Вспышка ярких лазурно-голубых искр парирования
                for (int i = 0; i < 28; ++i) {
                    HitParticle p;
                    p.position = enemy.position;
                    float angle = (i * 360.0f / 28.0f) * 3.14159f / 180.0f;
                    float spd = 180.0f + (_rng() % 160);
                    p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                    p.color = sf::Color(0, 230, 255);
                    p.size = 3.5f;
                    p.lifetime = 0.38f;
                    p.alpha = 255.0f;
                    _hitParticles.push_back(p);
                }

                DamageNumber dn;
                dn.position = {enemy.position.x - 20.0f, enemy.position.y - Enemy::HALF_H - 15.0f};
                dn.text = std::to_string(static_cast<int>(pcfg.counterDamage)) + " [COUNTER!]";
                dn.color = sf::Color(0, 240, 255);
                _damageNumbers.push_back(dn);
            } else if (player.isPerfectDodgeActive()) {
                // ИДЕАЛЬНЫЙ УВОРОТ (PERFECT DODGE)!
                // Удар спарирован: противник оглушается, комбо мгновенно прерывается
                player.triggerPerfectDodge();
                enemy.stagger(0.75f);
                enemy.attackHasHit = false;

                _shakeTimer = 0.14f;
                _shakeIntensity = 5.0f;
                _comboDisplayName = "PERFECT DODGE! -> [LMB/RMB] COUNTER!";
                _comboDisplayTimer = 1.4f;
                _comboDisplayColor = sf::Color(255, 220, 60);

                // Вспышка золотых искр идеального уворота
                for (int i = 0; i < 24; ++i) {
                    HitParticle p;
                    p.position = player.position;
                    float angle = (i * 360.0f / 24.0f) * 3.14159f / 180.0f;
                    float spd = 160.0f + (_rng() % 140);
                    p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                    p.color = sf::Color(255, 225, 80);
                    p.size = 3.5f;
                    p.lifetime = 0.35f;
                    p.alpha = 255.0f;
                    _hitParticles.push_back(p);
                }
            } else if (player.isInvulnerable()) {
                // Обычный уворот (кадры неуязвимости):
                // Игрок увернулся! Удар прошел мимо, комбо ИИ мгновенно сбрасывается
                enemy.dropCombo();
                enemy.attackHasHit = false;

                _comboDisplayName = "DODGE!";
                _comboDisplayTimer = 0.5f;
                _comboDisplayColor = sf::Color(140, 220, 255);

                for (int i = 0; i < 8; ++i) {
                    HitParticle p;
                    p.position = player.position;
                    float angle = (i * 360.0f / 8.0f) * 3.14159f / 180.0f;
                    p.velocity = {std::cos(angle) * 90.0f, std::sin(angle) * 90.0f};
                    p.color = sf::Color(150, 220, 255);
                    p.size = 2.5f;
                    p.lifetime = 0.20f;
                    _hitParticles.push_back(p);
                }
            } else {
                // Игрок не увернулся: попадание подтверждено (HIT CONFIRM)!
                enemy.attackHasHit = true;

                _shakeTimer = 0.12f;
                _shakeIntensity = 4.0f;
                player.takeDamage(enemy.getCurrentDamage(), 
                                  enemy.getCurrentKnockX(), 
                                  enemy.getCurrentKnockY(), 
                                  enemy.getCurrentStunTime());

                if (enemy.isCurrentAttackDiveSlam()) {
                    player.diveSlamBounce = true;
                    player.diveSlamDir = enemy.isFacingLeft ? -1.0f : 1.0f;
                }

                // Всплывающий урон по игроку
                DamageNumber dn;
                dn.position = {player.position.x, player.position.y - Player::TOP_OFFSET - 10.0f};
                dn.text = "-" + std::to_string(static_cast<int>(enemy.getCurrentDamage()));
                dn.color = sf::Color(255, 60, 60);
                _damageNumbers.push_back(dn);
            }
        }
    }

    // Обновление всплывающих чисел урона
    for (auto it = _damageNumbers.begin(); it != _damageNumbers.end(); ) {
        it->lifetime -= dt;
        it->position.y -= 45.0f * dt;
        it->alpha -= (255.0f / 0.65f) * dt;
        if (it->lifetime <= 0.0f || it->alpha <= 0.0f) {
            it = _damageNumbers.erase(it);
        } else {
            ++it;
        }
    }

    // Обновление искр
    for (auto it = _hitParticles.begin(); it != _hitParticles.end(); ) {
        it->lifetime -= dt;
        it->position += it->velocity * dt;
        it->velocity.y += 900.0f * dt;
        it->alpha -= (255.0f / 0.28f) * dt;
        if (it->lifetime <= 0.0f || it->alpha <= 0.0f) {
            it = _hitParticles.erase(it);
        } else {
            ++it;
        }
    }

    // Тряска камеры
    if (_shakeTimer > 0.0f) {
        _shakeTimer -= dt;
        float sx = ((static_cast<int>(_rng() % 200) - 100) / 100.0f) * _shakeIntensity;
        float sy = ((static_cast<int>(_rng() % 200) - 100) / 100.0f) * _shakeIntensity;
        _shakeOffset = {sx, sy};
    } else {
        _shakeOffset = {0.0f, 0.0f};
    }
}

void CombatSystem::drawWorld(sf::RenderWindow& window, const Player& player) {
    // 1. Искры попадания
    for (const auto& p : _hitParticles) {
        sf::RectangleShape pr({p.size, p.size});
        pr.setPosition(p.position);
        sf::Color pc = p.color;
        pc.a = static_cast<std::uint8_t>(std::clamp(p.alpha, 0.0f, 255.0f));
        pr.setFillColor(pc);
        window.draw(pr);
    }

    // 2. Хитбокс и динамический росчерк удара
    if (_attackTimer > 0.0f) {
        sf::RectangleShape hbRect({_attackHitbox.size.x, _attackHitbox.size.y});
        hbRect.setPosition(_attackHitbox.position);
        hbRect.setFillColor(sf::Color(_attackColor.r, _attackColor.g, _attackColor.b, 35));
        hbRect.setOutlineThickness(1.5f);
        hbRect.setOutlineColor(sf::Color(_attackColor.r, _attackColor.g, _attackColor.b, 220));
        window.draw(hbRect);

        sf::VertexArray slash(sf::PrimitiveType::TriangleStrip, 6);
        float startX = (_attackDirection > 0) ? _attackHitbox.position.x : (_attackHitbox.position.x + _attackHitbox.size.x);
        float endX = (_attackDirection > 0) ? (_attackHitbox.position.x + _attackHitbox.size.x) : _attackHitbox.position.x;
        float midY = _attackHitbox.position.y + _attackHitbox.size.y * 0.5f;

        float topY = midY - 18.0f;
        float botY = midY + 18.0f;

        if (_comboDisplayName.find("UPPERCUT") != std::string::npos) {
            topY = midY - 30.0f;
            botY = midY + 10.0f;
        } else if (_comboDisplayName.find("SLAM") != std::string::npos || _comboDisplayName.find("THROW") != std::string::npos) {
            topY = midY - 10.0f;
            botY = midY + 30.0f;
        } else if (_comboDisplayName.find("DIAGONAL") != std::string::npos) {
            topY = midY - 24.0f;
            botY = midY + 14.0f;
        }

        sf::Color coreColor = sf::Color::White;
        sf::Color edgeColor = _attackColor;
        edgeColor.a = 210;

        slash[0].position = {startX, topY};
        slash[0].color = edgeColor;
        slash[1].position = {startX + (endX - startX) * 0.35f, midY};
        slash[1].color = coreColor;
        slash[2].position = {endX, midY - 6.0f};
        slash[2].color = edgeColor;
        slash[3].position = {endX, midY + 6.0f};
        slash[3].color = coreColor;
        slash[4].position = {startX + (endX - startX) * 0.35f, midY + 4.0f};
        slash[4].color = edgeColor;
        slash[5].position = {startX, botY};
        slash[5].color = edgeColor;

        window.draw(slash);
    }

    // 3. Всплывающий урон
    if (_fontLoaded) {
        for (const auto& dn : _damageNumbers) {
            sf::Text dtText(_font, toSfString(dn.text), 17);
            dtText.setPosition(dn.position);
            sf::Color c = dn.color;
            c.a = static_cast<std::uint8_t>(std::clamp(dn.alpha, 0.0f, 255.0f));
            dtText.setFillColor(c);
            window.draw(dtText);
        }
    }
}

void CombatSystem::drawUI(sf::RenderWindow& window, const Player* player) {
    if (!_fontLoaded) return;

    // Полоска здоровья игрока в верхнем левом углу
    if (player) {
        float pBarW = 160.0f;
        float pBarH = 10.0f;
        float pBarX = 30.0f;
        float pBarY = 12.0f;

        sf::RectangleShape pBorder({pBarW + 2.0f, pBarH + 2.0f});
        pBorder.setPosition({pBarX - 1.0f, pBarY - 1.0f});
        pBorder.setFillColor(sf::Color(20, 20, 25, 220));
        pBorder.setOutlineThickness(1.0f);
        pBorder.setOutlineColor(sf::Color(70, 75, 90, 220));
        window.draw(pBorder);

        sf::RectangleShape pBg({pBarW, pBarH});
        pBg.setPosition({pBarX, pBarY});
        pBg.setFillColor(sf::Color(45, 15, 15));
        window.draw(pBg);

        if (player->displayedHp > player->hp && player->maxHp > 0.0f) {
            float delayW = std::clamp(pBarW * (player->displayedHp / player->maxHp), 0.0f, pBarW);
            sf::RectangleShape pDelayBar({delayW, pBarH});
            pDelayBar.setPosition({pBarX, pBarY});
            pDelayBar.setFillColor(sf::Color(255, 195, 55));
            window.draw(pDelayBar);
        }

        if (player->hp > 0.0f && player->maxHp > 0.0f) {
            float curW = std::clamp(pBarW * (player->hp / player->maxHp), 0.0f, pBarW);
            sf::RectangleShape pHpBar({curW, pBarH});
            pHpBar.setPosition({pBarX, pBarY});
            pHpBar.setFillColor(sf::Color(75, 215, 85));
            window.draw(pHpBar);
        }

        sf::Text pLabel(_font, toSfString("PLAYER HP: " + std::to_string(static_cast<int>(player->hp))), 11);
        pLabel.setPosition({pBarX + pBarW + 10.0f, pBarY - 2.0f});
        pLabel.setFillColor(sf::Color(210, 235, 215));
        window.draw(pLabel);
    }

    // Отображение текущего комбо баннера
    if (_comboDisplayTimer > 0.0f && !_comboDisplayName.empty()) {
        sf::Text txt(_font, toSfString(_comboDisplayName), 22);
        txt.setPosition({30.0f, 32.0f});
        txt.setFillColor(_comboDisplayColor);
        window.draw(txt);
    }

    // Отображение цепочки ввода (например: "INPUT CHAIN: [1] -> [f2] -> [d1]")
    if (!_comboHistory.empty() && _comboResetTimer > 0.0f) {
        std::string chainStr = "INPUT CHAIN: ";
        for (size_t i = 0; i < _comboHistory.size(); ++i) {
            chainStr += "[" + _comboHistory[i] + "]";
            if (i + 1 < _comboHistory.size()) chainStr += " -> ";
        }
        sf::Text chainTxt(_font, toSfString(chainStr), 15);
        chainTxt.setPosition({30.0f, 62.0f});
        chainTxt.setFillColor(sf::Color(180, 220, 255, 200));
        window.draw(chainTxt);
    }

    // Предупреждение об изнеможении / стан-локе игрока
    if (player && player->isStunned()) {
        std::string sVal = std::to_string(player->getStunTimer());
        if (sVal.size() > 4) sVal = sVal.substr(0, 4);
        sf::Text stunTxt(_font, "", 15);
        if (player->isCounterWhiffStun() || _lastStunReason == StunReason::COUNTER_WHIFF) {
            stunTxt.setString(toSfString("! СТАН-ЛОК ИГРОКА (ПРОМАХ СТОЙКИ КОНТР-АТАКИ [Q]): " + sVal + "с !"));
            stunTxt.setFillColor(sf::Color(255, 120, 80));
        } else if (_lastStunReason == StunReason::WHIFF) {
            stunTxt.setString(toSfString("! СТАН-ЛОК ИГРОКА (ПРОМАХ ПО ВРАГУ / WHIFF LAG): " + sVal + "с !"));
            stunTxt.setFillColor(sf::Color(255, 110, 110));
        } else if (_lastStunReason == StunReason::SPAM) {
            stunTxt.setString(toSfString("! СТАН-ЛОК ИГРОКА (ШТРАФ ЗА СПАМ ОДНОГО КОМБО В ТЕЧЕНИЕ 3с): " + sVal + "с !"));
            stunTxt.setFillColor(sf::Color(255, 75, 75));
        } else if (_lastStunReason == StunReason::COUNTER_WHIFF) {
            stunTxt.setString(toSfString("! СТАН-ЛОК ИГРОКА (ПРОМАХ СТОЙКИ КОНТР-АТАКИ [Q]): " + sVal + "с !"));
            stunTxt.setFillColor(sf::Color(255, 120, 80));
        } else {
            stunTxt.setString(toSfString("! СТАН-ЛОК ИГРОКА: " + sVal + "с !"));
            stunTxt.setFillColor(sf::Color(255, 80, 80));
        }
        stunTxt.setPosition({30.0f, 88.0f});
        window.draw(stunTxt);
    } else if (player && player->isCounterStanceActive()) {
        sf::Text counterTxt(_font, toSfString("[Q] СТОЙКА КОНТР-АТАКИ / ПАРИРОВАНИЯ АКТИВНА!"), 15);
        counterTxt.setPosition({30.0f, 88.0f});
        counterTxt.setFillColor(sf::Color(0, 230, 255));
        window.draw(counterTxt);
    }

    // Нижняя панель подсказок управления
    sf::Text hint1(_font, 
        toSfString("[ЛКМ/J]: 1 | [ПКМ/K]: 2 | [Q]: Контр-атака | [Shift]: Рывок/Уворот | [Tab]: ИИ | [F5]: КВ Баланс | [T]: Атака"), 
        13);
    hint1.setPosition({30.0f, static_cast<float>(window.getSize().y) - 46.0f});
    hint1.setFillColor(sf::Color(220, 220, 235, 210));
    window.draw(hint1);

    sf::Text hint2(_font, 
        toSfString("Комбо: 111 | 222 | 11b2 (бросок назад) | 112 | 1f2d1 | 21db1 (подсечка) | a1a2 | a1a1adw2 | a1adw2 | a2ad1"), 
        13);
    hint2.setPosition({30.0f, static_cast<float>(window.getSize().y) - 26.0f});
    hint2.setFillColor(sf::Color(255, 200, 110, 220));
    window.draw(hint2);
}
