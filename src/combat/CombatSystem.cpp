#include "combat/CombatSystem.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "world/Arena.h"
#include "audio/SoundManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// Непрерывное пересечение отрезка [A, B] с прямоугольником box (AABB Continuous Collision Detection).
// Возвращает true, если отрезок пересекает box, и вычисляет точку первого контакта outHitPoint и нормаль outNormal.
static bool raySegmentAABB(const sf::Vector2f& a, const sf::Vector2f& b, const sf::FloatRect& box,
                           sf::Vector2f& outHitPoint, sf::Vector2f& outNormal) {
    if (box.contains(a)) {
        outHitPoint = a;
        sf::Vector2f diff = b - a;
        float dLen = std::hypot(diff.x, diff.y);
        outNormal = (dLen > 0.001f) ? (-diff / dLen) : sf::Vector2f(-1.0f, 0.0f);
        return true;
    }

    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float tMin = 0.0f;
    float tMax = 1.0f;
    sf::Vector2f normalMin(0.0f, 0.0f);

    float boxMinX = box.position.x;
    float boxMaxX = box.position.x + box.size.x;
    float boxMinY = box.position.y;
    float boxMaxY = box.position.y + box.size.y;

    // Ось X
    if (std::abs(dx) < 1e-6f) {
        if (a.x < boxMinX || a.x > boxMaxX) return false;
    } else {
        float invD = 1.0f / dx;
        float t1 = (boxMinX - a.x) * invD;
        float t2 = (boxMaxX - a.x) * invD;
        sf::Vector2f n1(-1.0f, 0.0f);
        sf::Vector2f n2(1.0f, 0.0f);
        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
        if (t1 > tMin) { tMin = t1; normalMin = n1; }
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return false;
    }

    // Ось Y
    if (std::abs(dy) < 1e-6f) {
        if (a.y < boxMinY || a.y > boxMaxY) return false;
    } else {
        float invD = 1.0f / dy;
        float t1 = (boxMinY - a.y) * invD;
        float t2 = (boxMaxY - a.y) * invD;
        sf::Vector2f n1(0.0f, -1.0f);
        sf::Vector2f n2(0.0f, 1.0f);
        if (t1 > t2) { std::swap(t1, t2); std::swap(n1, n2); }
        if (t1 > tMin) { tMin = t1; normalMin = n1; }
        if (t2 < tMax) tMax = t2;
        if (tMin > tMax) return false;
    }

    if (tMin >= 0.0f && tMin <= 1.0f) {
        outHitPoint = a + sf::Vector2f(dx * tMin, dy * tMin);
        outNormal = normalMin;
        return true;
    }
    return false;
}

CombatSystem::CombatSystem() {
    s_active = this;
    std::string fontPath = resolveAssetPath("assets/fonts/font.ttf");
    _fontLoaded = _font.openFromFile(fontPath);
    std::string macePath = resolveAssetPath("assets/sprites/mace_tenderizer.png");
    _maceTextureLoaded = _maceTexture.loadFromFile(macePath);
}

CombatSystem::~CombatSystem() {
    if (s_active == this) {
        s_active = nullptr;
    }
}

void CombatSystem::reset() {
    _comboHistory.clear();
    _comboResetTimer = 0.0f;
    _pendingBaseCombo = "";
    _comboCooldowns.clear();
    _bufferedAttackInput.reset();
    _bufferedInputTimer = 0.0f;
    _currentAttackType = "";
    _comboDisplayName = "";
    _comboDisplayTimer = 0.0f;
    _isAirAttack = false;
    _airSuspendedRemaining = 0.0f;
    _isAirGrabActive = false;
    _airGrabTimer = 0.0f;
    _grabbedEnemy = nullptr;
    _attackTimer = 0.0f;
    _attackCooldown = 0.0f;
    _airAttackCooldown = 0.0f;
    _airAttacksInCurrentJump = 0;
    _lastAirAttackType = "";
    _currentWhiffStun = 0.40f;
    _hasHitThisSwing = false;
    _lastStunReason = StunReason::NONE;
    _shakeTimer = 0.0f;
    _shakeOffset = {0.0f, 0.0f};
    _damageNumbers.clear();
    _hitParticles.clear();
    _specialComboMarkers.clear();
    _empoweredShockwaves.clear();
    _thrownMaces.clear();
    _comboHitCount = 0;
    _currentDamageScaling = 1.0f;
    _currentHitstunDecay = 1.0f;
    _isEmpoweredStrike = false;
}

void CombatSystem::handleInput(const std::optional<sf::Event>& event, Player& player, const sf::Vector2f& mouseWorld) {
    if (!event || player.isDead) return;
    if (player.isStunned() || _isAirGrabActive) return;

    if (const auto* mbPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mbPressed->button == sf::Mouse::Button::Left) {
            triggerAttack(AttackInput::LIGHT, player, mouseWorld);
        } else if (mbPressed->button == sf::Mouse::Button::Right) {
            triggerAttack(AttackInput::HEAVY, player, mouseWorld);
        } else if (mbPressed->button == sf::Mouse::Button::Middle) {
            // Выброс или подбор оружия на колесико мыши (клик колесика)
            if (player.hasWeapon()) {
                throwWeapon(player);
            } else {
                tryRetrieveWeapon(player);
            }
        }
    }

    // Выброс или подбор оружия на прокрутку колесика мыши
    if (event->getIf<sf::Event::MouseWheelScrolled>()) {
        if (player.hasWeapon()) {
            throwWeapon(player);
        } else {
            tryRetrieveWeapon(player);
        }
    }

    if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->code == sf::Keyboard::Key::Space) {
            registerJump();
        } else if (keyPressed->code == sf::Keyboard::Key::J) {
            triggerAttack(AttackInput::LIGHT, player, mouseWorld);
        } else if (keyPressed->code == sf::Keyboard::Key::K || keyPressed->code == sf::Keyboard::Key::G) {
            triggerAttack(AttackInput::HEAVY, player, mouseWorld);
        } else if (keyPressed->code == sf::Keyboard::Key::R) {
            // Использование хилки (восстановление до макс ХП) на клавишу [R]
            player.useHealFlask();
        }
    }
}

void CombatSystem::registerJump() {
    if (_comboResetTimer > 0.0f && !_comboHistory.empty()) {
        if (_comboHistory.back() != "j") {
            _comboHistory.push_back("j");
        }
        _comboResetTimer = std::max(_comboResetTimer, 0.85f);
    }
}

AttackAnimType CombatSystem::resolveAttackAnimType(const std::string& attackType, const std::string& comboName) {
    if (comboName.find("1-1-b2") != std::string::npos || comboName.find("THROW") != std::string::npos ||
        comboName.find("dw1-j-adu2") != std::string::npos || comboName.find("METEOR SLAM") != std::string::npos) {
        return AttackAnimType::THROW;
    }
    if (comboName.find("b2-1-u2") != std::string::npos || comboName.find("SKY LAUNCHER") != std::string::npos ||
        comboName.find("db1-u1") != std::string::npos || comboName.find("SKY POP-UP") != std::string::npos ||
        comboName.find("1-1-2") != std::string::npos || comboName.find("UPPERCUT") != std::string::npos || 
        attackType == "u1" || attackType == "u2") {
        return AttackAnimType::UPPERCUT;
    }
    if (comboName.find("1-2-f1") != std::string::npos || comboName.find("BAT SWING") != std::string::npos) {
        return AttackAnimType::TURN_ATTACK;
    }
    if (comboName.find("1-2-1") != std::string::npos || comboName.find("TENDERIZER PUSH") != std::string::npos ||
        comboName.find("1-f2-d1") != std::string::npos || comboName.find("DIAGONAL") != std::string::npos || 
        attackType == "d1" || attackType == "d2" || attackType == "df1" || attackType == "df2") {
        return AttackAnimType::DASH_ATTACK;
    }
    if (comboName.find("2-1-db1") != std::string::npos || comboName.find("2-1-dw1") != std::string::npos || 
        comboName.find("SWEEP") != std::string::npos || 
        attackType == "dw1" || attackType == "dw2" || attackType == "db1" || attackType == "ddw1" || attackType == "ddw2") {
        return AttackAnimType::SWEEP;
    }
    if (comboName.find("DIVE SLAM") != std::string::npos || comboName.find("GROUND BOUNCE SLAM") != std::string::npos || 
        comboName.find("dw1-j-adw1") != std::string::npos || comboName.find("SKY HOOK") != std::string::npos ||
        attackType == "adw1" || attackType == "adw2" || attackType == "addw2" || attackType == "adu2") {
        return AttackAnimType::DIVE_SLAM;
    }
    if (comboName.find("AIR PUSH") != std::string::npos || comboName.find("AIR JUGGLE") != std::string::npos || 
        attackType == "a1" || attackType == "a2" || attackType == "af1" || attackType == "af2" || 
        attackType == "ad1" || attackType == "ad2" || attackType == "adu2") {
        return AttackAnimType::AIR_ATTACK;
    }
    if (attackType == "b1" || attackType == "b2" || attackType == "db2" || 
        comboName.find("BACK SWING") != std::string::npos || comboName.find("TURN JAB") != std::string::npos) {
        return AttackAnimType::TURN_ATTACK;
    }
    if (attackType == "2" || attackType == "f2" || 
        comboName.find("2-2-2") != std::string::npos || comboName.find("HEAVY") != std::string::npos) {
        return AttackAnimType::HEAVY;
    }
    return AttackAnimType::LIGHT;
}

bool CombatSystem::tryRetrieveWeapon(Player& player) {
    if (player.hasWeapon() || player.isDead || player.isStunned() || _isAirGrabActive) return false;

    // Ищем ближайшую упавшую булаву
    float minDist = 110.0f; // Радиус подбора по нажатию кнопки
    auto bestIt = _thrownMaces.end();

    for (auto it = _thrownMaces.begin(); it != _thrownMaces.end(); ++it) {
        if (!it->active) continue;
        float d = std::hypot(player.position.x - it->position.x, 
                             (player.position.y - Player::HALF_H) - it->position.y);
        if (d < minDist) {
            minDist = d;
            bestIt = it;
        }
    }

    if (bestIt != _thrownMaces.end()) {
        player.setHasWeapon(true);
        bestIt->active = false;
        _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -50.0f), 
                                        "MACE RETRIEVED!", sf::Color(100, 255, 140), 0.75f, 0.75f, 1.0f, 255.0f});
        _thrownMaces.erase(bestIt);
        return true;
    }
    return false;
}

void CombatSystem::spawnHealEffect(const sf::Vector2f& playerPos, int remainingFlasks) {
    DamageNumber dn;
    dn.position = {playerPos.x, playerPos.y - Player::HALF_H - 16.0f};
    dn.lifetime = 1.0f;
    dn.color = sf::Color(80, 255, 120);
    dn.text = "+MAX HP! [ХИЛКА] (" + std::to_string(remainingFlasks) + " ост.)";
    _damageNumbers.push_back(dn);

    for (int i = 0; i < 22; ++i) {
        HitParticle p;
        p.position = {playerPos.x + (static_cast<int>(_rng() % 32) - 16),
                      playerPos.y + (static_cast<int>(_rng() % 40) - 20)};
        float angle = (-90.0f + static_cast<float>(_rng() % 60) - 30.0f) * 3.14159f / 180.0f;
        float spd = 80.0f + static_cast<float>(_rng() % 120);
        p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
        p.color = sf::Color(80, 255, 140);
        p.size = 3.5f;
        p.lifetime = 0.45f;
        _hitParticles.push_back(p);
    }

    _specialComboMarkers.push_back({playerPos + sf::Vector2f(0.0f, -50.0f),
                                    "❤ FULL HEAL RESTORED! ❤", sf::Color(80, 255, 140), 1.0f, 1.0f, 1.2f, 255.0f});
    SoundManager::get().play("heal", 90.0f);
}

void CombatSystem::throwWeapon(Player& player, const sf::Vector2f& targetPoint) {
    if (!player.hasWeapon() || player.isDead || player.isStunned() || player.isBlocking() || _isAirGrabActive) return;
    player.cancelSprint();

    sf::Vector2f playerCenter = player.position + sf::Vector2f(0.0f, -Player::HALF_H);

    // 1. Проверяем направление ввода клавиш движения (WASD / стрелки / симуляция)
    float kx = 0.0f;
    float ky = 0.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || player.getSimulatedMoveDir() < -0.1f)  kx -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || player.getSimulatedMoveDir() > 0.1f) kx += 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || player.getSimulatedVerticalDir() < -0.1f)    ky -= 1.0f;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || player.getSimulatedVerticalDir() > 0.1f)  ky += 1.0f;

    float angleRad = 0.0f;
    if (kx != 0.0f || ky != 0.0f) {
        // При движении (например, идем влево на 'A') бросок летит СТРОГО в направлении движения / инпута!
        angleRad = std::atan2(ky, kx);
    } else if (targetPoint != sf::Vector2f(0.0f, 0.0f)) {
        // Если явно передан targetPoint (в юнит-тестах для проверки 8 направлений снаппинга)
        sf::Vector2f diff = targetPoint - playerCenter;
        angleRad = std::atan2(diff.y, diff.x);
    } else {
        // КОГДА СТОИМ НА МЕСТЕ: бросок летит СТРОГО в направлении взгляда персонажа!
        angleRad = player.isFacingRight() ? 0.0f : 3.14159265f;
    }

    // Снаппинг к 8 направлениям (шаг 45 градусов / pi/4):
    // 0: вправо, 45: вниз-вправо, 90: вниз, 135: вниз-влево, 180: влево, 225: вверх-влево, 270: вверх, 315: вверх-вправо
    const float step = 3.14159265f / 4.0f; // 45 градусов
    float snappedAngle = std::round(angleRad / step) * step;

    sf::Vector2f throwDir(std::cos(snappedAngle), std::sin(snappedAngle));

    // Проверяем тип броска:
    // Вверх и вверх-прямо (throwDir.y < -0.3f): закручиваются, зависают в пике, медленно падают (для комбо)
    // Прямо, прямо вниз и вниз (throwDir.y >= -0.3f): летят прямо как копье без вращения
    bool isUpward = (throwDir.y < -0.3f);

    // Скорость полета булавы зависит от эффективной скорости игрока (ходьба, бег, дэш):
    float baseThrowSpeed = CombatConfig::getThrowBaseSpeed();
    if (isUpward && targetPoint == sf::Vector2f(0.0f, 0.0f)) {
        baseThrowSpeed = CombatConfig::getThrowUpwardSpeed();
    }
    sf::Vector2f effVel = player.getEffectiveVelocity();
    float currentSpeed = std::hypot(effVel.x, effVel.y);

    // При броске вверх (для сетапа комбо) вертикальная скорость прыжка (-900 px/s)
    // не должна умножаться и запускать оружие за пределы экрана:
    float effVy = isUpward ? std::max(effVel.y, -80.0f) : effVel.y;
    float playerSpeedInDir = effVel.x * throwDir.x + effVy * throwDir.y;
    float boostMult = isUpward ? std::min(CombatConfig::getThrowSpeedBoostMultiplier(), 0.5f) : CombatConfig::getThrowSpeedBoostMultiplier();
    float boost = std::max(0.0f, playerSpeedInDir * boostMult);
    if (isUpward) {
        boost = std::min(boost, CombatConfig::getThrowMaxVerticalBoost());
    } else if (boost < 1.0f && currentSpeed > 30.0f) {
        boost = currentSpeed * 0.5f;
    }
    float totalSpeed = baseThrowSpeed + boost;

    ThrownMace mace;
    mace.position = playerCenter + throwDir * 8.0f;
    mace.prevPosition = playerCenter;
    mace.isFirstFrame = true;
    mace.velocity = throwDir * totalSpeed;
    if (isUpward && targetPoint == sf::Vector2f(0.0f, 0.0f)) {
        // Ограничиваем вертикальную скорость взлета оружия, чтобы оно оставалось в зоне досягаемости комбо
        float maxVy = -(baseThrowSpeed + CombatConfig::getThrowMaxVerticalBoost());
        if (mace.velocity.y < maxVy) {
            mace.velocity.y = maxVy;
        }
    }
    mace.isUpwardThrow = isUpward;
    mace.reachedApex = false;
    mace.apexHoverTimer = 0.0f;

    if (isUpward) {
        // Броски вверх и вверх-прямо: закручиваются в полете!
        float spinDir = (throwDir.x < -0.1f) ? -1.0f : 1.0f;
        mace.angularVelocity = spinDir * CombatConfig::getThrowUpwardSpinSpeed();
        mace.rotation = 0.0f;
    } else {
        // Броски прямо, прямо вниз и вниз: летят прямо как копье без вращения!
        mace.angularVelocity = 0.0f;
        mace.rotation = std::atan2(mace.velocity.y, mace.velocity.x) * 180.0f / 3.14159265f + 90.0f;
    }

    mace.active = true;
    mace.grounded = false;
    mace.hasHit = false;
    mace.hitCooldown = 0.0f;
    mace.lifetime = 25.0f;
    mace.pickupCooldown = 0.22f;

    _thrownMaces.push_back(mace);

    // Персонаж бросает оружие и временно остается безоружным
    player.setHasWeapon(false);

    SoundManager::get().playAttack("2", "", !player.isOnGround());

    std::string markerText = isUpward ? "COMBO SPIN MACE THROW (UP)" : "SPEAR MACE THROW";
    _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -60.0f), 
                                    markerText, sf::Color(255, 180, 50), 0.75f, 0.75f, 1.0f, 255.0f});
}

void CombatSystem::triggerAttack(AttackInput input, Player& player, const sf::Vector2f& mouseWorld) {
    if (player.isStunned() || player.isBlocking() || _isAirGrabActive) return;

    bool inAir = !player.isOnGround();
    bool isDash = player.isDashWindow() || player.isDashing();

    if (player.isDashing()) {
        player.cancelDash();
    }
    player.cancelSprint();
    player.clearDashWindow();

    _isAirAttack = inAir;

    // -------------------------------------------------------------
    // Определение направления с учетом приоритетов:
    // 1. Нижняя атака (S / Down) — САМЫЙ ВЫСОКИЙ ПРИОРИТЕТ:
    //    Если зажата S, то при любых сочетаниях кнопок (as, ds, ws) ВСЕГДА выбирается нижняя атака ("dw").
    // 2. Атака вверх (W / Up):
    //    Если зажата W (без S), выбирается атака вверх ("u").
    // 3. Горизонтальные направления (A / D, Backward / Forward):
    //    Если S и W не зажаты, проверяются A/D (назад "b" или вперед "f").
    // -------------------------------------------------------------
    std::string dir = "";
    if (player.isHoldingDown()) {
        dir = "dw"; // Нижняя атака имеет наивысший приоритет (as, ds, ws)
    } else if (player.isHoldingUp() && (!_comboHistory.empty() && _comboResetTimer > 0.0f)) {
        dir = "u";  // В цепочке комбо (например, b2-1 -> u2) зажатая W/Up дает атаку вверх "u"
    } else if (player.isHoldingBackward()) {
        dir = "b";
    } else if (player.isHoldingForward()) {
        dir = "f";
    }

    std::string attackNum = (input == AttackInput::LIGHT) ? "1" : "2";
    std::string attackType = "";

    if (inAir) {
        if (dir == "dw") {
            // Нижняя атака в воздухе (Dive Slam adw2 / adw1) — не дублирует префикс дэша
            attackType = "adw" + attackNum;
        } else if (isDash) {
            if (dir == "u") {
                attackType = "adu" + attackNum;
            } else if (dir.empty() || dir == "f") {
                attackType = "ad" + attackNum;
            } else {
                attackType = "ad" + dir + attackNum;
            }
        } else {
            if (dir == "u") {
                attackType = "adu" + attackNum;
            } else if (dir.empty()) {
                attackType = "a" + attackNum;
            } else {
                attackType = "a" + dir + attackNum;
            }
        }
    } else {
        if (dir == "dw") {
            // Нижняя атака на земле (dw1 / dw2) — имеет наивысший приоритет и не конфликтует
            // с движениями дэша (не создает "ddw1", ломавший комбо)
            attackType = "dw" + attackNum;
        } else if (isDash) {
            if (dir.empty() || dir == "u") {
                attackType = "d" + attackNum;
            } else if (dir == "f") {
                attackType = "df" + attackNum;
            } else {
                attackType = "d" + dir + attackNum;
            }
        } else {
            if (dir == "u") {
                attackType = "u" + attackNum;
            } else if (dir.empty()) {
                attackType = attackNum;
            } else {
                attackType = dir + attackNum;
            }
        }
    }

    // Комбо-отмена (Combo Cancel / Gatling):
    // Если игрок успевает нажать следующий удар в окне комбо (_comboResetTimer > 0)
    // и предыдущий удар ПОПАЛ по врагу (_hasHitThisSwing),
    // то игрок НЕ ЖДЕТ весь откат/стан-лок, а отменяет остаток отката и наносит следующий удар СРАЗУ!
    // При промахе (whiff) комбо-отмена строго заблокирована — персонаж наказывается стан-локом!
    // ПРАВИЛА В ВОЗДУХЕ:
    // 1. Нельзя отменить атаку в саму себя (adw1 -> adw1, a2 -> a2) — это не комбо, а спам!
    // 2. adw1 (Air Down Hook) не имеет продолжений и не может быть отменена ни в какую другую атаку!
    bool canComboCancel = (!_comboHistory.empty() && _comboResetTimer > 0.0f && _hasHitThisSwing);
    if (inAir) {
        if (attackType == "adw1" || _lastAirAttackType == attackType || _lastAirAttackType == "adw1") {
            // Исключение: a1 -> a1 при подтвержденном комбо-хите
            if (attackType == "a1" && _lastAirAttackType == "a1" && _airAttacksInCurrentJump < 2 && canComboCancel) {
                // Разрешено для выхода в трипл комбо a1-a1-adw2
            } else {
                canComboCancel = false;
            }
        }
        // Строгий запрет спама одиночной одинаковой атаки в воздухе (adw1, a2, adw2, повтор a1 вне комбо)
        if (_lastAirAttackType == attackType) {
            if (attackType != "a1" || !canComboCancel || _airAttacksInCurrentJump >= 2) {
                return;
            }
        }
        if (attackType == "adw1" && _airAttacksInCurrentJump >= 1) {
            return;
        }
    }

    if ((_attackCooldown > 0.0f || _attackTimer > 0.0f) && !canComboCancel) {
        // Буферизация ввода следующего удара комбо во время активной фазы замаха (только при подтвержденном попадании)
        if (!_comboHistory.empty() && _comboResetTimer > 0.0f && _attackTimer > 0.0f && _hasHitThisSwing && !inAir) {
            _bufferedAttackInput = input;
            _bufferedInputTimer = 0.18f;
        }
        return;
    }

    if (inAir && _airAttackCooldown > 0.0f && !canComboCancel) {
        return;
    }

    // Отменяем остаток кулдауна при успешном продолжении комбо
    _attackCooldown = 0.0f;
    _bufferedAttackInput.reset();
    _bufferedInputTimer = 0.0f;

    _attackDirection = player.isFacingRight() ? 1 : -1;
    _hasHitThisSwing = false;

    // Комбо-правило для воздуха:
    // Персонаж может нанести a1, затем a1 (связка a1a1), но после нее разрешен ТОЛЬКО
    // завершитель adw2 (Dual Air Dive Slam), либо ничего (любые другие атаки, включая a1, отклоняются).
    if (inAir && _comboResetTimer > 0.0f && _comboHistory.size() >= 2) {
        size_t n = _comboHistory.size();
        bool endsWithA1A1 = (_comboHistory[n - 1] == "a1" || _comboHistory[n - 1] == "af1") &&
                            (_comboHistory[n - 2] == "a1" || _comboHistory[n - 2] == "af1");
        if (endsWithA1A1) {
            bool isAdw2 = (attackType == "adw2" || attackType == "addw2" || attackType == "a2" || attackType == "af2" || attackType == "ad2");
            if (!isAdw2) {
                return; // Разрешен ТОЛЬКО adw2 / a2 либо ничего!
            }
        }
    }

    _currentAttackType = attackType;
    _isEmpoweredStrike = false;

    // Контратака после Идеального Уворота
    if (player.isPerfectDodgeEmpowered()) {
        player.consumePerfectDodgeEmpower();
        _isEmpoweredStrike = true;
        _attackTimer = 0.16f;
        _attackCooldown = 0.22f;
        _currentWhiffStun = 0.20f * CombatConfig::getWhiffMultiplier();
        _currentDamage = 6.0f; // Мощный урон контратаки
        _knockX = _attackDirection * 580.0f; // Подброс вверх и вперед по диагонали
        _knockY = -435.0f; // Подброс ровно на высоту прыжка + 13px
        _enemyStunTime = 0.50f;
        _attackColor = sf::Color(255, 220, 60); // Сияющий золотой
        _comboDisplayName = "PERFECT COUNTER-STRIKE!";
        _comboDisplayTimer = 1.3f;
        _comboDisplayColor = sf::Color(255, 220, 60);

        // Спавн визуальных спецэффектов усиленного удара из идеального уворота
        _empoweredShockwaves.push_back({player.position, 14.0f, 95.0f, 0.40f, 0.40f, sf::Color(255, 220, 60)});
        _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -65.0f), 
                                        "★ PERFECT DODGE STRIKE ★", sf::Color(255, 225, 70), 0.9f, 0.9f, 1.25f, 255.0f});

        player.lunge(25.0f);
        player.triggerAttackAnimation(AttackAnimType::UPPERCUT, 0.22f);
        SoundManager::get().playAttack("2", "", false);
        _comboHistory.push_back("counter");
        _comboResetTimer = 0.42f;
        return;
    }

    if (_comboResetTimer <= 0.0f) {
        _comboHistory.clear();
    }
    _comboHistory.push_back(attackType);
    _comboResetTimer = 0.42f; // Короткое окно продолжения комбо цепочки
    if (player.hasWeapon()) {
        if (attackType == "dw1" || attackType == "ddw1") {
            _comboResetTimer = 1.05f; // Больше времени для прыжка и продолжения серии в воздухе
        } else if (attackType == "2" || attackType == "df2" || attackType == "f2") {
            _comboResetTimer = 0.65f; // Комфортное окно для продолжения комбо 1-2 -> f1
        }
    }

    auto tokenMatches = [](const std::string& actual, const std::string& expected) -> bool {
        if (actual == expected) return true;
        if ((actual == "df1" && expected == "d1") || (actual == "d1" && expected == "df1")) return true;
        if ((actual == "df2" && expected == "d2") || (actual == "d2" && expected == "df2")) return true;
        if ((actual == "df1" && expected == "f1") || (actual == "f1" && expected == "df1")) return true;
        if ((actual == "df2" && expected == "f2") || (actual == "f2" && expected == "df2")) return true;
        if ((actual == "db2" && expected == "b2") || (actual == "b2" && expected == "db2")) return true;
        if ((actual == "du2" && expected == "u2") || (actual == "u2" && expected == "du2")) return true;
        if (actual == "2" && expected == "u2") return true;
        if ((actual == "af1" && expected == "a1") || (actual == "a1" && expected == "af1")) return true;
        if ((actual == "af2" && expected == "a2") || (actual == "a2" && expected == "af2")) return true;
        if ((actual == "adw2" && expected == "a2") || (actual == "a2" && expected == "adw2")) return true;
        if ((actual == "ad2" && expected == "a2") || (actual == "a2" && expected == "ad2")) return true;
        if ((actual == "f1" && expected == "1") || (actual == "1" && expected == "f1")) return true;
        if ((actual == "addw2" && expected == "adw2") || (actual == "adw2" && expected == "addw2")) return true;
        if ((actual == "addw1" && expected == "adw1") || (actual == "adw1" && expected == "addw1")) return true;
        if ((actual == "adu2" && expected == "ad2") || (actual == "ad2" && expected == "adu2")) return true;
        if ((actual == "adu2" && expected == "u2") || (actual == "u2" && expected == "adu2")) return true;
        if (actual == "a2" && expected == "adu2") return true;
        if ((actual == "dw1" || actual == "ddw1") && (expected == "db1" || expected == "dw1" || expected == "ddw1")) return true;
        if (actual == "db1" && (expected == "dw1" || expected == "ddw1")) return true;
        if ((actual == "u1" && (expected == "up1" || expected == "u1")) || (actual == "up1" && (expected == "u1" || expected == "up1"))) return true;
        if (actual == "1" && expected == "u1") return true;
        if ((actual == "b1" && (expected == "db1" || expected == "b1")) || (actual == "db1" && (expected == "b1" || expected == "db1"))) return true;
        return false;
    };

    auto endsWith = [this, &tokenMatches](const std::vector<std::string>& seq) -> bool {
        if (_comboHistory.size() >= seq.size()) {
            size_t start = _comboHistory.size() - seq.size();
            bool match = true;
            for (size_t i = 0; i < seq.size(); ++i) {
                if (!tokenMatches(_comboHistory[start + i], seq[i])) {
                    match = false;
                    break;
                }
            }
            if (match) return true;
        }

        // Гибкое сопоставление комбо с прыжками (пропускаем 'j', если игрок прыгнул в тайминг)
        std::vector<std::string> histNoJ;
        for (const auto& s : _comboHistory) {
            if (s != "j") histNoJ.push_back(s);
        }
        std::vector<std::string> seqNoJ;
        for (const auto& s : seq) {
            if (s != "j") seqNoJ.push_back(s);
        }
        if (seqNoJ.size() > 1 && histNoJ.size() >= seqNoJ.size()) {
            size_t start = histNoJ.size() - seqNoJ.size();
            bool match = true;
            for (size_t i = 0; i < seqNoJ.size(); ++i) {
                if (!tokenMatches(histNoJ[start + i], seqNoJ[i])) {
                    match = false;
                    break;
                }
            }
            if (match) return true;
        }
        return false;
    };

    // ==========================================
    // ПОИСК КОМБО В РЕЕСТРЕ COMBATCONFIG
    // Приоритет отдается самой длинной цепочке (например, a1a1a2 приоритетнее a1a2)
    // ==========================================
    const ComboEntry* matchedCombo = nullptr;
    size_t longestMatchLen = 0;
    for (const auto& entry : CombatConfig::getCombos()) {
        // Разделение комбо: оружие (isWeapon) vs рукопашные
        bool isWeaponCombo = entry.isWeapon || (entry.id.rfind("weapon_", 0) == 0) || 
                              (entry.id == "12f1" || entry.id == "b21u2" || entry.id == "b21" ||
                              entry.id == "121" || entry.id == "db1u1" || entry.id == "db1up1" || entry.id == "b1u1" ||
                              entry.id == "dw1jadw1" || entry.id == "dw1adw1" ||
                              entry.id == "dw1jadu2" || entry.id == "dw1adu2");
        if (entry.id == "12" || entry.id == "weapon_12") {
            // Базовый комбо 1-2 (Ground Slam & Pop-Up) разрешен как для рукопашного боя, так и с оружием (начало для 12f1, 12df2, 121)
        } else if (player.hasWeapon()) {
            if (!isWeaponCombo) {
                continue; // С оружием в руках рукопашные комбо заблокированы
            }
        } else {
            if (isWeaponCombo) {
                continue; // Без оружия комбо булавы заблокированы
            }
        }

        if (endsWith(entry.sequence)) {
            if (entry.sequence.size() > longestMatchLen) {
                matchedCombo = &entry;
                longestMatchLen = entry.sequence.size();
            }
        }
    }

    // Проверка анти-спам системы:
    // 1. В воздухе: запрещен повтор одного и того же приема, запрещено более 1 adw1 за прыжок,
    //    и запрещен спам одиночных ударов без связки в комбо.
    bool spamStunTriggered = false;
    if (inAir) {
        if (attackType == "adw1") {
            // adw1 строго 1 раз за прыжок! Любой повтор adw1 в воздухе — мгновенный стан-лок
            if (_airAttacksInCurrentJump >= 1 || _lastAirAttackType == "adw1") {
                spamStunTriggered = true;
            }
        } else if (_lastAirAttackType == attackType && attackType != "a1") {
            // Повтор той же атаки в воздухе (a2 -> a2, adw2 -> adw2)
            spamStunTriggered = true;
        } else if (_airAttacksInCurrentJump >= 2) {
            // Максимум 2 атаки за прыжок (например a1 -> a2 или a1 -> adw2).
            // Исключение: завершение 3-ударного комбо a1a1adw2
            if (!canComboCancel || attackType != "adw2") {
                spamStunTriggered = true;
            }
        } else if (!canComboCancel && _airAttacksInCurrentJump >= 1) {
            if (_lastAirAttackType == attackType) {
                spamStunTriggered = true;
            }
        }
        _airAttacksInCurrentJump++;
        _lastAirAttackType = attackType;
    }

    // 2. Независимый таймер спама для adw1 (даже при прыжках с земли!):
    if (attackType == "adw1") {
        auto it = _comboCooldowns.find("adw1");
        if (it != _comboCooldowns.end() && it->second > 0.0f) {
            spamStunTriggered = true;
            it->second = 0.0f;
        } else {
            _comboCooldowns["adw1"] = 2.0f; // 2 секунды кулдауна на повторный запуск adw1
        }
    }
    if (matchedCombo != nullptr) {
        if (matchedCombo->isFinisher) {
            _pendingBaseCombo = "";
            auto it = _comboCooldowns.find(matchedCombo->id);
            if (it != _comboCooldowns.end() && it->second > 0.0f) {
                // Повтор того же самого комбо в течение 2.5 секунд (не важно, что было между ними)!
                spamStunTriggered = true;
                it->second = 0.0f; // Сбрасываем таймер после срабатывания наказания
            } else {
                // Запуск 2.5-секундного окна блокировки спама для этого конкретного комбо
                _comboCooldowns[matchedCombo->id] = COMBO_SPAM_COOLDOWN;
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
        if (player.hasWeapon() && (attackType == "dw1" || attackType == "ddw1")) {
            if (CombatConfig::hasAttack("weapon_dw1")) {
                stats = CombatConfig::getBaseAttack("weapon_dw1");
            } else {
                stats.damage = 3.2f;
                stats.knockX = 15.0f;
                stats.knockY = -350.0f; // Подброс чуть ниже высоты прыжка (~118px при прыжке 135px)
                stats.playerRecovery = 0.16f;
                stats.enemyStun = 0.45f;
                stats.whiffStun = 0.22f;
                stats.activeTime = 0.14f;
                stats.lunge = 2.0f;
                stats.displayName = "WEAPON STOMP & POP-UP!";
                stats.color = sf::Color(255, 175, 40);
            }
        }
    }

    _attackTimer = stats.activeTime;
    _attackCooldown = stats.playerRecovery;
    if (inAir) {
        _airAttackCooldown = std::max(0.35f, stats.playerRecovery + 0.12f);
    }
    float baseWhiff = (stats.whiffStun > 0.0f) ? stats.whiffStun : std::max(0.18f, stats.playerRecovery * 1.3f);
    _currentWhiffStun = baseWhiff * CombatConfig::getWhiffMultiplier();
    _currentDamage = stats.damage;
    _knockX = _attackDirection * stats.knockX;
    _knockY = stats.knockY;
    _enemyStunTime = stats.enemyStun;
    _currentHitLevel = stats.hitLevel;
    if (inAir || _isAirAttack || attackType.rfind("a", 0) == 0) {
        _currentHitLevel = HitLevel::OVERHEAD; // Все воздушные атаки игрока — строго оверхэды
    }
    _currentIsBackHit = stats.isBackHit;
    if (attackType == "b1" || attackType == "b2") {
        _currentIsBackHit = true;
    }
    _attackColor = stats.color;
    _comboDisplayName = stats.displayName;
    _comboDisplayTimer = (matchedCombo && matchedCombo->isUniqueFinisher) ? 1.3f : 0.8f;
    _comboDisplayColor = stats.color;
    player.lunge(stats.lunge);

    // Пикирование вниз для всех атак Dive Slam в воздухе (a1a1a2, a1a1adw2, adw2, a1adw2):
    // ВНИМАНИЕ: a1a2 НЕ пикирует — персонаж зависает на месте в воздухе и вбивает врага вниз!
    if (inAir && (_comboDisplayName.find("DIVE SLAM") != std::string::npos ||
                  _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos ||
                  attackType == "adw2")) {
        _airSuspendedRemaining = 0.0f;
        player.setAirSuspended(false);
        player.velocity.y = std::max(player.velocity.y, 650.0f); // Стремительное пикирование вниз в пол
    } else if (inAir && _comboDisplayName.find("AIR GROUND SLAM") != std::string::npos) {
        _airSuspendedRemaining = stats.activeTime + 0.15f;
        player.setAirSuspended(true);
        player.velocity.y = 0.0f;
    }

    AttackAnimType animType = resolveAttackAnimType(attackType, _comboDisplayName);
    player.triggerAttackAnimation(animType, std::max(stats.activeTime, stats.playerRecovery));

    std::string comboPrefix = "";
    if (matchedCombo != nullptr && matchedCombo->sequence.size() > 1) {
        for (size_t i = 0; i < matchedCombo->sequence.size() - 1; ++i) {
            comboPrefix += matchedCombo->sequence[i];
        }
    } else if (_comboHistory.size() > 1) {
        for (size_t i = 0; i < _comboHistory.size() - 1; ++i) {
            comboPrefix += _comboHistory[i];
        }
    }
    SoundManager::get().playAttack(attackType, comboPrefix, inAir);

    if (player.hasWeapon() && (attackType == "dw1" || attackType == "ddw1")) {
        _empoweredShockwaves.push_back({sf::Vector2f(player.position.x + _attackDirection * 20.0f, player.position.y), 8.0f, 65.0f, 0.28f, 0.28f, sf::Color(255, 175, 40)});
        _shakeTimer = 0.12f;
        _shakeIntensity = 5.0f;
        SoundManager::get().play("adw2", 65.0f);
    }

    // Если комбо повторено 2 раза подряд или спамятся атаки в воздухе — штрафной стан-лок игрока
    if (spamStunTriggered) {
        player.triggerStun(0.75f);
        _attackCooldown = 0.85f;
        _airAttackCooldown = 0.85f;
        _lastStunReason = StunReason::SPAM;
        _comboDisplayName += inAir ? " [AIR SPAM EXHAUSTION: STUNNED!]" : " [SPAM EXHAUSTION: STUNNED!]";
        _comboDisplayColor = sf::Color(255, 75, 75);
        _comboDisplayTimer = 1.6f;
        player.setAirSuspended(false);
        _airSuspendedRemaining = 0.0f;
        if (player.velocity.y < 200.0f) {
            player.velocity.y = 200.0f; // Падение вниз в стан-локе
        }
        SoundManager::get().play("whiff");
    }
}

void CombatSystem::update(float dt, Player& player, Enemy& enemy) {
    std::vector<Enemy*> list = {&enemy};
    updateInternal(dt, player, list);
}

void CombatSystem::update(float dt, Player& player, std::vector<Enemy>& enemies) {
    std::vector<Enemy*> list;
    list.reserve(enemies.size());
    for (auto& e : enemies) {
        list.push_back(&e);
    }
    updateInternal(dt, player, list);
}

void CombatSystem::updateInternal(float dt, Player& player, const std::vector<Enemy*>& enemiesList) {
    VfxSystem::get().update(dt);
    bool isGroundedNow = player.isOnGround();
    if (_wasGroundedLastFrame && !isGroundedNow && player.velocity.y < -120.0f) {
        registerJump();
    }
    _wasGroundedLastFrame = isGroundedNow;

    if (_attackCooldown > 0.0f) {
        _attackCooldown -= dt;
    }
    if (_airAttackCooldown > 0.0f) {
        _airAttackCooldown -= dt;
    }
    if (player.isOnGround()) {
        _airAttackCooldown = 0.0f;
        _airAttacksInCurrentJump = 0;
        _lastAirAttackType = "";
    }

    if (!player.isStunned()) {
        _lastStunReason = StunReason::NONE;
    }

    // Блокируем смену взгляда персонажа от клавиш движения ТОЛЬКО во время активного замаха атаки (_attackTimer > 0.0f).
    // Между ударами (в окне комбо) разворот свободен с коротким буфером (turnDelay ~1-2 кадра),
    // чтобы игрок мог мгновенно развернуться и атаковать врага сзади без залипания и ложной задней атаки!
    player.setFacingLocked(_attackTimer > 0.0f);

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

    // Обновление таймеров анти-спама для всех завершенных комбо (окно 2.5 секунды)
    for (auto it = _comboCooldowns.begin(); it != _comboCooldowns.end(); ) {
        it->second -= dt;
        if (it->second <= 0.0f) {
            it = _comboCooldowns.erase(it);
        } else {
            ++it;
        }
    }

    if (_comboResetTimer > 0.0f) {
        _comboResetTimer -= dt;
        if (_comboResetTimer <= 0.0f) {
            _comboHitCount = 0;
            _currentDamageScaling = 1.0f;
            _currentHitstunDecay = 1.0f;
            _isEmpoweredStrike = false;
            std::string finishedId = _pendingBaseCombo;
            if (finishedId.empty() && !_comboHistory.empty() && (_comboHistory[0] == "a1" || _comboHistory[0] == "af1")) {
                finishedId = "a1";
            }
            _comboHistory.clear();
            if (!finishedId.empty()) {
                auto it = _comboCooldowns.find(finishedId);
                if (it != _comboCooldowns.end() && it->second > 0.0f) {
                    player.triggerStun(0.75f);
                    _attackCooldown = 0.85f;
                    _lastStunReason = StunReason::SPAM;
                    _comboDisplayName = "SPAM EXHAUSTION! PLAYER STUNNED (0.75s)!";
                    _comboDisplayColor = sf::Color(255, 75, 75);
                    _comboDisplayTimer = 1.6f;
                    it->second = 0.0f;
                } else {
                    _comboCooldowns[finishedId] = COMBO_SPAM_COOLDOWN;
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
    } else if (!_isAirGrabActive) {
        player.setAirSuspended(false);
    }

    player.setAirGrabLocked(_isAirGrabActive);
    if (_isAirGrabActive) {
        _bufferedAttackInput.reset();
        _bufferedInputTimer = 0.0f;
    }

    // Обработка удержания в воздухе и последующего швырка об пол (Air Grab & Meteor Slam)
    if (_isAirGrabActive && _grabbedEnemy) {
        if (_grabbedEnemy->isDead) {
            _isAirGrabActive = false;
            _grabbedEnemy = nullptr;
        } else {
            _airGrabTimer -= dt;
            if (_airGrabTimer > 0.0f) {
                // ФАЗА 1: УДЕРЖАНИЕ В ЗАХВАТЕ БУЛАВОЙ И ПЛАВНЫЙ ПОДЪЕМ В ВОЗДУХЕ
                _grabbedEnemy->position.x = player.position.x + _airGrabDirection * 18.0f;
                _grabbedEnemy->position.y = player.position.y - 14.0f;
                _grabbedEnemy->velocity = {0.0f, 0.0f};
                _grabbedEnemy->triggerStun(0.75f);

                // Мягкий подъем игрока вверх
                player.velocity.y = std::min(player.velocity.y + 350.0f * dt, -50.0f);
                player.setAirSuspended(true);

                // Искры удержания булавой
                if (_rng() % 3 == 0) {
                    HitParticle p;
                    p.position = _grabbedEnemy->position + sf::Vector2f(static_cast<float>(_rng() % 24 - 12), static_cast<float>(_rng() % 24 - 12));
                    p.velocity = {static_cast<float>(_rng() % 100 - 50), static_cast<float>(_rng() % 100 - 50)};
                    p.color = sf::Color(255, 160, 40);
                    p.size = 2.6f;
                    p.lifetime = 0.20f;
                    _hitParticles.push_back(p);
                }
            } else {
                // ФАЗА 2: МОЩНЫЙ БРОСОК ВРАГА ОБ ПОЛ (METEOR SLAM RELEASE)
                _isAirGrabActive = false;
                _airGrabTimer = 0.0f;

                _grabbedEnemy->velocity.x = _airGrabDirection * 75.0f; // легкий сдвиг по X
                _grabbedEnemy->velocity.y = 880.0f;                   // вертикально об пол
                _grabbedEnemy->triggerStun(0.65f);
                _grabbedEnemy->diveSlamBounce = true;
                _grabbedEnemy->diveSlamDir = static_cast<float>(_airGrabDirection);

                // Игрок зависает в воздухе в пиковой точке после швырка
                player.velocity.y = -70.0f;
                player.setAirSuspended(true);
                _airSuspendedRemaining = 0.30f;

                SoundManager::get().playAttack("2", "", true);

                _empoweredShockwaves.push_back({_grabbedEnemy->position, 14.0f, 95.0f, 0.35f, 0.35f, sf::Color(255, 95, 35)});
                for (int i = 0; i < 24; ++i) {
                    HitParticle p;
                    p.position = _grabbedEnemy->position;
                    float angle = (90.0f + static_cast<float>(_rng() % 80) - 40.0f) * 3.14159f / 180.0f;
                    float spd = 280.0f + static_cast<float>(_rng() % 200);
                    p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                    p.color = sf::Color(255, 120, 40);
                    p.size = 3.5f;
                    p.lifetime = 0.38f;
                    _hitParticles.push_back(p);
                }

                _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -55.0f),
                                                "★ AIR GRAB METEOR SLAM! ★", sf::Color(255, 110, 40), 0.85f, 0.85f, 1.3f, 255.0f});
                _grabbedEnemy = nullptr;
            }
        }
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
        } else if (_comboDisplayName.find("dw1-j-adw1") != std::string::npos || _comboDisplayName.find("SKY HOOK") != std::string::npos || _currentAttackType == "adw1") {
            // Удар с зацепом в воздухе (adw1 / dw1jadw1 / dw1adw1):
            // Хитбокс компактный и строго под игроком
            hbw = 54.0f;
            hbh = 65.0f;
            hby = player.position.y + 5.0f;
        } else if (_comboDisplayName.find("dw1-j-adu2") != std::string::npos || _comboDisplayName.find("METEOR SLAM") != std::string::npos || _currentAttackType == "adu2") {
            // Захват врага над игроком или в игроке (dw1-j-adu2):
            hbw = 90.0f;
            hbh = 100.0f;
            hby = player.position.y - 50.0f;
        } else if (player.hasWeapon() && (_currentAttackType == "dw1" || _comboDisplayName.find("STOMP") != std::string::npos)) {
            // Топот булавой по земле
            hbw = 90.0f;
            hbh = 50.0f;
            hby = player.position.y - 12.0f;
        } else if (_comboDisplayName.find("SLAM") != std::string::npos || _comboDisplayName.find("THROW") != std::string::npos) {
            hbw = 75.0f;
            hbh = 60.0f;
            hby = player.position.y - 12.0f;
        } else if (_comboDisplayName.find("DIAGONAL") != std::string::npos) {
            hbw = 72.0f;
            hbh = 58.0f;
            hby = player.position.y - 28.0f;
        } else if (_comboDisplayName.find("1-2-f1") != std::string::npos || _comboDisplayName.find("BAT SWING") != std::string::npos) {
            // 1-2-f1 (удар булавой словно битой) - широкий размашистый хитбокс
            hbw = 140.0f;
            hbh = 65.0f;
            hby = player.position.y - 25.0f;
        } else if (_comboDisplayName.find("b2-1-u2") != std::string::npos || _comboDisplayName.find("SKY LAUNCHER") != std::string::npos) {
            hbw = 85.0f;
            hbh = 75.0f;
            hby = player.position.y - 45.0f;
        } else if (_currentAttackType == "b1" || _currentAttackType == "b2" || _currentAttackType == "db1") {
            // Для одиночных атак назад (b1, b2, db1) хитбокс охватывает обе стороны от персонажа
            hbw = 140.0f;
            hbh = 54.0f;
            hby = player.position.y - 22.0f;
        }

        float hbx = (_attackDirection > 0) ? (player.position.x + 8.0f) : (player.position.x - 8.0f - hbw);
        if (_comboDisplayName.find("1-1-b2") != std::string::npos) {
            // Захват тянется от тела персонажа (-10px) далеко вперед (+115px),
            // гарантируя поимку отлетевшего от форсы врага
            hbx = (_attackDirection > 0) ? (player.position.x - 10.0f) : (player.position.x + 10.0f - hbw);
        } else if (_comboDisplayName.find("dw1-j-adw1") != std::string::npos || _comboDisplayName.find("SKY HOOK") != std::string::npos || _currentAttackType == "adw1") {
            // adw1 центрирован строго ровно под игроком
            hbx = player.position.x - hbw * 0.5f;
        } else if (_comboDisplayName.find("dw1-j-adu2") != std::string::npos || _comboDisplayName.find("METEOR SLAM") != std::string::npos || _currentAttackType == "adu2") {
            // adu2 центрирован вокруг и над игроком
            hbx = player.position.x - hbw * 0.5f;
        } else if (_comboDisplayName.find("1-2-f1") != std::string::npos || _comboDisplayName.find("BAT SWING") != std::string::npos) {
            // 12f1 охватывает от тела игрока (-15px) далеко вперед (+125px)
            hbx = (_attackDirection > 0) ? (player.position.x - 15.0f) : (player.position.x + 15.0f - hbw);
        } else if (_currentAttackType == "b2" || _comboDisplayName.find("b2 (") != std::string::npos) {
            // b2 бьет далеко вперед и охватывает пространство вокруг персонажа (-30px позади до +110px впереди)
            hbx = (_attackDirection > 0) ? (player.position.x - 30.0f) : (player.position.x + 30.0f - hbw);
        } else if (_currentAttackType == "b1" || _currentAttackType == "db1") {
            hbx = player.position.x - hbw * 0.5f; // Центрирован по персонажу, надежно поражает цель сзади и спереди
        }
        _attackHitbox = sf::FloatRect({hbx, hby}, {hbw, hbh});

        for (Enemy* enemyPtr : enemiesList) {
            if (!enemyPtr) continue;
            Enemy& enemy = *enemyPtr;

            if (!_hasHitThisSwing && !enemy.isDead) {
            if (_attackHitbox.findIntersection(enemy.getBounds()).has_value()) {
                _hasHitThisSwing = true;

                if (enemy.isCounterStanceActive()) {
                    // БОТ УСПЕШНО СПАРИРОВАЛ УДАР ИГРОКА (BOT COUNTER-ATTACK)!
                    const auto& ecfg = EnemyConfig::get();
                    enemy.endCounterSuccess();
                    float dir = enemy.isFacingLeft ? -1.0f : 1.0f;
                    player.triggerStun(ecfg.counter.playerStun); // ДОЛГИЙ СТАН ИГРОКА!
                    bool playerWasDead = player.isDead;
                    player.takeDamage(ecfg.counter.damage, dir * ecfg.counter.knockX, ecfg.counter.knockY, ecfg.counter.playerStun);
                    if (!playerWasDead && player.isDead && Arena::getActive()) {
                        Arena::getActive()->spawnKillBlood(player.position, sf::Vector2f(dir * 0.707f, -0.707f), 100.0f);
                    }

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
                    // Инкремент счетчика ударов в серии и расчет скейлинга
                    _comboHitCount++;

                    float dStep = CombatConfig::getDamageScalingStep();
                    float dMin  = CombatConfig::getDamageScalingMin();
                    float hStep = CombatConfig::getHitstunDecayStep();
                    float hMin  = CombatConfig::getHitstunDecayMin();

                    // Скейлинг урона: 100% -> 90% -> 80% ... минимум 30%
                    _currentDamageScaling = std::max(dMin, 1.0f - dStep * static_cast<float>(_comboHitCount - 1));

                    // Снижение стана (Hitstun Decay): уменьшение времени стана в комбо (мин 30%)
                    _currentHitstunDecay  = std::max(hMin, 1.0f - hStep * static_cast<float>(_comboHitCount - 1));

                    // Проверка: является ли текущий удар завершающим комбо (Finisher)
                    bool isFinisher = false;
                    for (const auto& entry : CombatConfig::getCombos()) {
                        if (entry.stats.displayName == _comboDisplayName && (entry.isFinisher || entry.isUniqueFinisher)) {
                            isFinisher = true;
                            break;
                        }
                    }
                    if (!isFinisher && (_comboDisplayName.find("1-1-b2") != std::string::npos || 
                                        _comboDisplayName.find("2-1-db1") != std::string::npos || 
                                        _comboDisplayName.find("a1-a1-adw2") != std::string::npos || 
                                        _comboDisplayName.find("a1-adw2") != std::string::npos || 
                                        _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos ||
                                        _comboDisplayName.find("1-1-2") != std::string::npos ||
                                        _comboDisplayName.find("1-f2-d1") != std::string::npos ||
                                        _comboDisplayName.find("1-2-df2") != std::string::npos ||
                                        _comboDisplayName.find("12df2") != std::string::npos ||
                                        _comboDisplayName.find("1-2-f1") != std::string::npos ||
                                        _comboDisplayName.find("12f1") != std::string::npos ||
                                        _comboDisplayName.find("BAT SWING") != std::string::npos ||
                                        _comboDisplayName.find("b2-1-u2") != std::string::npos ||
                                        _comboDisplayName.find("b21u2") != std::string::npos ||
                                        _comboDisplayName.find("SKY LAUNCHER") != std::string::npos ||
                                        _comboDisplayName.find("dw1-j-adw1") != std::string::npos ||
                                        _comboDisplayName.find("SKY HOOK") != std::string::npos ||
                                        _comboDisplayName.find("a1-a2") != std::string::npos)) {
                        isFinisher = true;
                    }

                    // Скейлинг урона применяется ко всем ударам комбо (100% -> 90% -> 80% ... минимум 30%)
                    float scaledDamage = _currentDamage * _currentDamageScaling;
                    // Снижение стана (Hitstun Decay) уменьшает стан связующих ударов (финишеры сохраняют кинематику запуска)
                    float scaledStun   = isFinisher ? _enemyStunTime : (_enemyStunTime * _currentHitstunDecay);

                    // Все воздушные атаки — строго оверхэды
                    HitLevel pAttackHitLevel = _currentHitLevel;
                    if (_isAirAttack || !player.isOnGround() || _currentAttackType.rfind("a", 0) == 0) {
                        pAttackHitLevel = HitLevel::OVERHEAD;
                    }

                    // Наносим урон и импульс врагу с проверкой блока и добивания
                    // Враг может заблокировать удар ТОЛЬКО если стоит на земле и уже блокирует или находится в блок-стане, но НЕ в чистом хитстане от ударов комбо!
                    bool targetWasBlocking = enemy.isBlocking && enemy.isOnGround && (enemy.hurtTimer <= 0.0f || enemy.blockStunTimer > 0.0f);
                    bool hitFromBehind = _currentIsBackHit;
                    if (enemy.isFacingLeft && player.position.x > enemy.position.x + 5.0f) {
                        hitFromBehind = true;
                    } else if (!enemy.isFacingLeft && player.position.x < enemy.position.x - 5.0f) {
                        hitFromBehind = true;
                    }

                    bool blockSucceeded = false;
                    if (targetWasBlocking && !hitFromBehind) {
                        if (pAttackHitLevel == HitLevel::MID) {
                            blockSucceeded = true;
                        } else if (pAttackHitLevel == HitLevel::OVERHEAD) {
                            if (enemy.getBlockType() == BlockType::HIGH) blockSucceeded = true;
                        } else if (pAttackHitLevel == HitLevel::LOW) {
                            if (enemy.getBlockType() == BlockType::LOW) blockSucceeded = true;
                        }
                    }

                    bool targetWasDeadBefore = (enemy.isDead || enemy.hp <= 0.0f);
                    enemy.takeDamage(scaledDamage, _knockX, _knockY, scaledStun, pAttackHitLevel, _currentIsBackHit, player.position);
                    VfxSystem::get().spawnHitSpark(enemy.position, (_knockX < 0.0f ? -1.0f : 1.0f) * 1.15f, 1.15f);
                    bool enemyJustDied = !targetWasDeadBefore && (enemy.isDead || enemy.hp <= 0.0f);

                    // Особые комбо-маркеры и эффекты при попадании усиленной атаки из идеального уворота
                    if (_isEmpoweredStrike) {
                        if (enemy.getType() == EnemyType::BOSS_1) {
                            enemy.interruptBossComboAndStun(0.85f);
                        }
                        _shakeTimer = 0.28f;
                        _shakeIntensity = 9.5f;

                        // Вспышка ударной волны на враге
                        _empoweredShockwaves.push_back({enemy.position, 12.0f, 90.0f, 0.35f, 0.35f, sf::Color(255, 220, 60)});

                        // 32 яркие золотистые искры идеального контр-удара
                        for (int i = 0; i < 32; ++i) {
                            HitParticle p;
                            p.position = enemy.position;
                            float angle = (i * 360.0f / 32.0f) * 3.14159f / 180.0f;
                            float spd = 200.0f + (_rng() % 160);
                            p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                            p.color = (i % 2 == 0) ? sf::Color(255, 230, 80) : sf::Color(255, 175, 40);
                            p.size = 3.6f;
                            p.lifetime = 0.45f;
                            _hitParticles.push_back(p);
                        }

                        // Особый парящий маркер критического контрудара
                        _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f),
                                                        "★ CRITICAL COUNTER! ★", sf::Color(255, 230, 80), 0.85f, 0.85f, 1.3f, 255.0f});
                    }

                    // 1. СИСТЕМА ЗАЩИТЫ / БЛОКА:
                    if (targetWasBlocking) {
                        if (hitFromBehind) {
                            _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f),
                                                            "★ BACK HIT! ★", sf::Color(255, 60, 60), 0.85f, 0.85f, 1.25f, 255.0f});
                        } else if (!blockSucceeded) {
                            if (_currentHitLevel == HitLevel::LOW) {
                                _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f),
                                                                "★ LOW HIT! ★", sf::Color(255, 160, 40), 0.85f, 0.85f, 1.2f, 255.0f});
                            } else if (_currentHitLevel == HitLevel::OVERHEAD) {
                                _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f),
                                                                "★ OVERHEAD! ★", sf::Color(220, 80, 255), 0.85f, 0.85f, 1.2f, 255.0f});
                            }
                        }
                    }

                    if (targetWasBlocking && blockSucceeded) {
                        float guardDmg = scaledDamage * 1.5f;
                        enemy.damageGuard(guardDmg);
                        if (enemy.isGuardBroken()) {
                            enemy.breakGuard();
                            _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f),
                                                            "★ GUARD BREAK! ★", sf::Color(255, 120, 40), 1.2f, 1.2f, 1.5f, 255.0f});
                            SoundManager::get().play("heavy", 85.0f);
                            _shakeTimer = 0.20f;
                            _shakeIntensity = 6.0f;
                            for (int i = 0; i < 28; ++i) {
                                HitParticle p;
                                p.position = enemy.position;
                                float angle = (i * 360.0f / 28.0f) * 3.14159f / 180.0f;
                                float spd = 180.0f + (_rng() % 160);
                                p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                                p.color = sf::Color(255, 180, 50);
                                p.size = 3.5f;
                                p.lifetime = 0.40f;
                                _hitParticles.push_back(p);
                            }
                        }

                        for (int i = 0; i < 14; ++i) {
                            HitParticle p;
                            p.position = enemy.position;
                            float angle = (i * 360.0f / 14.0f) * 3.14159f / 180.0f;
                            p.velocity = {std::cos(angle) * 110.0f, std::sin(angle) * 110.0f};
                            p.color = sf::Color(100, 220, 255);
                            p.size = 2.5f;
                            p.lifetime = 0.22f;
                            _hitParticles.push_back(p);
                        }

                        // Проверка: завершающий ли удар комбо в блок?
                        bool isFinisher = false;
                        for (const auto& entry : CombatConfig::getCombos()) {
                            if (entry.stats.displayName == _comboDisplayName && (entry.isFinisher || entry.isUniqueFinisher)) {
                                isFinisher = true;
                                break;
                            }
                        }
                        if (!isFinisher && (_comboDisplayName.find("1-1-b2") != std::string::npos || 
                                            _comboDisplayName.find("2-1-db1") != std::string::npos || 
                                            _comboDisplayName.find("a1-a1-adw2") != std::string::npos || 
                                            _comboDisplayName.find("a1-adw2") != std::string::npos || 
                                            _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos ||
                                            _comboDisplayName.find("1-1-2") != std::string::npos ||
                                            _comboDisplayName.find("1-f2-d1") != std::string::npos ||
                                            _comboDisplayName.find("dw1-j-adw1") != std::string::npos ||
                                            _comboDisplayName.find("SKY HOOK") != std::string::npos ||
                                            _comboDisplayName.find("a1-a2") != std::string::npos)) {
                            isFinisher = true;
                        }

                        if (isFinisher) {
                            // После проведения комбо в блок атакующий получает стан-лок,
                            // длительность которого лежит между попаданием и промахом!
                            float midStun = (_attackCooldown + _currentWhiffStun) / 2.0f;
                            if (midStun < 0.25f) midStun = 0.28f;
                            player.triggerStun(midStun);
                            _attackCooldown = midStun;
                            _comboDisplayName += " [COMBO BLOCKED: STUN-LOCK!]";
                            _comboDisplayColor = sf::Color(100, 210, 255);
                            _comboDisplayTimer = 1.2f;
                        }
                    }

                    // 2. ДОБИВАНИЕ: если враг умирает от уникального удара в комбо
                    if (enemy.isDead || enemy.hp <= 0.0f) {
                        bool isUniqueFinisher = _currentAttackType == "(11)2" || _currentAttackType == "(21)db1" ||
                                                _comboDisplayName.find("2-1-db1") != std::string::npos ||
                                                _comboDisplayName.find("1-1-2") != std::string::npos ||
                                                _comboDisplayName.find("1-1-b2") != std::string::npos ||
                                                _comboDisplayName.find("a1-a1-adw2") != std::string::npos ||
                                                _comboDisplayName.find("a1-adw2") != std::string::npos ||
                                                _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos ||
                                                _comboDisplayName.find("1-f2-d1") != std::string::npos ||
                                                _comboDisplayName.find("1-2-df2") != std::string::npos ||
                                                _comboDisplayName.find("12df2") != std::string::npos ||
                                                _comboDisplayName.find("1-2-f1") != std::string::npos ||
                                                _comboDisplayName.find("12f1") != std::string::npos ||
                                                _comboDisplayName.find("BAT SWING") != std::string::npos ||
                                                _comboDisplayName.find("b2-1-u2") != std::string::npos ||
                                                _comboDisplayName.find("b21u2") != std::string::npos ||
                                                _comboDisplayName.find("SKY LAUNCHER") != std::string::npos ||
                                                _comboDisplayName.find("1-2-1") != std::string::npos ||
                                                _comboDisplayName.find("121") != std::string::npos ||
                                                _comboDisplayName.find("TENDERIZER PUSH") != std::string::npos ||
                                                _comboDisplayName.find("db1-u1") != std::string::npos ||
                                                _comboDisplayName.find("db1u1") != std::string::npos ||
                                                _comboDisplayName.find("SKY POP-UP") != std::string::npos ||
                                                _comboDisplayName.find("dw1-j-adw1") != std::string::npos ||
                                                _comboDisplayName.find("SKY HOOK") != std::string::npos ||
                                                _comboDisplayName.find("dw1-j-adu2") != std::string::npos ||
                                                _comboDisplayName.find("METEOR SLAM") != std::string::npos ||
                                                _comboDisplayName.find("UNIQUE") != std::string::npos;
                        if (isUniqueFinisher) {
                            if (Arena::getActive()) {
                                float dirX = (player.isFacingRight() ? 1.0f : -1.0f);
                                sf::Vector2f finisherForceDir;

                                if (_comboDisplayName.find("a1-a1-adw2") != std::string::npos || 
                                    _comboDisplayName.find("a1-adw2") != std::string::npos || 
                                    _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos || 
                                    _currentAttackType == "adw2" || _currentAttackType == "addw2" ||
                                    _comboDisplayName.find("DIVE SLAM") != std::string::npos) {
                                    // a1a1adw2 / a1adw2 (a1s2) бьет вниз по диагонали вперед прямо в пол
                                    finisherForceDir = sf::Vector2f(dirX * 0.707f, 0.707f);
                                } else if (_comboDisplayName.find("dw1-j-adu2") != std::string::npos || 
                                           _comboDisplayName.find("METEOR SLAM") != std::string::npos) {
                                    // dw1-j-adu2 вертикальный слэм вниз об пол
                                    finisherForceDir = sf::Vector2f(dirX * 0.12f, 0.99f);
                                } else if (_comboDisplayName.find("dw1-j-adw1") != std::string::npos || 
                                           _comboDisplayName.find("SKY HOOK") != std::string::npos) {
                                    // dw1-j-adw1 подброс булавой строго вверх в небо
                                    finisherForceDir = sf::Vector2f(dirX * 0.15f, -0.99f);
                                } else if (_comboDisplayName.find("1-1-b2") != std::string::npos || 
                                           _comboDisplayName.find("BACK THROW") != std::string::npos) {
                                    // 1-1-b2 бросок назад и вверх
                                    finisherForceDir = sf::Vector2f(-dirX * 0.75f, -0.66f);
                                } else if (_comboDisplayName.find("1-1-2") != std::string::npos || 
                                           _comboDisplayName.find("UPPERCUT") != std::string::npos) {
                                    // 1-1-2 апперкот вертикально вверх
                                    finisherForceDir = sf::Vector2f(dirX * 0.15f, -0.99f);
                                } else if (_comboDisplayName.find("2-1-db1") != std::string::npos || 
                                           _comboDisplayName.find("SWEEP") != std::string::npos) {
                                    // 2-1-db1 подсечка строго вертикально вверх
                                    finisherForceDir = sf::Vector2f(0.0f, -1.0f);
                                } else if (_comboDisplayName.find("1-f2-d1") != std::string::npos || 
                                           _comboDisplayName.find("DIAGONAL") != std::string::npos) {
                                    // 1-f2-d1 диагональный запуск вверх и вперед
                                    finisherForceDir = sf::Vector2f(dirX * 0.707f, -0.707f);
                                } else if (_comboDisplayName.find("1-2-df2") != std::string::npos || 
                                           _comboDisplayName.find("12df2") != std::string::npos || 
                                           _comboDisplayName.find("DASH SMASH") != std::string::npos) {
                                    // 1-2-df2 мощный горизонтальный удар строго по координате x
                                    finisherForceDir = sf::Vector2f(dirX * 1.0f, 0.0f);
                                } else if (_comboDisplayName.find("1-2-f1") != std::string::npos || 
                                           _comboDisplayName.find("12f1") != std::string::npos || 
                                           _comboDisplayName.find("BAT SWING") != std::string::npos) {
                                    // 1-2-f1 удар словно битой строго по координате x
                                    finisherForceDir = sf::Vector2f(dirX * 1.0f, 0.0f);
                                } else if (_comboDisplayName.find("1-2-1") != std::string::npos || 
                                           _comboDisplayName.find("TENDERIZER PUSH") != std::string::npos) {
                                    // 1-2-1 силовой толчок по координате x
                                    finisherForceDir = sf::Vector2f(dirX * 1.0f, 0.0f);
                                } else if (_comboDisplayName.find("db1-u1") != std::string::npos || 
                                           _comboDisplayName.find("SKY POP-UP") != std::string::npos) {
                                    // db1-u1 подброс вверх под диагональный бросок оружия
                                    finisherForceDir = sf::Vector2f(dirX * 0.5f, -0.866f);
                                } else if (_comboDisplayName.find("b2-1-u2") != std::string::npos || 
                                           _comboDisplayName.find("b21u2") != std::string::npos || 
                                           _comboDisplayName.find("SKY LAUNCHER") != std::string::npos) {
                                    // b2-1-u2 круговой взмах снизу вверх в небо
                                    finisherForceDir = sf::Vector2f(dirX * 0.1f, -0.99f);
                                } else if (_comboDisplayName.find("a1-a2") != std::string::npos || 
                                           _comboDisplayName.find("AIR PUSH") != std::string::npos) {
                                    // a1-a2 толчок по горизонтали вперед
                                    finisherForceDir = sf::Vector2f(dirX * 0.99f, -0.05f);
                                } else {
                                    float len = std::hypot(_knockX, _knockY);
                                    if (len > 0.001f) {
                                        finisherForceDir = sf::Vector2f(_knockX / len, _knockY / len);
                                    } else {
                                        finisherForceDir = sf::Vector2f(dirX * 0.707f, -0.707f);
                                    }
                                }

                                Arena::getActive()->spawnFinisherBlood(enemy.position, finisherForceDir, 65, enemy.maxHp);
                            }
                            _comboDisplayName += " [FINISHER! BLOOD SPLATTER!]";
                            _comboDisplayColor = sf::Color(255, 30, 40);
                            _comboDisplayTimer = 1.6f;
                        }
                    }

                    // Если враг погиб от обычной атаки/серии (не финишера) — спавним аккуратную локальную кровь
                    if (enemyJustDied && !isFinisher && Arena::getActive()) {
                        float pDir = (player.isFacingRight() ? 1.0f : -1.0f);
                        float kLen = std::hypot(_knockX, _knockY);
                        sf::Vector2f knockDir = (kLen > 0.001f) ? sf::Vector2f(_knockX / kLen, _knockY / kLen)
                                                               : sf::Vector2f(pDir * 0.707f, -0.707f);
                        Arena::getActive()->spawnKillBlood(enemy.position, knockDir, enemy.maxHp);
                    }

                // Задерживаем в воздухе ТОЛЬКО ПРИ ПОПАДАНИИ по врагу!
                if (!player.isOnGround()) {
                    if (_comboDisplayName.find("dw1-j-adu2") == std::string::npos &&
                        _comboDisplayName.find("METEOR SLAM") == std::string::npos &&
                        _currentAttackType != "adw1" &&
                        _comboDisplayName.find("adw1") == std::string::npos) {
                        _airSuspendedRemaining = 0.22f;
                        player.setAirSuspended(true);
                        player.velocity.y = 0.0f;
                    } else if (_currentAttackType == "adw1" || _comboDisplayName.find("adw1") != std::string::npos) {
                        // adw1 (удар под себя) направляет персонажа вниз к земле
                        _airSuspendedRemaining = 0.0f;
                        player.setAirSuspended(false);
                        if (player.velocity.y < 140.0f) {
                            player.velocity.y = 140.0f;
                        }
                    }
                }

                // Особая логика комбо 11b2 (захват и бросок назад на дистанцию прыжка + разворот):
                if (_comboDisplayName.find("1-1-b2") != std::string::npos) {
                    player.turnAround();
                }

                // Особая логика комбо db1u1 (удар за себя с разворотом в обратную сторону):
                if ((_currentAttackType == "db1" || _currentAttackType == "b1") && player.hasWeapon()) {
                    player.turnAround();
                }

                // Особая логика комбо 21db1 (подсечка за спиной и запуск вверх):
                if (_comboDisplayName.find("2-1-db1") != std::string::npos || 
                    _comboDisplayName.find("2-1-dw1") != std::string::npos || 
                    _comboDisplayName.find("REVERSE SWEEP") != std::string::npos) {
                    player.turnAround();
                    _knockX = (player.isFacingRight() ? 1.0f : -1.0f) * 220.0f;
                }

                // Особая логика комбо a1a1adw2 / a1a1a2 / a1adw2 (падение вместе с врагом в пол и отскок на длину дэша):
                if (_comboDisplayName.find("a1-a1-adw2") != std::string::npos ||
                    _comboDisplayName.find("a1-a1-a2") != std::string::npos ||
                    _comboDisplayName.find("a1-adw2") != std::string::npos ||
                    _comboDisplayName.find("DUAL AIR DIVE SLAM") != std::string::npos ||
                    _comboDisplayName.find("GROUND BOUNCE SLAM") != std::string::npos) {
                    _airSuspendedRemaining = 0.0f;
                    player.setAirSuspended(false);
                    player.velocity.y = 1200.0f; // Падают вместе с врагом вниз
                    if (enemy.getType() != EnemyType::BOSS_1) {
                        enemy.velocity.y = 1200.0f;
                        enemy.diveSlamBounce = true;
                        enemy.diveSlamDir = static_cast<float>(_attackDirection);
                    }
                } else if (_comboDisplayName.find("a1-a2") != std::string::npos ||
                           _comboDisplayName.find("AIR GROUND SLAM") != std::string::npos) {
                    // a1-a2: Сам игрок остается на месте, а противника сбивает вниз в пол с отскоком!
                    _airSuspendedRemaining = _attackTimer + 0.15f;
                    player.setAirSuspended(true);
                    player.velocity.y = 0.0f;
                    if (enemy.getType() != EnemyType::BOSS_1) {
                        enemy.velocity.y = 850.0f;
                        enemy.diveSlamBounce = true;
                        enemy.diveSlamDir = static_cast<float>(_attackDirection);
                    }
                }

                // Атака a2 / комбо a1a2 (толчок по прямой на уровень дэша):
                // Выравниваем врага строго по высоте уровня дэша игрока
                if (enemy.getType() != EnemyType::BOSS_1 &&
                    (_comboDisplayName.find("a2") != std::string::npos || _comboDisplayName.find("a1-a2") != std::string::npos) && 
                    _comboDisplayName.find("SLAM") == std::string::npos && 
                    _comboDisplayName.find("LAUNCH") == std::string::npos) {
                    enemy.position.y = player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H);
                    enemy.velocity.y = 0.0f;
                }

                // Особая логика комбо dw1jadw1 (зацеп булавой в воздухе и подброс высоко в небо):
                if (_comboDisplayName.find("dw1-j-adw1") != std::string::npos ||
                    _comboDisplayName.find("SKY HOOK") != std::string::npos) {
                    if (enemy.getType() != EnemyType::BOSS_1) {
                        enemy.velocity.x = _attackDirection * 25.0f;
                        enemy.velocity.y = -520.0f;
                        enemy.triggerStun(0.50f);
                    }
                    player.setAirSuspended(true);
                    _airSuspendedRemaining = 0.25f;
                    player.velocity.y = -60.0f; // Легкий подскок игрока в воздухе

                    _empoweredShockwaves.push_back({enemy.position, 10.0f, 75.0f, 0.32f, 0.32f, sf::Color(255, 170, 40)});
                    for (int i = 0; i < 20; ++i) {
                        HitParticle p;
                        p.position = enemy.position;
                        float angle = (-90.0f + static_cast<float>(_rng() % 70) - 35.0f) * 3.14159f / 180.0f;
                        float spd = 200.0f + static_cast<float>(_rng() % 160);
                        p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                        p.color = sf::Color(255, 190, 50);
                        p.size = 3.2f;
                        p.lifetime = 0.35f;
                        _hitParticles.push_back(p);
                    }
                    _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -55.0f),
                                                    "★ SKY HOOK LAUNCH! ★", sf::Color(255, 185, 45), 0.85f, 0.85f, 1.3f, 255.0f});
                }

                // Особая логика комбо dw1jadu2 (захват врага булавой, подъем в воздухе и слэм в пол):
                if (_comboDisplayName.find("dw1-j-adu2") != std::string::npos ||
                    _comboDisplayName.find("METEOR SLAM") != std::string::npos) {
                    _isAirGrabActive = true;
                    _airGrabTimer = 0.24f;
                    _airGrabTotalDuration = 0.24f;
                    _airGrabDirection = _attackDirection;
                    _grabbedEnemy = &enemy;

                    // Хватаем врага и прижимаем прямо перед/над игроком на булаве
                    enemy.position.x = player.position.x + _attackDirection * 18.0f;
                    enemy.position.y = player.position.y - 14.0f;
                    enemy.velocity = {0.0f, 0.0f};
                    enemy.triggerStun(0.75f);

                    // Игрок слегка поднимается в воздух
                    player.velocity.y = -220.0f;
                    player.setAirSuspended(true);
                    _airSuspendedRemaining = 0.0f;

                    _empoweredShockwaves.push_back({enemy.position, 10.0f, 65.0f, 0.25f, 0.25f, sf::Color(255, 140, 40)});
                }

                // Особая логика импульсов комбо: берется строго из KV таблицы баланса (_knockX, _knockY)
                if (enemy.getType() != EnemyType::BOSS_1 &&
                    (_comboDisplayName.find("1-2-df2") != std::string::npos ||
                    _comboDisplayName.find("12df2") != std::string::npos ||
                    _comboDisplayName.find("DASH SMASH") != std::string::npos ||
                    _comboDisplayName.find("1-2-f1") != std::string::npos ||
                    _comboDisplayName.find("12f1") != std::string::npos ||
                    _comboDisplayName.find("BAT SWING") != std::string::npos ||
                    _comboDisplayName.find("1-2-1") != std::string::npos ||
                    _comboDisplayName.find("TENDERIZER PUSH") != std::string::npos ||
                    _comboDisplayName.find("db1-u1") != std::string::npos ||
                    _comboDisplayName.find("SKY POP-UP") != std::string::npos ||
                    _comboDisplayName.find("b2-1-u2") != std::string::npos ||
                    _comboDisplayName.find("b21u2") != std::string::npos ||
                    _comboDisplayName.find("SKY LAUNCHER") != std::string::npos ||
                    _comboDisplayName.find("dw1-j-adw1") != std::string::npos ||
                    _comboDisplayName.find("SKY HOOK") != std::string::npos ||
                    _comboDisplayName.find("COMBO 1-2:") != std::string::npos ||
                    _comboDisplayName.find("b2-1") != std::string::npos ||
                    _currentAttackType == "b2" || _comboDisplayName.find("b2 (") != std::string::npos)) {
                    enemy.velocity.x = _knockX;
                    enemy.velocity.y = _knockY;
                }

                bool isHeavyOrCombo = (_comboDisplayName.find("COMBO") != std::string::npos) ||
                                      (_currentAttackType == "2") || (_currentAttackType == "d2") || 
                                      (_currentAttackType == "a2") || (_currentAttackType == "ad2");

                _shakeTimer = isHeavyOrCombo ? 0.09f : 0.05f;
                _shakeIntensity = isHeavyOrCombo ? 4.5f : 2.5f;

                // Всплывающий урон с учетом скейлинга
                DamageNumber dn;
                dn.position = {enemy.position.x + (static_cast<int>(_rng() % 16) - 8), 
                               enemy.position.y - Enemy::HALF_H - 12.0f};
                if (_isEmpoweredStrike) {
                    dn.text = "★ CRIT: " + std::to_string(static_cast<int>(std::round(scaledDamage))) + " ★";
                    dn.color = sf::Color(255, 230, 80);
                } else {
                    dn.text = "-" + std::to_string(static_cast<int>(std::round(scaledDamage)));
                    dn.color = _attackColor;
                }
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
                _comboHitCount = 0;
                _currentDamageScaling = 1.0f;
                _currentHitstunDecay = 1.0f;
                _isEmpoweredStrike = false;
                int whiffMs = static_cast<int>(std::round(_currentWhiffStun * 1000.0f));
                _comboDisplayName = "WHIFF! (" + std::to_string(whiffMs) + "ms)";
                _comboDisplayColor = sf::Color(255, 110, 110);
                _comboDisplayTimer = 0.65f;
            }
        }
    }

    // -------------------------------------------------------------
    // Проверка попадания атаки противников по игроку (Уворот на месте / Идеальный уворот)
    // -------------------------------------------------------------
    for (Enemy* enemyPtr : enemiesList) {
        if (!enemyPtr) continue;
        Enemy& enemy = *enemyPtr;

        if (enemy.isAttacking() && !enemy.attackHasHit && !enemy.isDead) {
        if (enemy.getAttackHitbox().findIntersection(player.getBounds()).has_value()) {
            if (player.isCounterStanceActive()) {
                // ИГРОК УСПЕШНО СПАРИРОВАЛ УДАР ВРАГА НА КНОПКУ [Q] (COUNTER-ATTACK)!
                player.endCounterSuccess();
                const auto& pcfg = PlayerConfig::get();
                float pDir = player.isFacingRight() ? 1.0f : -1.0f;
                bool enemyWasDeadBefore = (enemy.isDead || enemy.hp <= 0.0f);
                if (enemy.isBurrowed || enemy.isErupting) {
                    enemy.cancelBurrowAndStun(pcfg.counterEnemyStun);
                }
                if (enemy.getType() == EnemyType::BOSS_1) {
                    enemy.interruptBossComboAndStun(pcfg.counterEnemyStun);
                }
                enemy.takeDamage(pcfg.counterDamage, pDir * pcfg.counterKnockX, pcfg.counterKnockY, pcfg.counterEnemyStun);
                if (!enemyWasDeadBefore && (enemy.isDead || enemy.hp <= 0.0f) && Arena::getActive()) {
                    Arena::getActive()->spawnKillBlood(enemy.position, sf::Vector2f(pDir * 0.707f, -0.707f), enemy.maxHp);
                }
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
                // Удар спарирован: противник оглушается (кроме босса), комбо мгновенно прерывается, кд уворота сбрасывается в 0
                player.triggerPerfectDodge();
                if (enemy.getType() != EnemyType::BOSS_1) {
                    enemy.stagger(0.75f);
                }
                enemy.attackHasHit = true; // Текущий замах успешно разрешен (увернулись), чтобы не триггерить повторно в этом замахе

                _shakeTimer = 0.16f;
                _shakeIntensity = 5.0f;
                _comboDisplayName = "★ PERFECT DODGE! ★ -> [LMB/RMB] COUNTER!";
                _comboDisplayTimer = 1.5f;
                _comboDisplayColor = sf::Color(255, 220, 60);

                // Оповещение об идеальном увороте (всплывающий текст над игроком, золотая ударная волна и звук)
                _specialComboMarkers.push_back({
                    player.position + sf::Vector2f(0.0f, -55.0f),
                    "★ PERFECT DODGE! ★",
                    sf::Color(255, 230, 60),
                    1.2f, 1.2f, 1.4f, 255.0f
                });
                SoundManager::get().play("a1", 55.0f);
                _empoweredShockwaves.push_back({
                    player.position,
                    12.0f,
                    80.0f,
                    0.30f,
                    0.30f,
                    sf::Color(255, 225, 70)
                });

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
                enemy.attackHasHit = true;

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

                bool playerWasBlocking = player.isBlocking() && player.isOnGround();
                HitLevel eHitLevel = enemy.getCurrentAttackHitLevel();
                if (!enemy.isOnGround || enemy.currentAttackIsDiveSlam) {
                    eHitLevel = HitLevel::OVERHEAD; // Все воздушные атаки врагов — строго оверхэды
                }
                bool eIsBack = enemy.getCurrentAttackIsBackHit();
                bool hitPlayerFromBehind = eIsBack;
                if (player.isFacingRight() && enemy.position.x < player.position.x - 5.0f) {
                    hitPlayerFromBehind = true;
                } else if (!player.isFacingRight() && enemy.position.x > player.position.x + 5.0f) {
                    hitPlayerFromBehind = true;
                }

                bool playerBlockSucceeded = false;
                if (playerWasBlocking && !hitPlayerFromBehind) {
                    if (eHitLevel == HitLevel::MID) {
                        playerBlockSucceeded = true;
                    } else if (eHitLevel == HitLevel::OVERHEAD) {
                        if (player.getBlockType() == BlockType::HIGH) playerBlockSucceeded = true;
                    } else if (eHitLevel == HitLevel::LOW) {
                        if (player.getBlockType() == BlockType::LOW) playerBlockSucceeded = true;
                    }
                }

                bool playerWasDead = player.isDead;
                player.takeDamage(enemy.getCurrentDamage(), 
                                  enemy.getCurrentKnockX(), 
                                  enemy.getCurrentKnockY(), 
                                  enemy.getCurrentStunTime(),
                                  eHitLevel, eIsBack, enemy.position);
                VfxSystem::get().spawnHitSpark(player.position, (enemy.isFacingLeft ? -1.0f : 1.0f) * 1.2f, 1.2f);
                if (!playerWasDead && player.isDead && Arena::getActive()) {
                    float kLen = std::hypot(enemy.getCurrentKnockX(), enemy.getCurrentKnockY());
                    sf::Vector2f kDir = (kLen > 0.001f) ? sf::Vector2f(enemy.getCurrentKnockX() / kLen, enemy.getCurrentKnockY() / kLen) : sf::Vector2f(0.0f, -1.0f);
                    Arena::getActive()->spawnKillBlood(player.position, kDir, 100.0f);
                }

                if (playerWasBlocking) {
                    if (hitPlayerFromBehind) {
                        _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -50.0f),
                                                        "★ BACK HIT! ★", sf::Color(255, 60, 60), 0.85f, 0.85f, 1.25f, 255.0f});
                    } else if (!playerBlockSucceeded) {
                        if (eHitLevel == HitLevel::LOW) {
                            _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -50.0f),
                                                            "★ LOW HIT! ★", sf::Color(255, 160, 40), 0.85f, 0.85f, 1.2f, 255.0f});
                        } else if (eHitLevel == HitLevel::OVERHEAD) {
                            _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -50.0f),
                                                            "★ OVERHEAD! ★", sf::Color(220, 80, 255), 0.85f, 0.85f, 1.2f, 255.0f});
                        }
                    }
                }

                if (playerWasBlocking && playerBlockSucceeded) {
                    if (player.isGuardBroken()) {
                        _specialComboMarkers.push_back({player.position + sf::Vector2f(0.0f, -50.0f),
                                                        "★ GUARD BREAK! ★", sf::Color(255, 80, 40), 1.2f, 1.2f, 1.5f, 255.0f});
                        SoundManager::get().play("heavy", 90.0f);
                        _shakeTimer = 0.22f;
                        _shakeIntensity = 7.0f;
                        for (int i = 0; i < 28; ++i) {
                            HitParticle p;
                            p.position = player.position;
                            float angle = (i * 360.0f / 28.0f) * 3.14159f / 180.0f;
                            float spd = 180.0f + (_rng() % 160);
                            p.velocity = {std::cos(angle) * spd, std::sin(angle) * spd};
                            p.color = sf::Color(255, 120, 50);
                            p.size = 3.5f;
                            p.lifetime = 0.40f;
                            _hitParticles.push_back(p);
                        }
                    }

                    for (int i = 0; i < 12; ++i) {
                        HitParticle p;
                        p.position = player.position;
                        float angle = (i * 360.0f / 12.0f) * 3.14159f / 180.0f;
                        p.velocity = {std::cos(angle) * 100.0f, std::sin(angle) * 100.0f};
                        p.color = sf::Color(0, 220, 255);
                        p.size = 2.5f;
                        p.lifetime = 0.20f;
                        _hitParticles.push_back(p);
                    }

                    bool isEnemyFinisher = (enemy.aiComboState == AIComboState::LAUNCHER_D1_ACTIVE ||
                                            enemy.aiComboState == AIComboState::STARTER_112_2_ACTIVE ||
                                            enemy.aiComboState == AIComboState::STARTER_21DB1_SWEEP_ACTIVE ||
                                            enemy.aiComboState == AIComboState::AIR_1_ADW2_ACTIVE ||
                                            enemy.aiComboState == AIComboState::AIR_2_ADW2_ACTIVE);
                    if (isEnemyFinisher) {
                        enemy.hurtTimer = 0.40f;
                        enemy.attackCooldown = 0.50f;
                        enemy.dropCombo();
                        enemy.currentComboName = "BOT COMBO BLOCKED! [STUNNED]";
                    }
                }

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
    }

    // Проверка попадания волн пыли от удара о землю Босса 1
    for (auto* enemyPtr : enemiesList) {
        if (!enemyPtr || enemyPtr->isDead || enemyPtr->getType() != EnemyType::BOSS_1) continue;
        for (const auto& wave : enemyPtr->bossDustWaves) {
            if (!wave.active) continue;
            float playerFeetY = player.position.y + Player::BOTTOM_OFFSET;
            float waveDistX = std::abs(player.position.x - wave.x);
            float waveDistY = std::abs(playerFeetY - wave.y);
            if (waveDistX <= 32.0f && waveDistY <= 26.0f && player.isOnGround()) {
                if (!player.isInvulnerable() && player.getStunTimer() <= 0.0f) {
                    player.takeDamage(18.0f, wave.dir * 320.0f, -180.0f, 1.2f); // Стан на 1.2 секунды!
                    DamageNumber dn;
                    dn.position = player.position;
                    dn.text = "-18 [СТАН!]";
                    dn.color = sf::Color(255, 80, 40);
                    _damageNumbers.push_back(dn);
                    triggerShake(0.25f, 6.0f);
                    SoundManager::get().play("hit");
                }
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

    // Обновление особых комбо-маркеров
    for (auto it = _specialComboMarkers.begin(); it != _specialComboMarkers.end(); ) {
        it->lifetime -= dt;
        it->position.y -= 30.0f * dt;
        float progress = 1.0f - (it->lifetime / it->maxLifetime);
        it->alpha = std::clamp(255.0f * (it->lifetime / it->maxLifetime), 0.0f, 255.0f);
        it->scale = 1.0f + 0.35f * (1.0f - std::exp(-progress * 4.0f));
        if (it->lifetime <= 0.0f) {
            it = _specialComboMarkers.erase(it);
        } else {
            ++it;
        }
    }

    // Обновление расширяющихся ударных волн
    for (auto it = _empoweredShockwaves.begin(); it != _empoweredShockwaves.end(); ) {
        it->lifetime -= dt;
        float progress = 1.0f - (it->lifetime / it->maxLifetime);
        it->radius = 12.0f + (it->maxRadius - 12.0f) * std::sqrt(progress);
        if (it->lifetime <= 0.0f) {
            it = _empoweredShockwaves.erase(it);
        } else {
            ++it;
        }
    }

    // -------------------------------------------------------------
    // ОБНОВЛЕНИЕ БРОШЕННЫХ БУЛАВ (THROWN MACE PHYSICS & IMPACT)
    // -------------------------------------------------------------
    for (auto it = _thrownMaces.begin(); it != _thrownMaces.end(); ) {
        // Оружие, лежащее на земле (grounded), НЕ исчезает со временем и ждет подбора игроком!
        if (!it->grounded) {
            it->lifetime -= dt;
            if (it->lifetime <= 0.0f) {
                it = _thrownMaces.erase(it);
                continue;
            }
        }

        if (it->hitCooldown > 0.0f) it->hitCooldown -= dt;
        if (it->pickupCooldown > 0.0f) it->pickupCooldown -= dt;

        float speed = std::hypot(it->velocity.x, it->velocity.y);
        sf::Vector2f startPos = it->position;

        // 1. Полет и физика в пространстве арены
        if (!it->grounded) {
            if (it->hasHit) {
                // Если булава уже попала во врага — она крутится и падает на пол по траектории отскока
                it->rotation += it->angularVelocity * dt;
                if (it->apexHoverTimer > 0.0f) {
                    it->apexHoverTimer -= dt;
                }
                it->velocity.y += CombatConfig::getThrowStraightGravity() * 0.75f * dt;
                it->velocity.x *= (1.0f - 0.75f * dt);
            } else if (it->isUpwardThrow) {
                // Закручивание булавы в воздухе (вверх и вверх-прямо)
                it->rotation += it->angularVelocity * dt;

                // Фазы полета: подъем, зависание в пике, медленное падение (для комбо)
                if (!it->reachedApex) {
                    if (it->velocity.y < -40.0f) {
                        it->velocity.y += CombatConfig::getThrowUpwardUpGravity() * dt;
                        it->velocity.x *= (1.0f - 0.15f * dt);
                    } else {
                        it->reachedApex = true;
                        it->apexHoverTimer = CombatConfig::getThrowUpwardApexHoverTime();
                        it->velocity.y = 0.0f;
                    }
                } else if (it->apexHoverTimer > 0.0f) {
                    it->apexHoverTimer -= dt;
                    it->velocity.y = 0.0f;
                    it->velocity.x *= (1.0f - 0.85f * dt);
                } else {
                    it->velocity.y += CombatConfig::getThrowUpwardFallGravity() * dt;
                    it->velocity.x *= (1.0f - 0.20f * dt);
                }
            } else {
                // Прямые броски (прямо, прямо вниз, вниз) - летят прямо как копье без вращения
                it->velocity.y += CombatConfig::getThrowStraightGravity() * dt;
                it->velocity.x *= (1.0f - 0.08f * dt);

                if (std::hypot(it->velocity.x, it->velocity.y) > 30.0f) {
                    it->rotation = std::atan2(it->velocity.y, it->velocity.x) * 180.0f / 3.14159265f + 90.0f;
                }
            }

            startPos = it->isFirstFrame ? it->prevPosition : it->position;
            it->isFirstFrame = false;
            it->prevPosition = it->position;
            sf::Vector2f targetPos = it->position + it->velocity * dt;
            sf::Vector2f hitPoint, hitNormal;
            sf::Vector2i hitTile;

            // Попадание брошенного оружия (weapon throw) в интерактивную дверь ломает ее (ТОЛЬКО ЕСЛИ ДВЕРЬ ЗАКРЫТА)
            if (Arena::getActive()) {
                int dTx0 = static_cast<int>(std::floor(startPos.x / Arena::getActive()->getTileSize()));
                int dTy0 = static_cast<int>(std::floor(startPos.y / Arena::getActive()->getTileSize()));
                int dTx1 = static_cast<int>(std::floor(targetPos.x / Arena::getActive()->getTileSize()));
                int dTy1 = static_cast<int>(std::floor(targetPos.y / Arena::getActive()->getTileSize()));
                auto* door0 = Arena::getActive()->findInteractiveDoor(dTx0, dTy0);
                auto* door1 = Arena::getActive()->findInteractiveDoor(dTx1, dTy1);
                if (door0 && !door0->isOpen && !door0->isBroken) {
                    Arena::getActive()->breakDoorAt(dTx0, dTy0, it->velocity);
                    triggerShake(0.35f, 9.0f);
                } else if (door1 && !door1->isOpen && !door1->isBroken) {
                    Arena::getActive()->breakDoorAt(dTx1, dTy1, it->velocity);
                    triggerShake(0.35f, 9.0f);
                }
            }

            if (Arena::getActive() && Arena::getActive()->raycastSolid(startPos, targetPos, hitPoint, hitTile, hitNormal)) {
                // Проверяем, не была ли повреждена закрытая дверь в точке столкновения луча
                auto* doorRay = Arena::getActive()->findInteractiveDoor(hitTile.x, hitTile.y);
                if (doorRay && !doorRay->isOpen && !doorRay->isBroken) {
                    Arena::getActive()->breakDoorAt(hitTile.x, hitTile.y, it->velocity);
                    triggerShake(0.35f, 9.0f);
                }

                // Удар булавы-копья о твердый блок карты (стена / пол / потолок)
                float dot = it->velocity.x * hitNormal.x + it->velocity.y * hitNormal.y;
                it->velocity = (it->velocity - 2.0f * dot * hitNormal) * 0.35f;
                it->position = hitPoint + hitNormal * 3.0f;

                // При ударе о пол (землю) вертикальный отскок тяжелой булавы сильно гасится:
                if (hitNormal.y < -0.5f) {
                    it->velocity.y *= 0.20f;
                }

                float bounceSpeed = std::hypot(it->velocity.x, it->velocity.y);
                if (hitNormal.y < -0.5f && (bounceSpeed < 180.0f || it->velocity.y > -90.0f)) {
                    it->grounded = true;
                    it->velocity = {0.0f, 0.0f};
                    it->angularVelocity = 0.0f;
                    it->rotation = 90.0f; // Лежит горизонтально на полу
                }
            } else {
                it->position = targetPos;
            }
        }

        // Обновляем текущую скорость после шага физики
            speed = std::hypot(it->velocity.x, it->velocity.y);

            // 2. Столкновение с врагами (Continuous Collision Detection против туннелирования на любой скорости)
            // Булава бьет СТРОГО 1 РАЗ за бросок и больше ударить не может (!it->hasHit)
            bool canHitEnemy = (!it->hasHit) && (it->hitCooldown <= 0.0f) && 
                               (speed > 80.0f || (it->isUpwardThrow && (it->apexHoverTimer > 0.0f || it->velocity.y > 0.0f)));
            if (canHitEnemy) {
                for (Enemy* enemyPtr : enemiesList) {
                    if (!enemyPtr) continue;
                    Enemy& enemy = *enemyPtr;
                    if (enemy.isDead || enemy.hp <= 0.0f) continue;

                    sf::FloatRect eBounds = enemy.getBounds();
                    // Расширяем хитбокс противника на 8px (радиус ударной части булавы)
                    sf::FloatRect expandedBox({eBounds.position.x - 8.0f, eBounds.position.y - 8.0f},
                                              {eBounds.size.x + 16.0f, eBounds.size.y + 16.0f});

                    sf::Vector2f hitPoint, hitNormal;
                    if (raySegmentAABB(startPos, it->position, expandedBox, hitPoint, hitNormal)) {
                        it->hasHit = true; // Булава нанесла удар и больше в этом броске ударить не может!
                        it->hitCooldown = 999.0f;

                        // Позиционируем оружие строго в точке контакта на поверхности врага (без телепортации в бок)
                        it->position = hitPoint;

                    // Рассчитываем урон пропорционально скорости полета булавы
                    float speedFactor = std::clamp(speed / 950.0f, 0.20f, 1.25f);
                    if (it->isUpwardThrow) speedFactor = std::max(speedFactor, 0.65f);
                    float maceDmg = 1.8f + speedFactor * 3.5f; // ~1.8 .. 6.17 урона

                    sf::Vector2f hitDir = (speed > 0.001f) ? (it->velocity / speed) : sf::Vector2f(1.0f, 0.0f);
                    if (it->isUpwardThrow) {
                        hitDir.y = -0.7f;
                    }
                    // Само попадание еще чуть форсает врага
                    float knockForce = 180.0f + speed * 0.35f;
                    if (it->isUpwardThrow) knockForce = std::max(knockForce, 240.0f);

                    // Проверка блока летящего оружия дуэлянтами (на обычную защиту, не 100%)
                    bool blockedByGuard = false;
                    bool isDuelist = (enemy.getType() == EnemyType::DUELIST || enemy.getType() == EnemyType::DUELIST_PASSIVE);
                    if (isDuelist && enemy.canBlock && !enemy.isDodging && enemy.hurtTimer <= 0.0f) {
                        float blockChance = (enemy.getType() == EnemyType::DUELIST_PASSIVE) ? 0.75f : 0.60f;
                        if (enemy.isBlocking || (static_cast<float>(_rng() % 1000) / 1000.0f <= blockChance)) {
                            blockedByGuard = true;
                            enemy.isBlocking = true;
                            enemy.blockHoldTimer = 0.55f;
                        }
                    }

                    if (blockedByGuard) {
                        // Обычная защита / блок щитом: урон снижен на 80%, короткий блок-стан
                        enemy.blockStunTimer = 0.14f;
                        float hpBefore = enemy.hp;
                        enemy.takeDamage(maceDmg, hitDir.x * knockForce, 0.0f, 0.14f);
                        float dmgDealt = std::max(hpBefore - enemy.hp, 0.0f);

                        // Звук блока и маркер
                        SoundManager::get().play("1", 70.0f);
                        _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -50.0f), 
                                                        "BLOCKED!", sf::Color(80, 210, 255), 0.6f, 0.6f, 1.0f, 255.0f});

                        // Голубые защитные искры блока
                        for (int s = 0; s < 10; ++s) {
                            HitParticle p;
                            p.position = it->position;
                            float ang = (s * 36.0f) * 3.14159f / 180.0f;
                            float sp = 100.0f + static_cast<float>(_rng() % 100);
                            p.velocity = {std::cos(ang) * sp, std::sin(ang) * sp};
                            p.size = 2.5f;
                            p.color = sf::Color(80, 215, 255);
                            p.lifetime = 0.20f;
                            p.alpha = 255.0f;
                            _hitParticles.push_back(p);
                        }

                        // Всплывающий урон при блоке
                        DamageNumber dn;
                        dn.position = it->position;
                        dn.text = "-" + std::to_string(static_cast<int>(std::round(dmgDealt)));
                        dn.color = sf::Color(140, 210, 255);
                        _damageNumbers.push_back(dn);
                    } else {
                        // Прямое попадание без блока
                        if (enemy.getType() != EnemyType::BOSS_1) {
                            enemy.velocity += hitDir * knockForce;
                        }
                        enemy.triggerStun(std::max(enemy.getStunTimer() + 0.38f, 0.50f));
                        enemy.spinTimer = 0.70f;
                        enemy.spinSpeed = 920.0f;

                        bool wasDeadBefore = (enemy.isDead || enemy.hp <= 0.0f);
                        enemy.takeDamage(maceDmg, hitDir.x * knockForce, hitDir.y * knockForce, 0.50f);
                        VfxSystem::get().spawnHitSpark(enemy.position, (hitDir.x < 0.0f ? -1.0f : 1.0f) * 1.3f, 1.3f);

                        // Если бросок был летальным — разлетается куча крови в направлении форсы
                        if (!wasDeadBefore && (enemy.isDead || enemy.hp <= 0.0f)) {
                            if (Arena::getActive()) {
                                Arena::getActive()->spawnFinisherBlood(enemy.position, hitDir, 65, enemy.maxHp);
                            }
                            _specialComboMarkers.push_back({enemy.position + sf::Vector2f(0.0f, -65.0f), 
                                                            "★ LETHAL MACE THROW! ★", sf::Color(255, 60, 50), 1.0f, 1.0f, 1.25f, 255.0f});
                        }

                        // Всплывающий урон
                        DamageNumber dn;
                        dn.position = it->position;
                        dn.text = "-" + std::to_string(static_cast<int>(std::round(maceDmg)));
                        dn.color = sf::Color(255, 175, 40);
                        _damageNumbers.push_back(dn);

                        // Искры попадания
                        for (int s = 0; s < 12; ++s) {
                            HitParticle p;
                            p.position = it->position;
                            float ang = (s * 30.0f) * 3.14159f / 180.0f;
                            float sp = 120.0f + static_cast<float>(_rng() % 140);
                            p.velocity = {std::cos(ang) * sp, std::sin(ang) * sp};
                            p.size = 2.5f + static_cast<float>(_rng() % 3);
                            p.color = sf::Color(255, 200, 60);
                            p.lifetime = 0.22f;
                            p.alpha = 255.0f;
                            _hitParticles.push_back(p);
                        }
                    }

                    // Враг всегда начинает крутиться после попадания булавы
                    enemy.triggerSpin(0.70f, (hitDir.x >= 0.0f ? 1.0f : -1.0f) * 900.0f);

                    // Оружие слегка отскакивает от противника, почти полностью теряя скорость,
                    // а скорость вращения падает, но не столь сильно
                    it->velocity.x = -hitDir.x * 90.0f;
                    it->velocity.y = -85.0f;
                    float baseSpin = (it->angularVelocity != 0.0f) ? it->angularVelocity : ((hitDir.x >= 0.0f ? -1.0f : 1.0f) * 720.0f);
                    it->angularVelocity = baseSpin * 0.55f;
                    it->apexHoverTimer = 0.0f; // Никакого зависания в воздухе
                    it->hitCooldown = 999.0f;  // Больше ударить не может

                    _shakeTimer = 0.06f;
                    _shakeIntensity = 3.5f;
                    break;
                }
            }
        }

        // Подбор булавы осуществляется игроком по нажатию кнопки (tryRetrieveWeapon)

        ++it;
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
    // 1. Искры попадания (пакетная отрисовка в 1 Draw Call)
    if (!_hitParticles.empty()) {
        sf::VertexArray hitVA(sf::PrimitiveType::Triangles);
        for (const auto& p : _hitParticles) {
            float s = p.size;
            sf::Color pc = p.color;
            pc.a = static_cast<std::uint8_t>(std::clamp(p.alpha, 0.0f, 255.0f));
            float x = p.position.x;
            float y = p.position.y;
            hitVA.append({{x, y}, pc, {0.0f, 0.0f}});
            hitVA.append({{x + s, y}, pc, {0.0f, 0.0f}});
            hitVA.append({{x + s, y + s}, pc, {0.0f, 0.0f}});

            hitVA.append({{x, y}, pc, {0.0f, 0.0f}});
            hitVA.append({{x + s, y + s}, pc, {0.0f, 0.0f}});
            hitVA.append({{x, y + s}, pc, {0.0f, 0.0f}});
        }
        window.draw(hitVA);
    }

    // 2. Хитбокс и динамический росчерк удара
    if (_attackTimer > 0.0f) {
        _cachedHitboxRect.setSize({_attackHitbox.size.x, _attackHitbox.size.y});
        _cachedHitboxRect.setPosition(_attackHitbox.position);
        _cachedHitboxRect.setFillColor(sf::Color(_attackColor.r, _attackColor.g, _attackColor.b, 35));
        _cachedHitboxRect.setOutlineThickness(1.5f);
        _cachedHitboxRect.setOutlineColor(sf::Color(_attackColor.r, _attackColor.g, _attackColor.b, 220));
        window.draw(_cachedHitboxRect);

        sf::VertexArray slash(sf::PrimitiveType::TriangleStrip, 6);
        float startX = (_attackDirection > 0) ? _attackHitbox.position.x : (_attackHitbox.position.x + _attackHitbox.size.x);
        float endX = (_attackDirection > 0) ? (_attackHitbox.position.x + _attackHitbox.size.x) : _attackHitbox.position.x;
        float midY = _attackHitbox.position.y + _attackHitbox.size.y * 0.5f;

        float topY = midY - 18.0f;
        float botY = midY + 18.0f;

        if (_comboDisplayName.find("UPPERCUT") != std::string::npos) {
            topY = midY - 30.0f;
            botY = midY + 10.0f;
        } else if (_comboDisplayName.find("dw1-j-adw1") != std::string::npos || _comboDisplayName.find("SKY HOOK") != std::string::npos || _currentAttackType == "adw1") {
            topY = midY - 10.0f;
            botY = midY + 25.0f;
        } else if (player.hasWeapon() && (_currentAttackType == "dw1" || _comboDisplayName.find("STOMP") != std::string::npos)) {
            topY = midY - 12.0f;
            botY = midY + 22.0f;
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

    // 3. Расширяющиеся ударные волны (Shockwave Rings)
    for (const auto& sw : _empoweredShockwaves) {
        _cachedCircle.setRadius(sw.radius);
        _cachedCircle.setOrigin({sw.radius, sw.radius});
        _cachedCircle.setPosition(sw.position);
        float alphaProg = (sw.lifetime / sw.maxLifetime);
        sf::Color ringCol = sw.color;
        ringCol.a = static_cast<std::uint8_t>(std::clamp(alphaProg * 230.0f, 0.0f, 255.0f));
        _cachedCircle.setFillColor(sf::Color::Transparent);
        _cachedCircle.setOutlineColor(ringCol);
        _cachedCircle.setOutlineThickness(std::max(1.5f, 3.5f * alphaProg));
        window.draw(_cachedCircle);
    }

    // 4. Всплывающий урон
    if (_fontLoaded) {
        if (!_cachedDamageText.has_value()) {
            _cachedDamageText.emplace(_font, "", 17);
        }
        for (const auto& dn : _damageNumbers) {
            _cachedDamageText->setString(toSfString(dn.text));
            _cachedDamageText->setPosition(dn.position);
            sf::Color c = dn.color;
            c.a = static_cast<std::uint8_t>(std::clamp(dn.alpha, 0.0f, 255.0f));
            _cachedDamageText->setFillColor(c);
            window.draw(*_cachedDamageText);
        }

        // 5. Особые парящие комбо-маркеры (Special Combo Markers)
        if (!_cachedSpecialMarkerText.has_value()) {
            _cachedSpecialMarkerText.emplace(_font, "", 16);
            _cachedSpecialMarkerText->setOutlineThickness(1.8f);
        }
        for (const auto& marker : _specialComboMarkers) {
            _cachedSpecialMarkerText->setString(toSfString(marker.text));
            _cachedSpecialMarkerText->setPosition(marker.position);
            sf::Color mc = marker.color;
            mc.a = static_cast<std::uint8_t>(std::clamp(marker.alpha, 0.0f, 255.0f));
            _cachedSpecialMarkerText->setFillColor(mc);
            _cachedSpecialMarkerText->setOutlineColor(sf::Color(0, 0, 0, mc.a));
            _cachedSpecialMarkerText->setScale({marker.scale, marker.scale});
            window.draw(*_cachedSpecialMarkerText);
        }

        // 6. Отрисовка летящих и лежащих на земле булав (Thrown Mace Meat Tenderizers)
        for (const auto& mace : _thrownMaces) {
            if (!mace.active) continue;
            if (!_maceTextureLoaded) {
                std::string macePath = resolveAssetPath("assets/sprites/mace_tenderizer.png");
                _maceTextureLoaded = _maceTexture.loadFromFile(macePath);
            }
            if (_maceTextureLoaded) {
                if (mace.grounded) {
                    // Пульсирующая подсветка на полу для легкого обнаружения оружия
                    _cachedCircle.setRadius(14.0f);
                    _cachedCircle.setOrigin({14.0f, 14.0f});
                    _cachedCircle.setPosition(mace.position);
                    _cachedCircle.setFillColor(sf::Color(255, 210, 80, 55));
                    _cachedCircle.setOutlineThickness(0.0f);
                    window.draw(_cachedCircle);
                }

                if (!_cachedMaceSprite.has_value()) {
                    _cachedMaceSprite.emplace(_maceTexture);
                }
                float texW = static_cast<float>(_maceTexture.getSize().x);
                float texH = static_cast<float>(_maceTexture.getSize().y);
                // Размер булавы в игре: ровно в 1.25 раза больше игрока (высота игрока = 64px, 64 * 1.25 = 80px)
                constexpr float playerHeight = static_cast<float>(FRAME_SIZE) * SPRITE_SCALE; // 64.0f
                constexpr float targetMaceH = playerHeight * 1.25f; // 80.0f
                float scaleY = targetMaceH / (texH > 0.0f ? texH : targetMaceH);
                float scaleX = scaleY; // Сохраняем идеальные 1:1 пропорции текстуры без искажения пикселей

                _cachedMaceSprite->setOrigin({texW * 0.5f, texH * 0.5f});
                _cachedMaceSprite->setScale({scaleX, scaleY});
                _cachedMaceSprite->setPosition(mace.position);
                _cachedMaceSprite->setRotation(sf::degrees(mace.rotation));
                window.draw(*_cachedMaceSprite);
            }
        }
    }

    // 7. Отрисовка динамических спец-эффектов Pixelfx
    VfxSystem::get().draw(window);
}

void CombatSystem::drawUI(sf::RenderWindow& window, const Player* player) {
    if (!_fontLoaded) return;

    if (!_uiTextsInitialized) {
        _hint1Text.emplace(_font, toSfString("[ЛКМ/J]: 1 | [ПКМ/K]: 2 | [R]: ХИЛКА | [Q]: Контр-атака | [Shift]: Рывок | [F]: Блок | [Tab]: ИИ"), 13);
        _hint1Text->setFillColor(sf::Color(220, 220, 235, 210));

        _hint2Text.emplace(_font, toSfString("Комбо: 111 | 222 | 11b2 (бросок назад) | 112 | 1f2d1 | 21db1 | dw1-j-adw1 (топот+крюк) | a1a2 | a1a1adw2"), 13);
        _hint2Text->setFillColor(sf::Color(255, 200, 110, 220));

        _pLabelText.emplace(_font, "", 11);
        _pLabelText->setFillColor(sf::Color(210, 235, 215));

        _comboBannerText.emplace(_font, "", 22);

        _chainText.emplace(_font, "", 15);
        _chainText->setPosition({30.0f, 62.0f});
        _chainText->setFillColor(sf::Color(180, 220, 255, 200));

        _scaleText.emplace(_font, "", 14);
        _scaleText->setPosition({30.0f, 82.0f});
        _scaleText->setFillColor(sf::Color(255, 215, 80));
        _scaleText->setOutlineColor(sf::Color(0, 0, 0, 200));
        _scaleText->setOutlineThickness(1.2f);

        _stunText.emplace(_font, "", 15);
        _stunText->setPosition({30.0f, 88.0f});

        _counterText.emplace(_font, toSfString("[Q] СТОЙКА КОНТР-АТАКИ / ПАРИРОВАНИЯ АКТИВНА!"), 15);
        _counterText->setPosition({30.0f, 88.0f});
        _counterText->setFillColor(sf::Color(0, 230, 255));

        _uiTextsInitialized = true;
    }

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

        int curHpInt = static_cast<int>(player->hp);
        if (curHpInt != _lastCachedHp) {
            _lastCachedHp = curHpInt;
            _pLabelText->setString(toSfString("PLAYER HP: " + std::to_string(curHpInt)));
        }
        _pLabelText->setPosition({pBarX + pBarW + 10.0f, pBarY - 2.0f});
        window.draw(*_pLabelText);

        // Шкала прочности блока (GUARD / POISE METER) игрока:
        float gBarW = pBarW;
        float gBarH = 6.0f;
        float gBarX = pBarX;
        float gBarY = pBarY + pBarH + 4.0f;

        sf::RectangleShape gBorder({gBarW + 2.0f, gBarH + 2.0f});
        gBorder.setPosition({gBarX - 1.0f, gBarY - 1.0f});
        gBorder.setFillColor(sf::Color(15, 20, 25, 220));
        gBorder.setOutlineThickness(1.0f);
        gBorder.setOutlineColor(sf::Color(50, 70, 90, 200));
        window.draw(gBorder);

        sf::RectangleShape gBg({gBarW, gBarH});
        gBg.setPosition({gBarX, gBarY});
        gBg.setFillColor(sf::Color(25, 30, 40));
        window.draw(gBg);

        float curGuardRatio = (player->maxGuardHp > 0.0f) ? std::clamp(player->guardHp / player->maxGuardHp, 0.0f, 1.0f) : 0.0f;
        sf::RectangleShape gBar({gBarW * curGuardRatio, gBarH});
        gBar.setPosition({gBarX, gBarY});
        if (player->isGuardBroken()) {
            gBar.setFillColor(sf::Color(255, 60, 60)); // Красный при пробитии
        } else if (curGuardRatio < 0.35f) {
            gBar.setFillColor(sf::Color(255, 170, 40)); // Оранжевый при низком запасе
        } else {
            gBar.setFillColor(sf::Color(60, 185, 245)); // Стальной лазурный
        }
        window.draw(gBar);

        std::string guardStr = player->isGuardBroken() ? "GUARD BROKEN!" : ("GUARD: " + std::to_string(static_cast<int>(player->guardHp)) + "%");
        sf::Text guardLabel(_font, toSfString(guardStr), 10);
        guardLabel.setPosition({gBarX + gBarW + 10.0f, gBarY - 3.0f});
        guardLabel.setFillColor(player->isGuardBroken() ? sf::Color(255, 80, 80) : sf::Color(140, 210, 255));
        window.draw(guardLabel);

        // Отображение запаса хилок [R]
        int flasks = player->getHealFlasks();
        int maxFlasks = player->getMaxHealFlasks();
        std::string healStr = "[R] ХИЛКИ: " + std::to_string(flasks) + " / " + std::to_string(maxFlasks);
        sf::Text healText(_font, toSfString(healStr), 11);
        healText.setPosition({pBarX, gBarY + gBarH + 5.0f});
        healText.setFillColor(flasks > 0 ? sf::Color(80, 255, 130) : sf::Color(160, 160, 160, 180));
        window.draw(healText);

        for (int i = 0; i < maxFlasks; ++i) {
            sf::RectangleShape flaskIcon({7.0f, 9.0f});
            flaskIcon.setPosition({pBarX + 130.0f + i * 11.0f, gBarY + gBarH + 6.0f});
            if (i < flasks) {
                flaskIcon.setFillColor(sf::Color(80, 255, 120));
                flaskIcon.setOutlineColor(sf::Color(40, 180, 70));
            } else {
                flaskIcon.setFillColor(sf::Color(50, 50, 60, 180));
                flaskIcon.setOutlineColor(sf::Color(80, 80, 90, 180));
            }
            flaskIcon.setOutlineThickness(1.0f);
            window.draw(flaskIcon);
        }
    }

    // Отображение текущего комбо баннера
    if (_comboDisplayTimer > 0.0f && !_comboDisplayName.empty()) {
        if (_comboDisplayName != _lastCachedComboName) {
            _lastCachedComboName = _comboDisplayName;
            _comboBannerText->setString(toSfString(_comboDisplayName));
        }
        _comboBannerText->setPosition({30.0f, 32.0f});
        _comboBannerText->setFillColor(_comboDisplayColor);
        window.draw(*_comboBannerText);
    }

    // Отображение цепочки ввода (например: "INPUT CHAIN: [1] -> [f2] -> [d1]")
    if (!_comboHistory.empty() && _comboResetTimer > 0.0f) {
        std::string chainStr = "INPUT CHAIN: ";
        for (size_t i = 0; i < _comboHistory.size(); ++i) {
            chainStr += "[" + _comboHistory[i] + "]";
            if (i + 1 < _comboHistory.size()) chainStr += " -> ";
        }
        if (chainStr != _lastCachedChainStr) {
            _lastCachedChainStr = chainStr;
            _chainText->setString(toSfString(chainStr));
        }
        window.draw(*_chainText);
    }

    // Отображение скейлинга урона и снижения стана при серии ударов
    if (_comboHitCount > 1 && _comboResetTimer > 0.0f) {
        int dmgPct = static_cast<int>(std::round(_currentDamageScaling * 100.0f));
        int stunPct = static_cast<int>(std::round(_currentHitstunDecay * 100.0f));
        std::string scaleStr = "COMBO HITS: " + std::to_string(_comboHitCount) +
                               " | PRORATION: " + std::to_string(dmgPct) + "%" +
                               " | STUN: " + std::to_string(stunPct) + "%";
        if (scaleStr != _lastCachedScaleStr) {
            _lastCachedScaleStr = scaleStr;
            _scaleText->setString(toSfString(scaleStr));
        }
        window.draw(*_scaleText);
    }

    // Предупреждение об изнеможении / стан-локе игрока
    if (player && player->isStunned()) {
        std::string sVal = std::to_string(player->getStunTimer());
        if (sVal.size() > 4) sVal = sVal.substr(0, 4);
        if (player->isCounterWhiffStun() || _lastStunReason == StunReason::COUNTER_WHIFF) {
            _stunText->setString(toSfString("! СТАН-ЛОК ИГРОКА (ПРОМАХ СТОЙКИ КОНТР-АТАКИ [Q]): " + sVal + "с !"));
            _stunText->setFillColor(sf::Color(255, 120, 80));
        } else if (_lastStunReason == StunReason::WHIFF) {
            _stunText->setString(toSfString("! СТАН-ЛОК ИГРОКА (ПРОМАХ ПО ВРАГУ / WHIFF LAG): " + sVal + "с !"));
            _stunText->setFillColor(sf::Color(255, 110, 110));
        } else if (_lastStunReason == StunReason::SPAM) {
            _stunText->setString(toSfString("! СТАН-ЛОК ИГРОКА (ШТРАФ ЗА СПАМ ОДНОГО КОМБО В ТЕЧЕНИЕ 3с): " + sVal + "с !"));
            _stunText->setFillColor(sf::Color(255, 75, 75));
        } else {
            _stunText->setString(toSfString("! СТАН-ЛОК ИГРОКА: " + sVal + "с !"));
            _stunText->setFillColor(sf::Color(255, 80, 80));
        }
        window.draw(*_stunText);
    } else if (player && player->isCounterStanceActive()) {
        window.draw(*_counterText);
    }

    // Нижняя панель подсказок управления (статические кешированные тексты)
    float winH = static_cast<float>(window.getSize().y);
    _hint1Text->setPosition({30.0f, winH - 46.0f});
    window.draw(*_hint1Text);

    _hint2Text->setPosition({30.0f, winH - 26.0f});
    window.draw(*_hint2Text);
}

void CombatSystem::drawBossHealthBar(sf::RenderWindow& window, const Enemy* boss) const {
    if (!boss || boss->isDead || boss->hp <= 0.0f) return;

    sf::Vector2u winSize = window.getSize();
    float barW = 560.0f;
    float barH = 22.0f;
    float barX = (winSize.x - barW) * 0.5f;
    float barY = 28.0f;

    // Внешняя рамка
    sf::RectangleShape border({barW + 6.0f, barH + 6.0f});
    border.setPosition({barX - 3.0f, barY - 3.0f});
    border.setFillColor(sf::Color(15, 15, 20, 230));
    border.setOutlineThickness(2.0f);
    border.setOutlineColor(sf::Color(190, 165, 90)); // Золотисто-благородная окантовка
    window.draw(border);

    // Темная подложка
    sf::RectangleShape bg({barW, barH});
    bg.setPosition({barX, barY});
    bg.setFillColor(sf::Color(45, 10, 10, 240));
    window.draw(bg);

    // Отстающая полоска плавного урона (Catch-up bar)
    if (boss->displayedHp > boss->hp && boss->maxHp > 0.0f) {
        float lagW = std::clamp(barW * (boss->displayedHp / boss->maxHp), 0.0f, barW);
        sf::RectangleShape lagBar({lagW, barH});
        lagBar.setPosition({barX, barY});
        lagBar.setFillColor(sf::Color(240, 150, 40));
        window.draw(lagBar);
    }

    // Текущее ХП (ярко-красная полоса)
    if (boss->hp > 0.0f && boss->maxHp > 0.0f) {
        float curW = std::clamp(barW * (boss->hp / boss->maxHp), 0.0f, barW);
        sf::RectangleShape curBar({curW, barH});
        curBar.setPosition({barX, barY});
        curBar.setFillColor(sf::Color(230, 30, 30));
        window.draw(curBar);
    }

    // Текстовая плашка с именем босса "БОСС 1"
    if (_fontLoaded) {
        if (!_bossNameText.has_value()) {
            _bossNameText.emplace(_font, toSfString("БОСС 1"), 15);
            _bossNameText->setStyle(sf::Text::Bold);
        }
        _bossNameText->setFillColor(sf::Color(255, 220, 130));
        float nameW = _bossNameText->getLocalBounds().size.x;
        _bossNameText->setPosition({(winSize.x - nameW) * 0.5f, barY - 20.0f});
        window.draw(*_bossNameText);

        // Цифры ХП
        if (!_bossHpText.has_value()) {
            _bossHpText.emplace(_font, "", 11);
            _bossHpText->setStyle(sf::Text::Bold);
        }
        std::string hpStr = std::to_string(static_cast<int>(std::round(boss->hp))) + " / " +
                            std::to_string(static_cast<int>(std::round(boss->maxHp)));
        _bossHpText->setString(toSfString(hpStr));
        _bossHpText->setFillColor(sf::Color(255, 255, 255, 220));
        float hpW = _bossHpText->getLocalBounds().size.x;
        _bossHpText->setPosition({(winSize.x - hpW) * 0.5f, barY + 3.0f});
        window.draw(*_bossHpText);
    }
}
