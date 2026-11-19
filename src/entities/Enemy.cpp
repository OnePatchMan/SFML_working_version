#include "entities/Enemy.h"
#include "world/Arena.h"
#include "audio/SoundManager.h"
#include "combat/CombatSystem.h"
#include <iostream>

Enemy::Enemy(sf::Vector2f spawnPos, EnemyType type)
    : position(spawnPos), spawnPosition(spawnPos), patrolStartX(spawnPos.x), enemyType(type)
{
    setType(type);
}

const EnemyArchetypeData& Enemy::getEConfig() const {
    return EnemyConfig::getArchetype(static_cast<int>(enemyType));
}

void Enemy::setType(EnemyType type) {
    enemyType = type;
    const auto& ecfg = getEConfig();
    if (enemyType == EnemyType::NORMAL) {
        maxHp = ecfg.hp;
        scale = ecfg.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = ecfg.damageMult;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = ecfg.canCounter;
        canBlock = ecfg.canBlock;
        canDodge = ecfg.canDodge;
        aggressiveness = 0.5f;
        intelligence = 0.5f;
        maxComboHits = 1; // Обычный враг дает 1 комбо
        allowAirFollowup = false;
    } else if (enemyType == EnemyType::TANK) {
        maxHp = ecfg.hp;
        scale = ecfg.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = ecfg.stunMult;
        knockMultiplier = ecfg.knockMult;
        damageMultiplier = ecfg.damageMult;
        hitboxScale = ecfg.hitboxScale;
        windupMultiplier = ecfg.windupMult;
        canCounter = ecfg.canCounter;
        canBlock = ecfg.canBlock;
        canDodge = false;
        aggressiveness = 0.55f;
        intelligence = 0.5f;
        maxComboHits = 2;
        allowAirFollowup = false;
    } else if (enemyType == EnemyType::DUELIST) {
        maxHp = ecfg.hp;
        scale = ecfg.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = ecfg.damageMult;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = ecfg.canCounter;
        canBlock = ecfg.canBlock;
        canDodge = ecfg.canDodge;
        aggressiveness = ecfg.aggressiveness;
        intelligence = ecfg.intelligence;
        maxComboHits = 3;
        allowAirFollowup = true;
    } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
        maxHp = ecfg.hp;
        scale = ecfg.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = ecfg.damageMult;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = ecfg.canCounter;
        canBlock = ecfg.canBlock;
        canDodge = ecfg.canDodge;
        aggressiveness = ecfg.aggressiveness;
        intelligence = ecfg.intelligence;
        maxComboHits = 3;
        allowAirFollowup = true;
        invisActiveTimer = 0.0f;
        invisCooldownTimer = ecfg.invisCooldown;
    } else if (enemyType == EnemyType::DUMMY) {
        maxHp = 9999.0f;
        scale = 1.0f;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = 0.0f;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = false;
        canBlock = false;
        canDodge = false;
        aggressiveness = 0.0f;
        intelligence = 0.0f;
        maxComboHits = 0;
        allowAirFollowup = false;
        isSmartAI = false;
        dummyAccumDamage = 0.0f;
        dummyResetTimer = 0.0f;
        dummyOutOfStunTimer = 0.0f;
        dummyLastGapFrames = 0;
        dummyHadFirstHit = false;
        dummyPopups.clear();
    }
    hp = maxHp;
    displayedHp = maxHp;
}

void Enemy::triggerDodge(float dir) {
    if (isDead || hurtTimer > 0.0f || isDodging) return;
    const auto& ecfg = getEConfig();
    isDodging = true;
    dodgeTimer = 0.24f;
    dodgeCooldownTimer = ecfg.dodgeCooldown;
    dodgeDirection = dir;
    if (std::abs(dir) > 0.01f) {
        velocity.x = dir * 450.0f;
    } else {
        velocity.x = 0.0f; // Уворот на месте (Spot Dodge)
    }
    if (attackState == EnemyAttackState::WINDUP) {
        attackState = EnemyAttackState::IDLE;
    }
}

sf::FloatRect Enemy::getBounds() const {
    return sf::FloatRect({position.x - halfW, position.y - halfH}, 
                         {halfW * 2.0f, halfH * 2.0f});
}

bool Enemy::takeDamage(float amount, float knockX, float knockY, float stunTime,
                       HitLevel hitLevel, bool isBackHit, const sf::Vector2f& attackerPos) {
    if (isDead) return false;

    // ОСОБАЯ ЛОГИКА ДЛЯ ТРЕНИРОВОЧНОГО МАНЕКЕНА (DUMMY):
    // Полная чувствительность к физике / силе отталкивания, бессмертие, трекинг комбо-урона и фрейм-гэпа
    if (enemyType == EnemyType::DUMMY) {
        if (hurtTimer <= 0.0f) {
            hasWallBounced = false;
        }

        // Расчет гэпа между ударами в кадрах (при 60 FPS):
        // Сколько кадров манекен находился вне стана между двумя атаками
        if (dummyHadFirstHit && dummyResetTimer > 0.0f) {
            dummyLastGapFrames = static_cast<int>(std::round(dummyOutOfStunTimer * 60.0f));
        } else {
            dummyLastGapFrames = 0;
            dummyAccumDamage = 0.0f;
            dummyHadFirstHit = true;
        }

        dummyAccumDamage += amount;
        dummyResetTimer = 5.0f; // сбрасывается каждые 5.0 секунд после прекращения ударов
        dummyOutOfStunTimer = 0.0f;

        // Всплывающее уведомление над головой (вылетает и плавно исчезает)
        DummyFloatingPopup popup;
        popup.timer = 1.3f;
        popup.maxTimer = 1.3f;
        popup.offset = {0.0f, -45.0f};
        if (dummyLastGapFrames == 0) {
            popup.text = "+" + std::to_string(static_cast<int>(std::round(amount))) + " (0f TRUE COMBO!)";
            popup.color = sf::Color(100, 255, 120);
        } else {
            popup.text = "+" + std::to_string(static_cast<int>(std::round(amount))) + " (GAP: " + std::to_string(dummyLastGapFrames) + "f)";
            popup.color = sf::Color(255, 180, 50);
        }
        dummyPopups.push_back(popup);

        hurtTimer = stunTime * stunMultiplier;
        velocity.x = knockX * knockMultiplier;
        velocity.y = knockY * knockMultiplier;
        isDead = false;
        isAirSuspended = false;
        attackState = EnemyAttackState::IDLE;
        aiComboState = AIComboState::IDLE;
        behaviorState = AIBehaviorState::CHASE;
        hasDetectedPlayer = true;
        hp = 9999.0f;
        displayedHp = 9999.0f;
        return true;
    }

    // Проверка уворота / доджа дуэлянта (i-frames):
    if (isDodging) {
        // Проверка идеального доджа (в начале окна уворота):
        if (dodgeTimer > 0.10f) {
            isPerfectDodged = true;
            perfectDodgeEffectTimer = 0.45f;
            if (canCounter && counterCooldownTimer <= 0.0f) {
                triggerCounterStance();
            }
        }
        return false; // Урон избегается благодаря i-frames!
    }

    // СИСТЕМА ЗАЩИТЫ / БЛОКА БОТА:
    // Снижает входящий урон на 80% (бот получает только 20% урона)
    // Входящий блок-стан от каждого удара
    // Проверка попадания в спину
    bool defenderFacingLeft = isFacingLeft;
    bool hitFromBehind = isBackHit;
    if (attackerPos != sf::Vector2f(0.0f, 0.0f)) {
        if (defenderFacingLeft && attackerPos.x > position.x + 5.0f) {
            hitFromBehind = true;
        } else if (!defenderFacingLeft && attackerPos.x < position.x - 5.0f) {
            hitFromBehind = true;
        }
    }

    bool canBlockNow = (isBlocking || blockType != BlockType::NONE) && (hurtTimer <= 0.0f || blockStunTimer > 0.0f);
    bool blockDefended = false;
    BlockType curBlock = getBlockType();

    if (canBlockNow && !hitFromBehind) {
        if (hitLevel == HitLevel::MID) {
            blockDefended = true; // Нейтральные атаки блокируются и верхним, и нижним блоком
        } else if (hitLevel == HitLevel::OVERHEAD) {
            // Оверхэды блокируются только верхним блоком! Пробивают нижний блок
            if (curBlock == BlockType::HIGH) {
                blockDefended = true;
            }
        } else if (hitLevel == HitLevel::LOW) {
            // Нижние удары блокируются только нижним блоком! Пробивают верхний блок
            if (curBlock == BlockType::LOW) {
                blockDefended = true;
            }
        }
    }

    if (blockDefended) {
        amount *= 0.20f;
        hp -= amount;
        velocity.x = knockX * 0.25f * knockMultiplier;
        velocity.y = 0.0f;
        blockStunTimer = 0.14f * stunMultiplier;
        hurtTimer = 0.14f * stunMultiplier;
        if (hp <= 0.0f) {
            hp = 0.0f;
            isDead = true;
            deathFadeTimer = 0.6f;
            respawnTimer = 0.0f;
        }
        return true;
    }

    if (hurtTimer <= 0.0f) {
        hasWallBounced = false;
    }

    hp -= amount;
    hurtTimer = stunTime * stunMultiplier;
    velocity.x = knockX * knockMultiplier;
    velocity.y = knockY * knockMultiplier;
    isBlocking = false;
    blockType = BlockType::NONE; // Чистое попадание гарантированно сбрасывает блок
    blockHoldTimer = 0.0f;
    blockStunTimer = 0.0f;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    // БЫСТРЫЙ РЕАКТИВНЫЙ КУЛДАУН (БОТЫ НЕ ЗАСТЫВАЮТ И НЕ СТОЯТ КАК ДЕРЕВЯННЫЕ СТОЛБЫ)
    attackCooldown = (enemyType == EnemyType::DUELIST) ? 0.20f : ((enemyType == EnemyType::TANK) ? 0.40f : 0.30f);
    aiComboState = AIComboState::IDLE;
    currentComboName = "";
    isAirSuspended = false;
    counterStanceTimer = 0.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});

    // При получении урона враг мгновенно фиксирует цель и переходит в погоню
    behaviorState = AIBehaviorState::CHASE;
    hasDetectedPlayer = true;
    lostSightTimer = EnemyConfig::get().perception.lostSightTimeout;

    if (hp <= 0.0f) {
        hp = 0.0f;
        isDead = true;
        deathFadeTimer = 0.6f; // Короткое исчезновение побежденного врага
        respawnTimer = 0.0f;   // Автоматическое воскрешение отключено
    }
    return true;
}

void Enemy::reset() {
    setType(enemyType);
    position = spawnPosition;
    patrolStartX = spawnPosition.x;
    velocity = {0.0f, 0.0f};
    hp = maxHp;
    displayedHp = maxHp;
    isDead = false;
    hurtTimer = 0.0f;
    spinTimer = 0.0f;
    spinAngle = 0.0f;
    isDodging = false;
    dodgeTimer = 0.0f;
    dodgeCooldownTimer = 0.0f;
    isPerfectDodged = false;
    perfectDodgeEffectTimer = 0.0f;
    invisActiveTimer = 0.0f;
    invisCooldownTimer = 7.0f;
    stunPunishCooldown = 0.0f;
    punishBannerTimer = 0.0f;
    grappleBranchChoice = 0;
    playerIdleTimer = 0.0f;
    spacingTimer = 0.0f;
    blockHoldTimer = 0.0f;
    blockType = BlockType::NONE;
    diveSlamBounce = false;
    hasWallBounced = false;
    diveSlamDir = 1.0f;
    respawnTimer = 0.0f;
    deathFadeTimer = 0.0f;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackCooldown = 0.3f;
    attackHasHit = false;
    aiComboState = AIComboState::IDLE;
    aiStateTimer = 0.0f;
    currentComboName = "";
    isAirSuspended = false;
    counterStanceTimer = 0.0f;
    counterCooldownTimer = 2.5f;
    currentStarterType = AIStarterType::STARTER_1F2D1;
    nextStarterChoice = 0;
    currentHbw = 55.0f * hitboxScale;
    currentHbh = 42.0f * hitboxScale;
    currentHbyOffset = -18.0f * scale;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});

    // Сброс системы патрулирования и обнаружения
    behaviorState = AIBehaviorState::PATROL;
    alertTimer = 0.0f;
    lostSightTimer = 0.0f;
    patrolWaitTimer = 0.0f;
    jumpCooldown = 0.0f;
    isPatrolMovingRight = false;
    hasDetectedPlayer = false;
    alertEffectTimer = 0.0f;
    lastKnownPlayerPos = {0.0f, 0.0f};
}

void Enemy::forceDetectPlayer(sf::Vector2f pos) {
    hasDetectedPlayer = true;
    lastKnownPlayerPos = pos;
    behaviorState = AIBehaviorState::CHASE;
    lostSightTimer = EnemyConfig::get().perception.lostSightTimeout;
    isFacingLeft = (pos.x < position.x);
}

bool Enemy::jump(float speedY) {
    if (jumpCooldown > 0.0f) return false;
    velocity.y = -std::abs(speedY);
    jumpCooldown = EnemyConfig::get().perception.jumpCooldown;
    return true;
}

void Enemy::applyAttackStats(const EnemyAttackStats& s, float dirX) {
    currentAttackDamage = s.damage * damageMultiplier;
    currentAttackKnockX = dirX * s.knockX;
    currentAttackKnockY = s.knockY;
    currentAttackStun = s.stun;
    currentAttackHitLevel = HitLevel::MID;
    currentAttackIsBackHit = false;
    currentAttackIsDiveSlam = s.isDiveSlam;
    currentHbw = s.hbw * hitboxScale;
    currentHbh = s.hbh * hitboxScale;
    currentHbyOffset = s.hbyOffset * scale;
}

void Enemy::triggerAttack() {
    if (isDead || hurtTimer > 0.0f || attackState != EnemyAttackState::IDLE) return;
    const auto& ecfg = getEConfig();
    behaviorState = AIBehaviorState::COMBAT;
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = ecfg.stats.dummyWindup * windupMultiplier; // Замах перед ударом
    attackHasHit = false;
    currentAttackDamage = ecfg.stats.dummyDamage * damageMultiplier;
    currentAttackKnockX = isFacingLeft ? -ecfg.stats.dummyKnockX * knockMultiplier : ecfg.stats.dummyKnockX * knockMultiplier;
    currentAttackKnockY = ecfg.stats.dummyKnockY;
    currentAttackStun = ecfg.stats.dummyStun;
    currentAttackHitLevel = HitLevel::MID;
    currentAttackIsBackHit = false;
    currentAttackIsDiveSlam = false;
    currentHbw = ecfg.stats.dummyHbw * hitboxScale;
    currentHbh = ecfg.stats.dummyHbh * hitboxScale;
    currentHbyOffset = ecfg.stats.dummyHbyOffset * scale;
}

void Enemy::triggerCounterStance() {
    if (!canCounter || isDead || hurtTimer > 0.0f || counterStanceTimer > 0.0f || counterCooldownTimer > 0.0f) return;
    const auto& ecfg = getEConfig();
    counterStanceTimer = ecfg.counter.window;
    counterCooldownTimer = ecfg.counter.cooldown;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    velocity.x = 0.0f;
    currentComboName = "BOT: PARRY STANCE!";
}

void Enemy::endCounterSuccess() {
    counterStanceTimer = 0.0f;
    currentComboName = "! BOT COUNTER-ATTACK !";
}

void Enemy::triggerComboSequence(int forcedStarter, int forcedHitLimit) {
    if (isDead || hurtTimer > 0.0f || aiComboState != AIComboState::IDLE) return;

    if (forcedStarter >= 0) {
        currentStarterType = static_cast<AIStarterType>(forcedStarter % 4);
        if (forcedHitLimit > 0) {
            maxComboHits = forcedHitLimit;
            allowAirFollowup = (forcedHitLimit >= 3);
        } else {
            maxComboHits = 3;
            allowAirFollowup = true;
        }
        if (currentStarterType == AIStarterType::STARTER_GRAPPLE_PULL) {
            grappleBranchChoice = (nextStarterChoice % 2);
        }
    } else {
        if (enemyType == EnemyType::DUELIST) {
            // Агрессивный дуэлянт чередует 4 стартера, но длинный зацеп Grapple Pull разрешен ТОЛЬКО с земли!
            bool isGrounded = Arena::getActive() ? Arena::getActive()->overlapsSolid(position.x, position.y + 3.0f, halfW, halfH, halfH) : true;
            int starterPool = isGrounded ? 4 : 3;
            currentStarterType = static_cast<AIStarterType>(nextStarterChoice % starterPool);
            nextStarterChoice = (nextStarterChoice + 1) % 4;
            grappleBranchChoice = (nextStarterChoice % 2);
            maxComboHits = 3;
            allowAirFollowup = true;
        } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
            currentStarterType = static_cast<AIStarterType>(nextStarterChoice % 3);
            nextStarterChoice = (nextStarterChoice + 1) % 3;
            maxComboHits = 3;
            allowAirFollowup = true;
        } else {
            currentStarterType = static_cast<AIStarterType>(nextStarterChoice % 3);
            nextStarterChoice = (nextStarterChoice + 1) % 3;

            if (forcedHitLimit > 0) {
                maxComboHits = forcedHitLimit;
                allowAirFollowup = (forcedHitLimit >= 3);
            } else {
                if (enemyType == EnemyType::NORMAL) {
                    maxComboHits = 1;
                    allowAirFollowup = false;
                } else if (enemyType == EnemyType::TANK) {
                    maxComboHits = 2;
                    allowAirFollowup = false;
                }
            }
        }
    }

    aiComboState = AIComboState::APPROACH;
    behaviorState = AIBehaviorState::COMBAT;
    aiStateTimer = 0.0f;
    attackCooldown = 0.0f;
    attackHasHit = false;

    switch (currentStarterType) {
    case AIStarterType::STARTER_1F2D1:
        currentComboName = (maxComboHits == 1) ? "AI: 1 (SINGLE POKE)" : 
                           (maxComboHits == 2) ? "AI: 1-f2 (2-HIT LINK)" : 
                                                 "AI: 1-f2-d1 (DIAGONAL LAUNCHER)";
        break;
    case AIStarterType::STARTER_112:
        currentComboName = (maxComboHits == 1) ? "AI: 1 (QUICK JAB)" : 
                           (maxComboHits == 2) ? "AI: 1-1 (DOUBLE JAB)" : 
                                                 "AI: 1-1-2 (RAPID COMBO LAUNCHER)";
        break;
    case AIStarterType::STARTER_21DB1:
        currentComboName = (maxComboHits == 1) ? "AI: 2 (HEAVY POKE)" : 
                           (maxComboHits == 2) ? "AI: 2-1 (HEAVY STRING)" : 
                                                 "AI: 2-1-db1 (REVERSE SWEEP LAUNCHER)";
        break;
    case AIStarterType::STARTER_GRAPPLE_PULL:
        currentComboName = "AI: GRAPPLE PULL [LONG REACH]";
        break;
    }
}

void Enemy::dropCombo() {
    const auto& ecfg = getEConfig();
    aiComboState = AIComboState::IDLE;
    attackState = EnemyAttackState::RECOVERY;
    attackStateTimer = ecfg.stats.whiffRecovery; // Стан-лок/восстановление при промахе
    attackCooldown = (enemyType == EnemyType::DUELIST) ? 0.20f : 0.32f; // Быстрое возвращение в бой
    currentComboName = "";
    isAirSuspended = false;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
}

void Enemy::finishCombo(bool hitConfirmed) {
    const auto& ecfg = getEConfig();
    aiComboState = AIComboState::IDLE;
    attackState = EnemyAttackState::RECOVERY;
    attackStateTimer = hitConfirmed ? ecfg.stats.hitRecovery : ecfg.stats.whiffRecovery;
    if (enemyType == EnemyType::DUELIST) {
        attackCooldown = hitConfirmed ? 0.18f : 0.28f; // Бешеный напор агрессивного дуэлянта без передышки
    } else if (enemyType == EnemyType::TANK) {
        attackCooldown = hitConfirmed ? 0.30f : 0.42f;
    } else {
        attackCooldown = hitConfirmed ? 0.24f : 0.32f;
    }
    currentComboName = "";
    isAirSuspended = false;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
}

void Enemy::stagger(float duration) {
    hurtTimer = duration;
    dropCombo();
}

void Enemy::update(float dt, const Arena& arena, const sf::Vector2f& playerPos, bool playerIsStunned, bool playerIsAttacking) {
    if (isDead) {
        if (deathFadeTimer > 0.0f) {
            deathFadeTimer -= dt;
            if (deathFadeTimer < 0.0f) deathFadeTimer = 0.0f;
        }
        return; // Мертвые враги не воскресают автоматически
    }

    // Плавное догоняние полоски урона
    displayedHp += (hp - displayedHp) * std::min(1.0f, dt * 10.0f);

    if (counterCooldownTimer > 0.0f) counterCooldownTimer -= dt;
    if (dodgeCooldownTimer > 0.0f) dodgeCooldownTimer -= dt;
    if (spacingTimer > 0.0f) spacingTimer -= dt;
    if (perfectDodgeEffectTimer > 0.0f) {
        perfectDodgeEffectTimer -= dt;
        if (perfectDodgeEffectTimer <= 0.0f) isPerfectDodged = false;
    }
    if (invisActiveTimer > 0.0f) invisActiveTimer -= dt;
    if (invisCooldownTimer > 0.0f) invisCooldownTimer -= dt;
    if (stunPunishCooldown > 0.0f) stunPunishCooldown -= dt;
    if (punishBannerTimer > 0.0f) punishBannerTimer -= dt;

    // Логика тренировочного манекена (Dummy): сброс урона через 5 секунд и вылетающие надписи
    if (enemyType == EnemyType::DUMMY) {
        if (dummyResetTimer > 0.0f) {
            dummyResetTimer -= dt;
            if (dummyResetTimer <= 0.0f) {
                dummyAccumDamage = 0.0f;
                dummyHadFirstHit = false;
                dummyLastGapFrames = 0;
            }
        }
        if (hurtTimer <= 0.0f) {
            dummyOutOfStunTimer += dt;
        }
        for (auto it = dummyPopups.begin(); it != dummyPopups.end();) {
            it->timer -= dt;
            it->offset.y -= 45.0f * dt; // плавно вылетает вверх над головой
            if (it->timer <= 0.0f) {
                it = dummyPopups.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Таймер бездействия игрока (для агрессивного дуэлянта)
    if (enemyType == EnemyType::DUELIST && !isDead && hurtTimer <= 0.0f) {
        if (playerIsAttacking) {
            playerIdleTimer = 0.0f;
        } else if (!playerIsStunned) {
            float distToPlayer = std::abs(playerPos.x - position.x);
            float vertDist = std::abs(playerPos.y - position.y);
            if (hasDetectedPlayer && distToPlayer < 240.0f && vertDist < 80.0f) {
                playerIdleTimer += dt;
            } else {
                playerIdleTimer = 0.0f;
            }
        } else {
            playerIdleTimer = 0.0f;
        }
    }

    // Тактическая невидимость пассивного дуэлянта (на 1с раз в 7с)
    if (enemyType == EnemyType::DUELIST_PASSIVE && !isDead && hurtTimer <= 0.0f) {
        if (invisCooldownTimer <= 0.0f && behaviorState != AIBehaviorState::PATROL) {
            invisActiveTimer = 1.0f;
            invisCooldownTimer = 7.0f;
            SoundManager::get().play("1", 35.0f);
        }
    }

    // Реакция на стан игрока (Паниш станов дуэлянтами, шанс зависит от интеллекта)
    if ((enemyType == EnemyType::DUELIST || enemyType == EnemyType::DUELIST_PASSIVE) && !isDead && hurtTimer <= 0.0f) {
        if (playerIsStunned && stunPunishCooldown <= 0.0f && aiComboState == AIComboState::IDLE) {
            const auto& ecfg = getEConfig();
            float absX = std::abs(playerPos.x - position.x);
            float absY = std::abs(playerPos.y - position.y);
            float distSq = (playerPos.x - position.x) * (playerPos.x - position.x) + (playerPos.y - position.y) * (playerPos.y - position.y);
            bool inFacingDir = isFacingLeft ? (playerPos.x < position.x) : (playerPos.x > position.x);
            bool canNotice = ((inFacingDir && absX <= ecfg.perception.viewDistance && absY <= ecfg.perception.verticalViewSpan) ||
                              (distSq <= ecfg.perception.rearViewDistance * ecfg.perception.rearViewDistance && absY <= ecfg.perception.verticalViewSpan * 0.8f) ||
                              (behaviorState != AIBehaviorState::PATROL && absX <= ecfg.perception.viewDistance * 1.2f && absY <= ecfg.perception.verticalViewSpan * 1.2f)) &&
                             arena.hasLineOfSight(position, playerPos);

            if (canNotice) {
                float roll = static_cast<float>(rand() % 1000) / 1000.0f;
                if (roll <= intelligence) {
                    // Бот видит стан игрока и мгновенно срывается в сокрушительный паниш!
                    blockType = BlockType::NONE;
                    blockHoldTimer = 0.0f;
                    attackCooldown = 0.0f;
                    stunPunishCooldown = 1.6f;
                    punishBannerTimer = 1.2f;
                    behaviorState = AIBehaviorState::COMBAT;
                    triggerComboSequence();
                } else {
                    stunPunishCooldown = 0.8f;
                }
            }
        }
    }
    if (spinTimer > 0.0f) {
        spinTimer -= dt;
        spinAngle += spinSpeed * dt;
        if (spinTimer <= 0.0f) {
            spinAngle = 0.0f;
        }
    } else {
        spinAngle = 0.0f;
    }
    if (isDodging) {
        dodgeTimer -= dt;
        if (dodgeTimer <= 0.0f) {
            isDodging = false;
            dodgeTimer = 0.0f;
        }
    }
    if ((isBlocking || blockType != BlockType::NONE) && blockHoldTimer > 0.0f) {
        blockHoldTimer -= dt;
        if (blockHoldTimer <= 0.0f) {
            isBlocking = false;
            blockType = BlockType::NONE;
            blockHoldTimer = 0.0f;
        }
    }
    if (hurtTimer > 0.0f && blockStunTimer <= 0.0f) {
        // Если бот находится в реальном хитстане (получает удары комбо), блок не может быть активен
        isBlocking = false;
        blockType = BlockType::NONE;
        blockHoldTimer = 0.0f;
    }
    if (blockStunTimer > 0.0f) {
        blockStunTimer -= dt;
        if (blockStunTimer <= 0.0f) blockStunTimer = 0.0f;
    }

    if (counterStanceTimer > 0.0f) {
        counterStanceTimer -= dt;
        velocity.x *= 0.1f; // Стоит на месте в стойке парирования
        if (counterStanceTimer <= 0.0f) {
            // ПРОМАХ СТОЙКИ ПАРИРОВАНИЯ БОТА (WHIFF)!
            // Игрок не ударил бота во время стойки — бот получает ДОЛГИЙ СТАН!
            counterStanceTimer = 0.0f;
            const auto& ecfg = getEConfig();
            hurtTimer = ecfg.counter.whiffStun; // Долгий стан бота (1.2с)!
            attackState = EnemyAttackState::IDLE;
            attackStateTimer = 0.0f;
            attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
            currentComboName = "BOT COUNTER WHIFFED! [STUNNED]";
        }
    }

    if (hurtTimer > 0.0f) {
        hurtTimer -= dt;
        attackState = EnemyAttackState::IDLE;
        attackStateTimer = 0.0f;
        if (hurtTimer > 0.3f) {
            aiComboState = AIComboState::IDLE;
            if (currentComboName.find("WHIFFED") == std::string::npos) {
                currentComboName = "";
            }
        }
    } else if (enemyType == EnemyType::DUMMY) {
        // Тренировочный манекен (Dummy) всегда стоит на месте, не патрулирует и не атакует
        attackState = EnemyAttackState::IDLE;
        attackStateTimer = 0.0f;
        aiComboState = AIComboState::IDLE;
    } else {
        // =========================================================
        // СИСТЕМА ВОСПРИЯТИЯ, ОБНАРУЖЕНИЯ И ПАТРУЛИРОВАНИЯ
        // =========================================================
        if (jumpCooldown > 0.0f) jumpCooldown -= dt;
        if (alertEffectTimer > 0.0f) alertEffectTimer -= dt;

        const auto& pcfg = EnemyConfig::get().perception;
        const auto& ecfg = getEConfig();

        bool hasPlayer = (playerPos.x != 0.0f || playerPos.y != 0.0f);
        bool canSeePlayer = false;

        if (hasPlayer && !isDead) {
            float diffX = playerPos.x - position.x;
            float diffY = playerPos.y - position.y;
            float absDistX = std::abs(diffX);
            float absDistY = std::abs(diffY);
            float totalDist = std::hypot(diffX, diffY);

            bool inFacingDir = isFacingLeft ? (diffX < 0.0f) : (diffX > 0.0f);
            bool inSightCone = inFacingDir && (absDistX <= pcfg.viewDistance) && (absDistY <= pcfg.verticalViewSpan);
            bool inCloseAwareness = (totalDist <= pcfg.rearViewDistance) && (absDistY <= pcfg.verticalViewSpan * 0.8f);

            if (inSightCone || inCloseAwareness) {
                canSeePlayer = arena.hasLineOfSight(position, playerPos);
            }
        }

        if (canSeePlayer) {
            if (!hasDetectedPlayer) {
                behaviorState = AIBehaviorState::ALERT;
                alertTimer = pcfg.alertDuration;
                alertEffectTimer = 1.0f; // Всплывающий маркер тревоги [!]
                hasDetectedPlayer = true;
            }
            lastKnownPlayerPos = playerPos;
            lostSightTimer = pcfg.lostSightTimeout;

            if (behaviorState == AIBehaviorState::ALERT) {
                alertTimer -= dt;
                isFacingLeft = (playerPos.x < position.x);
                if (alertTimer <= 0.0f) {
                    behaviorState = AIBehaviorState::CHASE;
                }
            } else if (behaviorState != AIBehaviorState::COMBAT) {
                behaviorState = AIBehaviorState::CHASE;
            }
        } else {
            if (hasDetectedPlayer) {
                lostSightTimer -= dt;
                if (lostSightTimer <= 0.0f) {
                    // Игрок ушел из зоны видимости: возврат в спокойное патрулирование
                    hasDetectedPlayer = false;
                    behaviorState = AIBehaviorState::PATROL;
                    patrolStartX = position.x;
                    patrolWaitTimer = pcfg.patrolWaitTime;
                    velocity.x = 0.0f;
                } else if (behaviorState != AIBehaviorState::COMBAT) {
                    behaviorState = AIBehaviorState::CHASE;
                }
            } else {
                behaviorState = AIBehaviorState::PATROL;
            }
        }

        // ---------------------------------------------------------
        // РЕЖИМ 1: ПАТРУЛИРОВАНИЕ (PATROL) - Игрок не обнаружен
        // ---------------------------------------------------------
        if (behaviorState == AIBehaviorState::PATROL && attackState == EnemyAttackState::IDLE && aiComboState == AIComboState::IDLE) {
            if (patrolWaitTimer > 0.0f) {
                patrolWaitTimer -= dt;
                velocity.x = 0.0f;
            } else {
                float pDir = isPatrolMovingRight ? 1.0f : -1.0f;
                isFacingLeft = (pDir < 0.0f);
                velocity.x = pDir * pcfg.patrolSpeed;

                // Проверка границ патрулирования
                bool reachedLimit = (isPatrolMovingRight && position.x >= patrolStartX + pcfg.patrolRadius) ||
                                    (!isPatrolMovingRight && position.x <= patrolStartX - pcfg.patrolRadius);

                // Проверка стены/препятствия впереди
                float checkWallX = position.x + pDir * (HALF_W + 12.0f);
                bool wallAhead = arena.overlapsSolid(checkWallX, position.y, 6.0f, HALF_H - 6.0f, HALF_H - 6.0f);

                // Проверка обрыва/пропасти впереди на текущем уровне пола (не падать вниз на нижний блок!)
                bool floorAhead = arena.overlapsSolid(checkWallX, position.y + HALF_H + 8.0f, 6.0f, 4.0f, 4.0f);
                bool footFloor = arena.overlapsSolid(position.x + pDir * HALF_W, position.y + HALF_H + 8.0f, 4.0f, 4.0f, 4.0f);

                if (reachedLimit || wallAhead || !floorAhead || !footFloor) {
                    isPatrolMovingRight = !isPatrolMovingRight;
                    patrolWaitTimer = pcfg.patrolWaitTime;
                    velocity.x = 0.0f;
                }
            }
        }

        if (isSmartAI) {
            // =========================================================
            // УМНЫЙ ИИ: СВЯЗКА КОМБО ЧЕРЕЗ ФОРСУ И ПРЕСЛЕДОВАНИЕ
            // (1f2d1 -> Jump a1adw2 -> Advance +72px -> Jump a1a1adw2)
            // =========================================================
            float dir = isFacingLeft ? -1.0f : 1.0f;
            if (isDodging) {
                // Во время уворота продолжаем смещение по инерции доджа
            } else if (isCounterStanceActive()) {
                velocity.x = 0.0f; // Во время стойки парирования ждем удара игрока
            } else if (hurtTimer > 0.0f && blockStunTimer <= 0.0f) {
                // ВРАГ В ХИТСТАНЕ / ПОЛУЧАЕТ УРОН В КОМБО:
                // Строго запрещено ставить блок, парировать, доджить или управлять движением!
                blockType = BlockType::NONE;
                blockHoldTimer = 0.0f;
            } else if (aiComboState == AIComboState::IDLE) {
                if (attackState == EnemyAttackState::RECOVERY) {
                    attackStateTimer -= dt;
                    if (attackStateTimer <= 0.0f) {
                        attackState = EnemyAttackState::IDLE;
                    }
                }
                if (attackCooldown > 0.0f) attackCooldown -= dt;

                // Если игрок обнаружен (CHASE или ALERT)
                if (behaviorState == AIBehaviorState::CHASE || behaviorState == AIBehaviorState::ALERT) {
                    sf::Vector2f targetPos = canSeePlayer ? playerPos : lastKnownPlayerPos;
                    isFacingLeft = (targetPos.x < position.x);
                    dir = isFacingLeft ? -1.0f : 1.0f;
                    float distToPlayer = std::abs(targetPos.x - position.x);
                    float vertDist = std::abs(targetPos.y - position.y);

                    // Ограничение дистанции преследования: если игрок убежал далеко или на другой ярус карты
                    if (distToPlayer > pcfg.viewDistance * 1.35f || vertDist > pcfg.verticalViewSpan * 1.5f) {
                        hasDetectedPlayer = false;
                        behaviorState = AIBehaviorState::PATROL;
                        patrolStartX = position.x;
                        patrolWaitTimer = pcfg.patrolWaitTime;
                        velocity.x = 0.0f;
                    } else {
                        // Базовые параметры движения по архетипам
                        float baseSpeed = ecfg.speed;
                        float spacingDist = (enemyType == EnemyType::NORMAL) ? 48.0f :
                                            (enemyType == EnemyType::TANK)   ? 65.0f : ecfg.spacingDistance;

                    // 1. ПОВЕДЕНИЕ АГРЕССИВНОГО ДУЭЛЯНТА (DUELIST AGGRESSIVE):
                    // Ближний выверенный спейсинг (~95px: прямо перед носом у игрока, но вне зоны базовых ударов ~90px).
                    // НИКОГДА не подходит в упор! При бездействии игрока агрессивно атакует сам через дэши!
                    if (enemyType == EnemyType::DUELIST) {
                        float minSpacing = spacingDist * 0.94f; // ~101.5px (вне зоны удара с выпадом 104-106px)
                        float maxSpacing = spacingDist * 1.15f; // ~124.2px

                        if (distToPlayer < minSpacing && vertDist < 60.0f) {
                            // Игрок приближается / напирает — дуэлянт отшагивает назад, никогда не подходя в упор!
                            float backFloorX = position.x - dir * (halfW + 12.0f);
                            bool floorBehind = arena.overlapsSolid(backFloorX, position.y + halfH + 8.0f, 6.0f, 4.0f, 4.0f);
                            if (floorBehind) {
                                velocity.x = -dir * (baseSpeed * 0.88f);
                            } else {
                                velocity.x = 0.0f; // Не спрыгивать спиной с обрыва
                            }

                            // Если игрок напирает совсем в упор (<65px) — игра через дэши!
                            if (canDodge && dodgeCooldownTimer <= 0.0f && distToPlayer < 65.0f) {
                                triggerDodge(dir); // Стремительный додж сквозь игрока за спину!
                            } else if (canCounter && counterCooldownTimer <= 0.0f && distToPlayer <= ecfg.counter.triggerDistance && playerIsAttacking) {
                                triggerCounterStance();
                            }
                        } else if (distToPlayer > maxSpacing && vertDist < 90.0f) {
                            // Игрок разрывает дистанцию — дуэлянт агрессивно поджимает вперед
                            velocity.x = dir * baseSpeed;

                            // Сближение через дэш при значительном разрыве
                            if (canDodge && dodgeCooldownTimer <= 0.0f && distToPlayer > 135.0f && distToPlayer < 240.0f) {
                                triggerDodge(dir);
                            }
                        } else {
                            // ЗОЛОТАЯ БЛИЖНЯЯ ЗОНА СПЕЙСИНГА (88 - 109px, около 95px):
                            // Стоит прямо у границы хитбокса, но удар игрока по нему не попадает!
                            velocity.x = 0.0f;
                        }

                        // ПРОВЕРКА БЕЗДЕЙСТВИЯ ИГРОКА (Player Inactivity Attack):
                        // Если игрок бездействует ~0.5с (зависит от интеллекта, нижняя граница ~0.25с — время атаки 2),
                        // дуэлянт сам переходит в агрессивное нападение через дэш!
                        float baseDelay = ecfg.idleAttackDelay;
                        float minDelay = 0.25f;
                        float intel = ecfg.intelligence;
                        float idleThreshold = baseDelay - (intel - 0.5f) * (baseDelay - minDelay) * 2.0f;
                        idleThreshold = std::clamp(idleThreshold, minDelay, baseDelay);

                        if (playerIdleTimer >= idleThreshold && attackCooldown <= 0.0f && distToPlayer < 140.0f && vertDist < 60.0f) {
                            playerIdleTimer = 0.0f;
                            behaviorState = AIBehaviorState::COMBAT;
                            triggerComboSequence();
                            if (canDodge && dodgeCooldownTimer <= 0.0f) {
                                triggerDodge(dir); // Игра через дэши: врывается рывком в атаку!
                            }
                        }
                    }
                    // 2. ПОВЕДЕНИЕ ПАССИВНОГО ДУЭЛЯНТА (DUELIST PASSIVE):
                    // Выверенный спейсинг, блок, парирование, тактическая невидимость на 1с раз в 7с
                    else if (enemyType == EnemyType::DUELIST_PASSIVE) {
                        if (isInvisible()) {
                            // Во время невидимости скрытно обходит с фланга на повышенной скорости
                            velocity.x = dir * (baseSpeed * 1.35f);
                        } else {
                            // Активный выверенный спейсинг (держит идеальную дистанцию боя):
                            // Коридор дистанции: minSpacing (~100px) и maxSpacing (~160px)
                            float minSpacing = spacingDist * 0.88f;
                            float maxSpacing = spacingDist * 1.40f;

                            if (distToPlayer < minSpacing && vertDist < 60.0f) {
                                // Игрок напирает — дуэлянт пятится назад, сохраняя комфортную дистанцию
                                float backFloorX = position.x - dir * (halfW + 12.0f);
                                bool floorBehind = arena.overlapsSolid(backFloorX, position.y + halfH + 8.0f, 6.0f, 4.0f, 4.0f);
                                if (floorBehind) {
                                    velocity.x = -dir * (baseSpeed * 0.72f);
                                } else {
                                    velocity.x = 0.0f; // Не спрыгивать спиной с обрыва
                                }

                                if (canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f) {
                                    blockType = (playerPos.y > position.y + 15.0f) ? BlockType::LOW : BlockType::HIGH;
                                    blockHoldTimer = 0.45f;
                                }

                                if (distToPlayer <= ecfg.counter.triggerDistance) {
                                    if (canCounter && counterCooldownTimer <= 0.0f) {
                                        triggerCounterStance();
                                    } else if (canDodge && dodgeCooldownTimer <= 0.0f) {
                                        triggerDodge(0.0f); // Спот-додж на месте
                                    }
                                }
                            } else if (distToPlayer > maxSpacing && vertDist < 90.0f) {
                                // Игрок отходит — пассивный дуэлянт подшагивает вперед, сохраняя дистанцию
                                float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.60f) : (baseSpeed * 0.75f);
                                velocity.x = dir * stalkSpeed;
                            } else {
                                // Идеальная зона спейсинга (золотая середина): держит стойку, следит за игроком
                                velocity.x = 0.0f;
                                if (canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f && (rand() % 100 < 20)) {
                                    blockType = (playerPos.y > position.y + 15.0f) ? BlockType::LOW : BlockType::HIGH;
                                    blockHoldTimer = 0.35f;
                                }
                            }
                        }
                    }
                    // 3. ПОВЕДЕНИЕ ЖИРНОГО ВРАГА (TANK):
                    // Медленный, тяжелая броня, при близком контакте может закрыться блоком
                    else if (enemyType == EnemyType::TANK) {
                        if (distToPlayer <= 65.0f && vertDist < 50.0f && canBlock && blockHoldTimer <= 0.0f && (nextStarterChoice % 2 == 0)) {
                            blockType = (rand() % 100 < 30) ? BlockType::LOW : BlockType::HIGH;
                            blockHoldTimer = 0.45f;
                        }
                        if (distToPlayer > 55.0f) {
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.75f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        } else {
                            if (attackCooldown > 0.0f) {
                                if (canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f) {
                                    blockType = BlockType::HIGH;
                                    blockHoldTimer = 0.35f;
                                }
                                velocity.x = -dir * (baseSpeed * 0.30f); // Медленно держит дистанцию во время отката
                            } else {
                                velocity.x = 0.0f;
                            }
                        }
                    }
                    // 4. ПОВЕДЕНИЕ ОБЫЧНОГО ВРАГА (NORMAL):
                    else {
                        if (distToPlayer > 48.0f) {
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.8f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        } else {
                            if (attackCooldown > 0.0f) {
                                velocity.x = -dir * (baseSpeed * 0.40f); // Маневрирует и отшагивает назад
                            } else {
                                velocity.x = 0.0f;
                            }
                        }
                    }

                    // НАУЧИЛИ ИИ ПРЫГАТЬ (ПРЫЖКИ ПРИ ПОГОНЕ)
                    bool isGrounded = arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH);
                    if (isGrounded && jumpCooldown <= 0.0f) {
                        float frontX = position.x + dir * (halfW + 24.0f);
                        float farFrontX = position.x + dir * (halfW + 48.0f);
                        bool blockedAhead = arena.overlapsSolid(frontX, position.y + halfH - 8.0f, 8.0f, 16.0f, 8.0f) ||
                                            arena.overlapsSolid(frontX, position.y, 8.0f, 16.0f, 16.0f) ||
                                            arena.overlapsSolid(farFrontX, position.y + halfH - 8.0f, 8.0f, 16.0f, 8.0f) ||
                                            arena.overlapsSolid(farFrontX, position.y, 8.0f, 16.0f, 16.0f);
                        bool spaceAbove = !arena.overlapsSolid(frontX, position.y - 50.0f, halfW, 16.0f, 16.0f) &&
                                          !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 16.0f, 16.0f);
                        float floorCheckX = position.x + dir * (halfW + 24.0f);
                        bool noFloorAhead = !arena.overlapsSolid(floorCheckX, position.y + halfH + 12.0f, 8.0f, 4.0f, 4.0f);
                        bool playerAbove = (targetPos.y < position.y - 45.0f) && (distToPlayer < 260.0f);
                        bool headroomClear = !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 12.0f, 12.0f);

                        if ((blockedAhead && spaceAbove) || (noFloorAhead && distToPlayer > 40.0f) || (playerAbove && headroomClear)) {
                            jump(pcfg.jumpSpeed);
                            velocity.x = dir * std::max(std::abs(velocity.x), 260.0f);
                        }
                    }

                    // В радиусе атаки автоматически начинаем комбо-серию
                    // Пассивный дуэлянт сам не начинает комбо — он только выжидает ошибок игрока (станы, контрудары)
                    // Агрессивный дуэлянт управляет ближними атаками по бездействию игрока, а на дальней дистанции инициирует Grapple Pull
                    if (enemyType == EnemyType::NORMAL || enemyType == EnemyType::TANK) {
                        float attackTriggerDist = (enemyType == EnemyType::TANK) ? 280.0f : 260.0f;
                        if (distToPlayer < attackTriggerDist && vertDist < 80.0f && attackCooldown <= 0.0f) {
                            behaviorState = AIBehaviorState::COMBAT;
                            triggerComboSequence();
                        }
                    } else if (enemyType == EnemyType::DUELIST) {
                        // На дальней дистанции (130 - 220px) агрессивный дуэлянт СТРОГО С ЗЕМЛИ периодически начинает бой длинным зацепом Grapple Pull
                        bool isGrounded = arena.overlapsSolid(position.x, position.y + 3.0f, halfW, halfH, halfH);
                        if (isGrounded && distToPlayer > 130.0f && distToPlayer < 220.0f && vertDist < 60.0f && attackCooldown <= 0.0f && (nextStarterChoice % 2 == 0)) {
                            behaviorState = AIBehaviorState::COMBAT;
                            triggerComboSequence(static_cast<int>(AIStarterType::STARTER_GRAPPLE_PULL));
                        }
                    }
                }
            }
        }

        switch (aiComboState) {
        case AIComboState::IDLE:
            break;

        case AIComboState::APPROACH: {
            const auto& ecfg = getEConfig();
            isFacingLeft = (playerPos.x < position.x);
            dir = isFacingLeft ? -1.0f : 1.0f;
            float distToPlayer = std::abs(playerPos.x - position.x);
            float vertDist = std::abs(playerPos.y - position.y);

            // Если игрок оторвался слишком далеко — прерываем комбо-наскок
            if (distToPlayer > ecfg.perception.viewDistance * 1.35f || vertDist > ecfg.perception.verticalViewSpan * 1.5f) {
                dropCombo();
                behaviorState = AIBehaviorState::PATROL;
                hasDetectedPlayer = false;
                patrolStartX = position.x;
                patrolWaitTimer = ecfg.perception.patrolWaitTime;
                velocity.x = 0.0f;
                break;
            }

            float targetDist = (currentStarterType == AIStarterType::STARTER_GRAPPLE_PULL) ? 175.0f :
                               (currentStarterType == AIStarterType::STARTER_21DB1) ? 54.0f : 48.0f;
            if (enemyType == EnemyType::TANK) targetDist *= 1.3f;
            if (distToPlayer > targetDist) {
                float curApproach = ecfg.approachSpeed;
                if (punishBannerTimer > 0.0f) curApproach *= 1.30f; // Ускоренный напор при панише стана
                velocity.x = dir * curApproach; // Стремительный рывок на сближение
                // Агрессивный дуэлянт: игра через дэши при сближении в комбо
                if (enemyType == EnemyType::DUELIST && canDodge && dodgeCooldownTimer <= 0.0f && distToPlayer > 85.0f) {
                    triggerDodge(dir);
                }
            } else {
                velocity.x = 0.0f;
                switch (currentStarterType) {
                case AIStarterType::STARTER_1F2D1:
                    aiComboState = AIComboState::LAUNCHER_1_WINDUP;
                    aiStateTimer = ecfg.starter1.windup;
                    attackState = EnemyAttackState::WINDUP;
                    attackStateTimer = ecfg.starter1.windup;
                    currentComboName = "AI: 1-f2-d1 (DIAGONAL LAUNCHER)";
                    break;
                case AIStarterType::STARTER_112:
                    aiComboState = AIComboState::STARTER_112_1_WINDUP;
                    aiStateTimer = ecfg.starter2.windup;
                    attackState = EnemyAttackState::WINDUP;
                    attackStateTimer = ecfg.starter2.windup;
                    currentComboName = "AI: 1-1-2 (RAPID COMBO LAUNCHER)";
                    break;
                case AIStarterType::STARTER_21DB1:
                    aiComboState = AIComboState::STARTER_21DB1_2_WINDUP;
                    aiStateTimer = ecfg.starter3.windup;
                    attackState = EnemyAttackState::WINDUP;
                    attackStateTimer = ecfg.starter3.windup;
                    currentComboName = "AI: 2-1-db1 (REVERSE SWEEP LAUNCHER)";
                    break;
                case AIStarterType::STARTER_GRAPPLE_PULL: {
                    bool isGrounded = arena.overlapsSolid(position.x, position.y + 3.0f, halfW, halfH, halfH);
                    if (!isGrounded) {
                        // Если в воздухе — длинный зацеп запрещен, отменяем в обычный лаунчер 1f2d1
                        aiComboState = AIComboState::LAUNCHER_1_WINDUP;
                        aiStateTimer = ecfg.starter1.windup;
                        attackState = EnemyAttackState::WINDUP;
                        attackStateTimer = ecfg.starter1.windup;
                        currentComboName = "AI: 1-f2-d1 (DIAGONAL LAUNCHER)";
                        break;
                    }
                    aiComboState = AIComboState::STARTER_GRAPPLE_WINDUP;
                    aiStateTimer = 0.10f;
                    attackState = EnemyAttackState::WINDUP;
                    attackStateTimer = 0.10f;
                    currentComboName = "AI: GRAPPLE PULL [LONG REACH]";
                    break;
                }
                }
            }
            break;
        }

        // =========================================================
        // СТАРТЕР 1: 1-f2-d1 (Диагональный лаунчер)
        // =========================================================
        case AIComboState::LAUNCHER_1_WINDUP: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::LAUNCHER_1_ACTIVE;
                aiStateTimer = ecfg.starter1.hit1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "", false, 65.0f);
                attackStateTimer = ecfg.starter1.hit1.activeTime;
                attackHasHit = false;
                applyAttackStats(ecfg.starter1.hit1, dir);
            }
            break;
        }

        case AIComboState::LAUNCHER_1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                // Если удар не попал (уворот/промах) — прекращаем комбо
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::LAUNCHER_1_RECOVERY;
                aiStateTimer = ecfg.starter1.hit1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter1.hit1.recoveryTime;
            }
            break;
        }

        case AIComboState::LAUNCHER_1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 1) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Удар 2: f2 (выпад вперед с сильным толчком)
                aiComboState = AIComboState::LAUNCHER_F2_ACTIVE;
                aiStateTimer = ecfg.starter1.hitF2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("f2", "1", false, 65.0f);
                attackStateTimer = ecfg.starter1.hitF2.activeTime;
                attackHasHit = false;
                float elungeF2 = dir * ecfg.starter1.hitF2.lunge;
                if (!arena.overlapsSolid(position.x + elungeF2, position.y, halfW, halfH, halfH)) {
                    position.x += elungeF2; // Выпад вперед
                }
                applyAttackStats(ecfg.starter1.hitF2, dir);
            }
            break;
        }

        case AIComboState::LAUNCHER_F2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::LAUNCHER_F2_RECOVERY;
                aiStateTimer = ecfg.starter1.hitF2.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter1.hitF2.recoveryTime;
            }
            break;
        }

        case AIComboState::LAUNCHER_F2_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 2) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Удар 3: d1 (рывок и апперкот-лаунчер на высоту прыжка + 13px)
                aiComboState = AIComboState::LAUNCHER_D1_ACTIVE;
                aiStateTimer = ecfg.starter1.hitD1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("d1", "1f2", false, 65.0f);
                attackStateTimer = ecfg.starter1.hitD1.activeTime;
                attackHasHit = false;
                float elungeD1 = dir * ecfg.starter1.hitD1.lunge;
                if (!arena.overlapsSolid(position.x + elungeD1, position.y, halfW, halfH, halfH)) {
                    position.x += elungeD1; // Рывок вперед
                }
                applyAttackStats(ecfg.starter1.hitD1, dir);
            }
            break;
        }

        case AIComboState::LAUNCHER_D1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::LAUNCHER_D1_RECOVERY;
                aiStateTimer = ecfg.starter1.hitD1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter1.hitD1.recoveryTime;
            }
            break;
        }

        case AIComboState::LAUNCHER_D1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                // Если игрок не подброшен вверх или отключено воздушное преследование — завершаем комбо
                if (!allowAirFollowup || playerPos.y > position.y - 20.0f) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Переход к общей воздушной связке
                aiComboState = AIComboState::CHASE_JUMP_1;
                aiStateTimer = ecfg.air1.jumpDuration;
                currentComboName = "AI: a1-adw2 (AIR SLAM JUGGLE)";
                velocity.y = ecfg.air1.jumpVelY; // Высокий прыжок вслед за подброшенным
                attackState = EnemyAttackState::IDLE;
                isAirSuspended = false;
            }
            break;
        }

        // =========================================================
        // СТАРТЕР 2: 1-1-2 (Быстрый лаунчер)
        // =========================================================
        case AIComboState::STARTER_112_1_WINDUP: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::STARTER_112_1_ACTIVE;
                aiStateTimer = ecfg.starter2.hit1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "", false, 65.0f);
                attackStateTimer = ecfg.starter2.hit1.activeTime;
                attackHasHit = false;
                applyAttackStats(ecfg.starter2.hit1, dir);
            }
            break;
        }

        case AIComboState::STARTER_112_1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_112_1_RECOVERY;
                aiStateTimer = ecfg.starter2.hit1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter2.hit1.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_112_1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 1) {
                    finishCombo(attackHasHit);
                    break;
                }
                aiComboState = AIComboState::STARTER_112_2ND_1_ACTIVE;
                aiStateTimer = ecfg.starter2.hit2nd1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "1", false, 65.0f);
                attackStateTimer = ecfg.starter2.hit2nd1.activeTime;
                attackHasHit = false;
                float elunge2nd1 = dir * ecfg.starter2.hit2nd1.lunge;
                if (!arena.overlapsSolid(position.x + elunge2nd1, position.y, halfW, halfH, halfH)) {
                    position.x += elunge2nd1; // Шаг вперед
                }
                applyAttackStats(ecfg.starter2.hit2nd1, dir);
            }
            break;
        }

        case AIComboState::STARTER_112_2ND_1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_112_2ND_1_RECOVERY;
                aiStateTimer = ecfg.starter2.hit2nd1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter2.hit2nd1.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_112_2ND_1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 2) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Удар 3: 2 (сильный лаунчер вверх)
                aiComboState = AIComboState::STARTER_112_2_ACTIVE;
                aiStateTimer = ecfg.starter2.hit2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("2", "11", false, 75.0f);
                attackStateTimer = ecfg.starter2.hit2.activeTime;
                attackHasHit = false;
                float elunge2 = dir * ecfg.starter2.hit2.lunge;
                if (!arena.overlapsSolid(position.x + elunge2, position.y, halfW, halfH, halfH)) {
                    position.x += elunge2; // Выпад
                }
                applyAttackStats(ecfg.starter2.hit2, dir);
            }
            break;
        }

        case AIComboState::STARTER_112_2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_112_2_RECOVERY;
                aiStateTimer = ecfg.starter2.hit2.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter2.hit2.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_112_2_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!allowAirFollowup || playerPos.y > position.y - 20.0f) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Переход к общей воздушной связке
                aiComboState = AIComboState::CHASE_JUMP_1;
                aiStateTimer = ecfg.air1.jumpDuration;
                currentComboName = "AI: a1-adw2 (AIR SLAM JUGGLE)";
                velocity.y = ecfg.air1.jumpVelY;
                attackState = EnemyAttackState::IDLE;
                isAirSuspended = false;
            }
            break;
        }

        // =========================================================
        // СТАРТЕР 3: 2-1-db1 (Обход со спины и вертикальная подсечка)
        // =========================================================
        case AIComboState::STARTER_21DB1_2_WINDUP: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::STARTER_21DB1_2_ACTIVE;
                aiStateTimer = ecfg.starter3.hit2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("2", "", false, 65.0f);
                attackStateTimer = ecfg.starter3.hit2.activeTime;
                attackHasHit = false;
                applyAttackStats(ecfg.starter3.hit2, dir);
            }
            break;
        }

        case AIComboState::STARTER_21DB1_2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_21DB1_2_RECOVERY;
                aiStateTimer = ecfg.starter3.hit2.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter3.hit2.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_21DB1_2_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 1) {
                    finishCombo(attackHasHit);
                    break;
                }
                aiComboState = AIComboState::STARTER_21DB1_1_ACTIVE;
                aiStateTimer = ecfg.starter3.hit1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "2", false, 65.0f);
                attackStateTimer = ecfg.starter3.hit1.activeTime;
                attackHasHit = false;
                float elungeHit1 = dir * ecfg.starter3.hit1.lunge;
                if (!arena.overlapsSolid(position.x + elungeHit1, position.y, halfW, halfH, halfH)) {
                    position.x += elungeHit1; // Шаг вперед вдогонку
                }
                applyAttackStats(ecfg.starter3.hit1, dir);
            }
            break;
        }

        case AIComboState::STARTER_21DB1_1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_21DB1_1_RECOVERY;
                aiStateTimer = ecfg.starter3.hit1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter3.hit1.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_21DB1_1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (maxComboHits <= 2) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Рывок за спину игрока
                aiComboState = AIComboState::STARTER_21DB1_DASH_BEHIND;
                aiStateTimer = ecfg.starter3.dashBehindDuration;
                attackState = EnemyAttackState::IDLE;
                // Перемещаемся за спину игрока с проверкой стен
                float tgtX = playerPos.x + dir * ecfg.starter3.dashBehindOffset;
                if (!arena.overlapsSolid(tgtX, position.y, halfW, halfH, halfH)) {
                    position.x = tgtX;
                }
                // Разворачиваемся лицом к игроку
                isFacingLeft = !isFacingLeft;
                dir = isFacingLeft ? -1.0f : 1.0f;
            }
            break;
        }

        case AIComboState::STARTER_21DB1_DASH_BEHIND: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                // Выполнение подсечки db1 с разворота: поднимает строго вертикально вверх!
                aiComboState = AIComboState::STARTER_21DB1_SWEEP_ACTIVE;
                aiStateTimer = ecfg.starter3.hitSweep.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("db1", "21", false, 75.0f);
                attackStateTimer = ecfg.starter3.hitSweep.activeTime;
                attackHasHit = false;
                applyAttackStats(ecfg.starter3.hitSweep, dir);
                currentAttackHitLevel = HitLevel::LOW;
                currentAttackKnockX = ecfg.starter3.hitSweep.knockX; // СТРОГО ВЕРТИКАЛЬНЫЙ ПОДЪЕМ!
            }
            break;
        }

        case AIComboState::STARTER_21DB1_SWEEP_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::STARTER_21DB1_SWEEP_RECOVERY;
                aiStateTimer = ecfg.starter3.hitSweep.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.starter3.hitSweep.recoveryTime;
            }
            break;
        }

        case AIComboState::STARTER_21DB1_SWEEP_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!allowAirFollowup || playerPos.y > position.y - 20.0f) {
                    finishCombo(attackHasHit);
                    break;
                }
                // Переход к общей воздушной связке (прыжок НА МЕСТЕ без движения вперед!)
                aiComboState = AIComboState::CHASE_JUMP_1;
                aiStateTimer = ecfg.air1.jumpDuration;
                currentComboName = "AI: a1-adw2 (VERTICAL AIR SLAM)";
                velocity.y = ecfg.air1.jumpVelY;
                velocity.x = 0.0f; // Прыжок строго на месте!
                attackState = EnemyAttackState::IDLE;
                isAirSuspended = false;
            }
            break;
        }

        // =========================================================
        // СТАРТЕР 4: Длинный зацеп с притягиванием (Grapple Pull)
        // =========================================================
        case AIComboState::STARTER_GRAPPLE_WINDUP: {
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::STARTER_GRAPPLE_ACTIVE;
                aiStateTimer = 0.20f;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "", false, 75.0f);
                attackStateTimer = 0.20f;
                attackHasHit = false;
                currentHbw = 200.0f; // ДЛИННЫЙ ХИТБОКС (200px)
                currentHbh = 50.0f;
                currentHbyOffset = -22.0f;
                currentAttackDamage = 2.0f;
                currentAttackKnockX = -dir * 580.0f; // Притягивает игрока к себе!
                currentAttackKnockY = -25.0f;
                currentAttackStun = 0.38f;
                currentAttackIsDiveSlam = false;
            }
            break;
        }

        case AIComboState::STARTER_GRAPPLE_ACTIVE: {
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    finishCombo(false);
                } else {
                    aiComboState = AIComboState::STARTER_GRAPPLE_RECOVERY;
                    aiStateTimer = 0.08f;
                    attackState = EnemyAttackState::RECOVERY;
                    attackStateTimer = 0.08f;
                }
            }
            break;
        }

        case AIComboState::STARTER_GRAPPLE_RECOVERY: {
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (grappleBranchChoice == 0) {
                    // Ветка А: Скай-Лаунчер (подброс в воздух)
                    aiComboState = AIComboState::STARTER_GRAPPLE_LAUNCHER_ACTIVE;
                    aiStateTimer = 0.16f;
                    attackState = EnemyAttackState::ATTACKING;
                    SoundManager::get().playAttack("2", "", false, 75.0f);
                    attackStateTimer = 0.16f;
                    attackHasHit = false;
                    currentHbw = 70.0f;
                    currentHbh = 65.0f;
                    currentHbyOffset = -30.0f;
                    currentAttackDamage = 3.0f;
                    currentAttackKnockX = dir * 60.0f;
                    currentAttackKnockY = -530.0f;
                    currentAttackStun = 0.40f;
                    currentComboName = "AI: GRAPPLE -> SKY LAUNCHER!";
                } else {
                    // Ветка Б: Силовой толчок (Heavy Shove)
                    aiComboState = AIComboState::STARTER_GRAPPLE_SHOVE_ACTIVE;
                    aiStateTimer = 0.16f;
                    attackState = EnemyAttackState::ATTACKING;
                    SoundManager::get().playAttack("2", "", false, 75.0f);
                    attackStateTimer = 0.16f;
                    attackHasHit = false;
                    currentHbw = 75.0f;
                    currentHbh = 50.0f;
                    currentHbyOffset = -20.0f;
                    currentAttackDamage = 3.5f;
                    currentAttackKnockX = dir * 650.0f;
                    currentAttackKnockY = -40.0f;
                    currentAttackStun = 0.45f;
                    currentComboName = "AI: GRAPPLE -> HEAVY SHOVE!";
                }
            }
            break;
        }

        case AIComboState::STARTER_GRAPPLE_LAUNCHER_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    finishCombo(false);
                } else {
                    aiComboState = AIComboState::CHASE_JUMP_1;
                    aiStateTimer = ecfg.air1.jumpDuration;
                    currentComboName = "AI: a1-adw2 (AIR SLAM JUGGLE)";
                    velocity.y = ecfg.air1.jumpVelY;
                    attackState = EnemyAttackState::IDLE;
                    isAirSuspended = false;
                }
            }
            break;
        }

        case AIComboState::STARTER_GRAPPLE_SHOVE_ACTIVE: {
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    finishCombo(false);
                } else {
                    finishCombo(true);
                    triggerDodge(dir); // Додж вперед вдогонку
                }
            }
            break;
        }

        // =========================================================
        // ОБЩАЯ ВОЗДУШНАЯ СВЯЗКА (СОВМЕЩЕНИЕ ЧЕРЕЗ ФОРСУ)
        // =========================================================
        case AIComboState::CHASE_JUMP_1: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            float diffX = playerPos.x - position.x;
            isFacingLeft = (diffX < 0.0f);
            if (currentStarterType == AIStarterType::STARTER_21DB1) {
                velocity.x = 0.0f; // Прыжок на месте
            } else {
                velocity.x = std::clamp(diffX * ecfg.air1.jumpHorizFactor, -ecfg.air1.jumpMaxSpeedX, ecfg.air1.jumpMaxSpeedX);
            }

            bool closeX = (currentStarterType == AIStarterType::STARTER_21DB1) || (std::abs(diffX) <= 45.0f);
            bool closeY = std::abs(position.y - playerPos.y) < 45.0f || velocity.y >= -100.0f;

            // Переход к удару в воздухе когда подобрался близко по X и поравнялся по Y, либо вышло время
            if ((closeX && closeY) || aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::AIR_1_A1_ACTIVE;
                aiStateTimer = ecfg.air1.hitA1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("a1", "", true, 70.0f);
                attackStateTimer = ecfg.air1.hitA1.activeTime;
                attackHasHit = false;
                isAirSuspended = true; // Зависаем в воздухе во время жонглирования!
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                applyAttackStats(ecfg.air1.hitA1, isFacingLeft ? -1.0f : 1.0f);
                currentAttackHitLevel = HitLevel::OVERHEAD;
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_1_A1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::AIR_1_A1_RECOVERY;
                aiStateTimer = ecfg.air1.hitA1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.air1.hitA1.recoveryTime;
            }
            break;
        }

        case AIComboState::AIR_1_A1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                // Удар adw2: слэм вниз в пол с Ground Bounce
                aiComboState = AIComboState::AIR_1_ADW2_ACTIVE;
                aiStateTimer = ecfg.air1.hitAdw2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("adw2", "", true, 75.0f);
                attackStateTimer = ecfg.air1.hitAdw2.activeTime;
                attackHasHit = false;
                isAirSuspended = false;
                velocity.y = 450.0f; // Стремительный удар вниз
                float curDir = isFacingLeft ? -1.0f : 1.0f;
                applyAttackStats(ecfg.air1.hitAdw2, curDir);
                currentAttackHitLevel = HitLevel::OVERHEAD;
                currentAttackIsDiveSlam = false;
                if (currentStarterType == AIStarterType::STARTER_21DB1) {
                    velocity.x = 0.0f;
                    currentAttackKnockX = 0.0f; // Вбивание строго вниз на месте!
                } else {
                    float diffX = playerPos.x - position.x;
                    velocity.x = std::clamp(diffX * 4.0f, -250.0f, 250.0f);
                }
            }
            break;
        }

        case AIComboState::AIR_1_ADW2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::AIR_1_ADW2_RECOVERY;
                aiStateTimer = ecfg.air1.hitAdw2.recoveryTime + 0.06f;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.air1.hitAdw2.recoveryTime + 0.06f;
            }
            break;
        }

        case AIComboState::AIR_1_ADW2_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            bool grounded = arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH);
            if (grounded || aiStateTimer <= 0.0f) {
                // ==========================================
                // СОВМЕЩЕНИЕ 2: ПРОТИВНИК ОТСКОЧИЛ ОТ ПОЛА ПО X!
                // Динамическое сближение с игроком по X
                // ==========================================
                aiComboState = AIComboState::CHASE_GROUND_ADVANCE;
                aiStateTimer = ecfg.air2.groundAdvanceDuration + 0.10f;
                currentComboName = (currentStarterType == AIStarterType::STARTER_21DB1)
                    ? "AI CHASE: GROUND BOUNCE RECOVERY"
                    : "AI CHASE: DYNAMIC ADVANCE TO PLAYER";
                attackState = EnemyAttackState::IDLE;
                isAirSuspended = false;
            }
            break;
        }

        case AIComboState::CHASE_GROUND_ADVANCE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            float diffX = playerPos.x - position.x;
            isFacingLeft = (diffX < 0.0f);
            if (currentStarterType == AIStarterType::STARTER_21DB1) {
                velocity.x = 0.0f; // Игрок отскочил прямо над нами!
                if (aiStateTimer <= ecfg.air2.groundAdvanceDuration) {
                    aiComboState = AIComboState::CHASE_JUMP_2;
                    aiStateTimer = ecfg.air2.jumpDuration;
                    currentComboName = "AI: a1-a1-adw2 (VERTICAL DUAL DIVE SLAM)";
                    velocity.y = ecfg.air2.jumpVelY;
                    velocity.x = 0.0f;
                    attackState = EnemyAttackState::IDLE;
                }
            } else {
                velocity.x = std::clamp(diffX * 7.0f, -680.0f, 680.0f);
                float distToPlayer = std::abs(diffX);
                if (distToPlayer <= 42.0f || aiStateTimer <= 0.0f) {
                    // Прыжок навстречу отскочившему игроку
                    aiComboState = AIComboState::CHASE_JUMP_2;
                    aiStateTimer = ecfg.air2.jumpDuration;
                    currentComboName = "AI: a1-a1-adw2 (DUAL AIR DIVE SLAM)";
                    velocity.y = ecfg.air2.jumpVelY;
                    velocity.x = std::clamp(diffX * 4.0f, -320.0f, 320.0f);
                    attackState = EnemyAttackState::IDLE;
                }
            }
            break;
        }

        case AIComboState::CHASE_JUMP_2: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            float diffX = playerPos.x - position.x;
            isFacingLeft = (diffX < 0.0f);
            if (currentStarterType == AIStarterType::STARTER_21DB1) {
                velocity.x = 0.0f;
            } else {
                velocity.x = std::clamp(diffX * 6.0f, -450.0f, 450.0f);
            }
            bool closeX = (currentStarterType == AIStarterType::STARTER_21DB1) || (std::abs(diffX) <= 45.0f);
            bool closeY = std::abs(position.y - playerPos.y) < 45.0f || velocity.y >= -100.0f;

            if ((closeX && closeY) || aiStateTimer <= 0.0f) {
                // Первый a1
                aiComboState = AIComboState::AIR_2_A1_1_ACTIVE;
                aiStateTimer = ecfg.air2.hitA1_1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("a1", "", true, 70.0f);
                attackStateTimer = ecfg.air2.hitA1_1.activeTime;
                attackHasHit = false;
                isAirSuspended = true; // Зависаем в воздухе
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                applyAttackStats(ecfg.air2.hitA1_1, isFacingLeft ? -1.0f : 1.0f);
                currentAttackHitLevel = HitLevel::OVERHEAD;
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_2_A1_1_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::AIR_2_A1_1_RECOVERY;
                aiStateTimer = ecfg.air2.hitA1_1.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.air2.hitA1_1.recoveryTime;
            }
            break;
        }

        case AIComboState::AIR_2_A1_1_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                // Второй a1
                aiComboState = AIComboState::AIR_2_A1_2_ACTIVE;
                aiStateTimer = ecfg.air2.hitA1_2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("a1", "a1", true, 70.0f);
                attackStateTimer = ecfg.air2.hitA1_2.activeTime;
                attackHasHit = false;
                isAirSuspended = true;
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                applyAttackStats(ecfg.air2.hitA1_2, isFacingLeft ? -1.0f : 1.0f);
                currentAttackHitLevel = HitLevel::OVERHEAD;
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_2_A1_2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                if (!attackHasHit) {
                    dropCombo();
                    break;
                }
                aiComboState = AIComboState::AIR_2_A1_2_RECOVERY;
                aiStateTimer = ecfg.air2.hitA1_2.recoveryTime;
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.air2.hitA1_2.recoveryTime;
            }
            break;
        }

        case AIComboState::AIR_2_A1_2_RECOVERY: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            velocity.y = 0.0f;
            isAirSuspended = true;
            if (aiStateTimer <= 0.0f) {
                // Финальный дайв-слэм adw2
                aiComboState = AIComboState::AIR_2_ADW2_ACTIVE;
                aiStateTimer = ecfg.air2.hitAdw2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("adw2", "a1a1", true, 75.0f);
                attackStateTimer = ecfg.air2.hitAdw2.activeTime;
                attackHasHit = false;
                isAirSuspended = false;
                float curDir = isFacingLeft ? -1.0f : 1.0f;
                applyAttackStats(ecfg.air2.hitAdw2, curDir);
                currentAttackHitLevel = HitLevel::OVERHEAD;
                if (currentStarterType == AIStarterType::STARTER_21DB1) {
                    currentAttackKnockX = curDir * ecfg.air2.finalKnockXVertical;
                }
                currentAttackIsDiveSlam = false; // Финишер комбо — не активирует повторный дайв-слэм отскок игрока
                velocity.y = ecfg.air2.diveSpeedY; // Пикируют вместе вниз
            }
            break;
        }

        case AIComboState::AIR_2_ADW2_ACTIVE: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            bool grounded = arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH);
            if (grounded || aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::COMBO_FINISHED;
                aiStateTimer = ecfg.air2.hitAdw2.recoveryTime; // Откат после комбо
                attackState = EnemyAttackState::RECOVERY;
                attackStateTimer = ecfg.air2.hitAdw2.recoveryTime;
                currentComboName = "AI: FULL COMBO CHAIN FINISHED!";
                isAirSuspended = false;
            }
            break;
        }

        case AIComboState::COMBO_FINISHED: {
            const auto& ecfg = getEConfig();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::IDLE;
                attackState = EnemyAttackState::IDLE;
                attackCooldown = (enemyType == EnemyType::DUELIST) ? 0.20f : ((enemyType == EnemyType::TANK) ? 0.38f : 0.28f);
                currentComboName = "";
                isAirSuspended = false;
                behaviorState = hasDetectedPlayer ? AIBehaviorState::CHASE : AIBehaviorState::PATROL;
            }
            break;
        }
        }
    } else {
        // Логика обычного противника / тренировочного манекена
        if (attackCooldown > 0.0f) attackCooldown -= dt;

        if (behaviorState == AIBehaviorState::CHASE || behaviorState == AIBehaviorState::ALERT) {
            sf::Vector2f targetPos = canSeePlayer ? playerPos : lastKnownPlayerPos;
            isFacingLeft = (targetPos.x < position.x);
            float dir = isFacingLeft ? -1.0f : 1.0f;
            float distToPlayer = std::abs(targetPos.x - position.x);
            float vertDist = std::abs(targetPos.y - position.y);

            // Ограничение дистанции преследования манекена
            if (distToPlayer > pcfg.viewDistance * 1.35f || vertDist > pcfg.verticalViewSpan * 1.5f) {
                hasDetectedPlayer = false;
                behaviorState = AIBehaviorState::PATROL;
                patrolStartX = position.x;
                patrolWaitTimer = pcfg.patrolWaitTime;
                velocity.x = 0.0f;
            } else if (attackState == EnemyAttackState::IDLE) {
                if (distToPlayer > 48.0f) {
                    velocity.x = dir * 180.0f;
                } else {
                    velocity.x = 0.0f;
                }

                // Прыжки манекена при преследовании
                bool isGrounded = arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH);
                if (isGrounded && jumpCooldown <= 0.0f) {
                    float frontX = position.x + dir * (halfW + 24.0f);
                    float farFrontX = position.x + dir * (halfW + 48.0f);
                    bool blockedAhead = arena.overlapsSolid(frontX, position.y + halfH - 8.0f, 8.0f, 16.0f, 8.0f) ||
                                        arena.overlapsSolid(frontX, position.y, 8.0f, 16.0f, 16.0f) ||
                                        arena.overlapsSolid(farFrontX, position.y + halfH - 8.0f, 8.0f, 16.0f, 8.0f) ||
                                        arena.overlapsSolid(farFrontX, position.y, 8.0f, 16.0f, 16.0f);
                    bool spaceAbove = !arena.overlapsSolid(frontX, position.y - 50.0f, halfW, 16.0f, 16.0f) &&
                                      !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 16.0f, 16.0f);
                    bool noFloorAhead = !arena.overlapsSolid(frontX, position.y + halfH + 12.0f, 8.0f, 4.0f, 4.0f);
                    bool playerAbove = (targetPos.y < position.y - 45.0f) && (distToPlayer < 260.0f);
                    bool headroomClear = !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 12.0f, 12.0f);

                    if ((blockedAhead && spaceAbove) || (noFloorAhead && distToPlayer > 40.0f) || (playerAbove && headroomClear)) {
                        jump(pcfg.jumpSpeed);
                        velocity.x = dir * 220.0f;
                    }
                }

                if (distToPlayer < 120.0f && vertDist < 50.0f && attackCooldown <= 0.0f) {
                    triggerAttack();
                }
            }
        }

        const auto& ecfg = getEConfig();
        if (attackState == EnemyAttackState::WINDUP) {
            attackStateTimer -= dt;
            if (attackStateTimer <= 0.0f) {
                attackState = EnemyAttackState::ATTACKING;
                SoundManager::get().playAttack("1", "", false, 65.0f);
                attackStateTimer = ecfg.stats.dummyActiveTime;
                attackHasHit = false;

                currentHbw = ecfg.stats.dummyHbw * hitboxScale;
                currentHbh = ecfg.stats.dummyHbh * hitboxScale;
                currentHbyOffset = ecfg.stats.dummyHbyOffset * scale;
            }
        } else if (attackState == EnemyAttackState::ATTACKING) {
            attackStateTimer -= dt;
            if (attackStateTimer <= 0.0f) {
                attackState = EnemyAttackState::RECOVERY;
                // При промахе восстанавливается дольше, при попадании быстрее
                attackStateTimer = attackHasHit ? ecfg.stats.hitRecovery : ecfg.stats.whiffRecovery;
                attackCooldown = ecfg.stats.dummyCooldown;
            }
        } else if (attackState == EnemyAttackState::RECOVERY) {
            attackStateTimer -= dt;
            if (attackStateTimer <= 0.0f) {
                attackState = EnemyAttackState::IDLE;
                behaviorState = hasDetectedPlayer ? AIBehaviorState::CHASE : AIBehaviorState::PATROL;
            }
        }
    }
    }

    // Динамическое обновление хитбокса атаки каждый кадр (привязано к актуальной позиции врага)
    if (attackState == EnemyAttackState::ATTACKING) {
        float hbx = isFacingLeft ? (position.x - halfW - currentHbw) : (position.x + halfW);
        attackHitbox = sf::FloatRect({hbx, position.y + currentHbyOffset}, {currentHbw, currentHbh});
    } else {
        attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    }

    // Гравитация
    const float enemyGravity = EnemyConfig::get().stats.gravity;
    if (isAirSuspended) {
        velocity.y = 0.0f;
    } else if (hurtTimer > 0.0f && std::abs(velocity.y) <= 1.0f) {
        velocity.y = 0.0f; // Зависание на уровне дэша во время стана
    } else if (hurtTimer > 0.0f && velocity.y <= 100.0f) {
        velocity.y += (enemyGravity * 0.18f) * dt;
    } else {
        velocity.y += enemyGravity * dt;
    }
    if (velocity.y > 1800.0f) velocity.y = 1800.0f;

    // Проверка, стоит ли противник на твердой поверхности
    bool isGrounded = arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH);
    if (isGrounded && hurtTimer <= 0.0f) {
        hasWallBounced = false;
    }

    // Защита от застревания в стенах при ударах (De-penetration / Unstuck logic)
    if (arena.overlapsSolid(position.x, position.y, halfW, halfH, halfH)) {
        for (float dist = 1.0f; dist <= 64.0f; dist += 1.0f) {
            if (!arena.overlapsSolid(position.x - dist, position.y, halfW, halfH, halfH)) {
                position.x -= dist;
                velocity.x = 0.0f;
                break;
            }
            if (!arena.overlapsSolid(position.x + dist, position.y, halfW, halfH, halfH)) {
                position.x += dist;
                velocity.x = 0.0f;
                break;
            }
            if (!arena.overlapsSolid(position.x, position.y - dist, halfW, halfH, halfH)) {
                position.y -= dist;
                velocity.y = 0.0f;
                break;
            }
        }
    }

    // Движение по X с пошаговой коллизией (предотвращает заталкивание в стену)
    float deltaX = velocity.x * dt;
    if (deltaX != 0.0f) {
        float stepSign = (deltaX > 0.0f) ? 1.0f : -1.0f;
        float remaining = std::abs(deltaX);
        float preCollisionSpeedX = std::abs(velocity.x);
        while (remaining > 0.0f) {
            float step = std::min(remaining, 1.0f) * stepSign;
            float nextX = position.x + step;
            if (!arena.overlapsSolid(nextX, position.y, halfW, halfH, halfH)) {
                // Если противник в режиме патрулирования, он ни в коем случае не должен делать шаг,
                // если его передний край теряет твердый пол (чтобы не падать вниз на нижний блок!)
                // Во время получения урона / отталкивания (hurtTimer > 0) враг и манекен отлетают свободно без блокировки обрыва!
                if (hurtTimer <= 0.0f && behaviorState == AIBehaviorState::PATROL && attackState == EnemyAttackState::IDLE && enemyType != EnemyType::DUMMY) {
                    bool hasFloor = arena.overlapsSolid(nextX + stepSign * (halfW - 2.0f), position.y + halfH + 8.0f, 4.0f, 4.0f, 4.0f);
                    if (!hasFloor) {
                        velocity.x = 0.0f;
                        isPatrolMovingRight = !isPatrolMovingRight;
                        patrolWaitTimer = EnemyConfig::get().perception.patrolWaitTime;
                        break;
                    }
                }
                position.x = nextX;
                remaining -= std::abs(step);
            } else {
                // Столкновение со стеной!
                // Механика отскока от стены (Wall Bounce как в Mortal Kombat X):
                float minSpeed = CombatConfig::getWallBounceMinSpeed();
                if (hurtTimer > 0.0f && preCollisionSpeedX >= minSpeed && !hasWallBounced && !isDead) {
                    hasWallBounced = true;
                    float bounceDmg = CombatConfig::getWallBounceDamage();
                    float forceDamp = CombatConfig::getWallBounceForceDamp();
                    float popY = CombatConfig::getWallBouncePopY();
                    float bounceHitstun = CombatConfig::getWallBounceHitstun();

                    hp -= bounceDmg;
                    if (hp <= 0.0f) {
                        hp = 0.0f;
                        isDead = true;
                        deathFadeTimer = 0.6f;
                        respawnTimer = 0.0f;
                        if (Arena::getActive()) {
                            Arena::getActive()->spawnKillBlood(position, sf::Vector2f(-stepSign * 0.707f, -0.707f), maxHp);
                        }
                    }

                    // Сильно сниженная форса (25-30% скорости, макс 240px/s в сторону от стены)
                    velocity.x = -stepSign * std::min(240.0f, preCollisionSpeedX * forceDamp);
                    velocity.y = popY;
                    hurtTimer = std::max(hurtTimer, bounceHitstun);

                    if (CombatSystem::getActive()) {
                        CombatSystem::getActive()->addSpecialMarker(
                            position + sf::Vector2f(0.0f, -45.0f),
                            "★ WALL BOUNCE! (-" + std::to_string(static_cast<int>(bounceDmg)) + " HP) ★",
                            sf::Color(255, 140, 40), 1.15f
                        );
                        CombatSystem::getActive()->triggerShake(0.20f, 6.0f);
                    }

                    SoundManager::get().play("adw2", 50.0f);

                    if (Arena::getActive()) {
                        Arena::getActive()->spawnWallBounceBlood(
                            sf::Vector2f(position.x + stepSign * halfW, position.y),
                            sf::Vector2f(-stepSign * 0.8f, -0.6f)
                        );
                    }
                } else {
                    velocity.x = 0.0f;
                }
                break;
            }
        }
    }

    // Горизонтальное затухание скорости (плавное экспоненциальное трение):
    float friction = ((isGrounded && velocity.y >= 0.0f) || (hurtTimer > 0.0f && std::abs(velocity.y) <= 1.0f)) ? 10.0f : 4.0f;
    velocity.x *= std::exp(-friction * dt);
    if (std::abs(velocity.x) < 0.1f) velocity.x = 0.0f;

    // Движение по Y с коллизией
    float deltaY = velocity.y * dt;
    if (deltaY != 0.0f) {
        float nextY = position.y + deltaY;
        if (deltaY > 0.0f) {
            // Падение
            if (arena.overlapsSolid(position.x, nextY, halfW, halfH, halfH)) {
                // Отскок от пола (Ground Bounce / Dive Slam Bounce)
                if (diveSlamBounce) {
                    velocity.y = -280.0f; // Подлетает слегка вверх
                    velocity.x = diveSlamDir * 1150.0f; // Отталкивается по X на длину дэша
                    diveSlamBounce = false;
                } else if (velocity.y > 450.0f) {
                    velocity.y = -280.0f;
                    velocity.x *= 0.55f; // Сохраняем умеренный боковой импульс
                } else {
                    velocity.y = 0.0f;
                }
                // Прижимаем к полу
                position.y = std::floor((nextY + halfH) / TILE_SIZE) * TILE_SIZE - halfH - 0.01f;
            } else {
                position.y = nextY;
            }
        } else {
            // Подброс в воздух
            if (arena.overlapsSolid(position.x, nextY, halfW, halfH, halfH)) {
                velocity.y = 0.0f;
            } else {
                position.y = nextY;
            }
        }
    }

}

void Enemy::draw(sf::RenderWindow& window, const sf::Font* font) {
    if (font && _cachedFont != font) {
        _cachedFont = font;
        _cachedParryText.emplace(*font, "PARRY", 11);
        _cachedParryText->setFillColor(sf::Color(0, 240, 255));
        _cachedWarnExText.emplace(*font, "!", 13);
        _cachedWarnExText->setFillColor(sf::Color::White);
        _cachedNameText.emplace(*font, "", 10);
        _cachedDodgeText.emplace(*font, toSfString("DODGE!"), 10);
        _cachedDodgeText->setFillColor(sf::Color(255, 230, 80));
        _cachedExText.emplace(*font, "!", 12);
        _cachedExText->setStyle(sf::Text::Bold);
        _cachedExText->setFillColor(sf::Color(30, 20, 0));
        _cachedAlertText.emplace(*font, toSfString("ОБНАРУЖЕН!"), 9);
        _cachedAlertText->setFillColor(sf::Color(255, 225, 60));
        _cachedPatText.emplace(*font, toSfString("[ПАТРУЛЬ]"), 9);
        _cachedPatText->setFillColor(sf::Color(100, 210, 255, 200));
        _cachedSearchBadge.emplace(*font, toSfString("[?] ПОИСК"), 9);
        _cachedSearchBadge->setFillColor(sf::Color(255, 195, 60));
        _cachedChaseBadge.emplace(*font, toSfString("[!] ПОГОНЯ"), 9);
        _cachedChaseBadge->setFillColor(sf::Color(255, 80, 80));
        _cachedAiBanner.emplace(*font, "", 12);
        _cachedAiBanner->setFillColor(sf::Color(255, 230, 90));
    }

    // 1. Модель противника (с учетом размера архетипа: Танк в 1.5 раза больше)
    sf::RectangleShape body({halfW * 2.0f, halfH * 2.0f});
    body.setOrigin({halfW, halfH});
    body.setPosition(position);
    if (spinTimer > 0.0f) {
        body.setRotation(sf::degrees(spinAngle));
    } else {
        body.setRotation(sf::degrees(0.0f));
    }
    body.setOutlineThickness(2.0f);
    body.setOutlineColor(sf::Color(25, 25, 30, 230));

    if (isDead) {
        if (deathFadeTimer > 0.0f) {
            float alpha = std::clamp((deathFadeTimer / 0.6f) * 160.0f, 0.0f, 255.0f);
            body.setFillColor(sf::Color(100, 100, 115, static_cast<std::uint8_t>(alpha)));
            body.setOutlineColor(sf::Color(40, 40, 45, static_cast<std::uint8_t>(alpha)));
            window.draw(body);
        }
        return;
    }

    // ЦВЕТОВЫЕ ОТЛИЧИЯ АРХЕТИПОВ:
    // Обычный — серый
    // Жирный (Танк) — красный
    // Агрессивный дуэлянт — фиолетовый
    // Пассивный дуэлянт — голубой
    sf::Color baseColor;
    if (enemyType == EnemyType::NORMAL) {
        baseColor = sf::Color(135, 140, 150); // Серый цвет
    } else if (enemyType == EnemyType::TANK) {
        baseColor = sf::Color(215, 38, 48);   // Красный цвет
    } else if (enemyType == EnemyType::DUELIST) {
        baseColor = sf::Color(155, 45, 225);  // Фиолетовый цвет (агрессивный)
    } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
        baseColor = sf::Color(45, 185, 245);  // Голубой цвет (пассивный)
    } else if (enemyType == EnemyType::DUMMY) {
        baseColor = sf::Color(220, 175, 75);  // Золотисто-древесный цвет (манекен)
    } else {
        baseColor = isSmartAI ? sf::Color(110, 45, 150) : sf::Color(220, 75, 55);
    }

    if (isInvisible()) {
        body.setFillColor(sf::Color(45, 185, 245, 30));
        body.setOutlineColor(sf::Color(70, 220, 255, 60));
        window.draw(body);
        return; // Полная невидимость: скрываем полоску ХП и маркеры!
    }

    if (isCounterStanceActive()) {
        body.setFillColor(sf::Color(0, 230, 255));   // Ярко-лазурная стойка парирования
        body.setOutlineColor(sf::Color(255, 255, 255));
    } else if (hurtTimer > 0.0f) {
        body.setFillColor(sf::Color(255, 130, 145)); // Вспышка при получении урона
    } else if (attackState == EnemyAttackState::ATTACKING) {
        if (enemyType == EnemyType::TANK) body.setFillColor(sf::Color(255, 60, 50));
        else if (enemyType == EnemyType::DUELIST) body.setFillColor(sf::Color(255, 50, 220));
        else if (enemyType == EnemyType::DUELIST_PASSIVE) body.setFillColor(sf::Color(70, 230, 255));
        else body.setFillColor(sf::Color(220, 120, 120));
    } else if (attackState == EnemyAttackState::WINDUP) {
        body.setFillColor(sf::Color(255, 130, 40));  // Оранжевый замах
    } else {
        body.setFillColor(baseColor);
    }

    // При увороте (додже) силуэт становится полупрозрачным фантомом (i-frames)
    if (isDodging) {
        sf::Color dCol = body.getFillColor();
        dCol.a = 110;
        body.setFillColor(dCol);
    }

    window.draw(body);

    // Волна идеального доджа дуэлянта (золотое расширяющееся кольцо)
    if (isPerfectDodged && perfectDodgeEffectTimer > 0.0f) {
        float pRadius = (0.45f - perfectDodgeEffectTimer) * 110.0f + 14.0f;
        sf::CircleShape pWave(pRadius);
        pWave.setOrigin({pRadius, pRadius});
        pWave.setPosition(position);
        pWave.setFillColor(sf::Color(255, 215, 0, static_cast<std::uint8_t>((perfectDodgeEffectTimer / 0.45f) * 160.0f)));
        pWave.setOutlineThickness(2.0f);
        pWave.setOutlineColor(sf::Color(255, 255, 255, 220));
        window.draw(pWave);
    }

    // Энергетический щит защиты / блока бота (Верхний и Нижний блок)
    if (blockType != BlockType::NONE) {
        float shieldX = isFacingLeft ? (position.x - halfW - 6.0f) : (position.x + halfW + 6.0f);
        if (blockType == BlockType::HIGH) {
            // Верхний блок: щит на уровне корпуса и головы
            sf::RectangleShape shieldBar({6.0f, halfH * 2.0f});
            shieldBar.setOrigin({3.0f, halfH * 1.0f});
            shieldBar.setPosition({shieldX, position.y - 4.0f});
            shieldBar.setFillColor(sf::Color(80, 200, 255, 200));
            shieldBar.setOutlineThickness(1.5f);
            shieldBar.setOutlineColor(sf::Color(255, 255, 255, 230));
            window.draw(shieldBar);
        } else {
            // Нижний блок: приземистый щит на уровне ног и пола
            sf::RectangleShape shieldBar({8.0f, halfH * 1.3f});
            shieldBar.setOrigin({4.0f, halfH * 0.4f});
            shieldBar.setPosition({shieldX, position.y + 10.0f});
            shieldBar.setFillColor(sf::Color(255, 150, 50, 200));
            shieldBar.setOutlineThickness(1.5f);
            shieldBar.setOutlineColor(sf::Color(255, 220, 100, 230));
            window.draw(shieldBar);
        }
    }

    // Мишень / визор на теле
    if (enemyType == EnemyType::DUMMY) {
        // Мишень на теле манекена: круглая контрастная мишень с кольцами
        sf::CircleShape targetRing(10.0f * scale);
        targetRing.setOrigin({10.0f * scale, 10.0f * scale});
        targetRing.setPosition(position);
        targetRing.setFillColor(sf::Color(185, 45, 40));
        targetRing.setOutlineThickness(1.5f);
        targetRing.setOutlineColor(sf::Color(255, 255, 255, 220));
        window.draw(targetRing);

        sf::CircleShape targetInner(5.0f * scale);
        targetInner.setOrigin({5.0f * scale, 5.0f * scale});
        targetInner.setPosition(position);
        targetInner.setFillColor(sf::Color(255, 255, 255, 240));
        window.draw(targetInner);

        sf::CircleShape targetDot(2.0f * scale);
        targetDot.setOrigin({2.0f * scale, 2.0f * scale});
        targetDot.setPosition(position);
        targetDot.setFillColor(sf::Color(185, 45, 40));
        window.draw(targetDot);
    } else {
        // Внутренний квадрат-мишень на теле противника
        sf::RectangleShape target({14.0f * scale, 14.0f * scale});
        target.setOrigin({7.0f * scale, 7.0f * scale});
        target.setPosition(position);
        target.setFillColor(sf::Color(40, 40, 50, 160));
        window.draw(target);

        // Простой глаз/визор с динамическим цветом в зависимости от восприятия (патруль/тревога/погоня)
        float eyeX = isFacingLeft ? (position.x - 7.0f * scale) : (position.x + 3.0f * scale);
        sf::RectangleShape eye({4.0f * scale, 4.0f * scale});
        eye.setPosition({eyeX, position.y - 16.0f * scale});
        sf::Color eyeColor = sf::Color(255, 245, 220);
        if (behaviorState == AIBehaviorState::PATROL) {
            eyeColor = sf::Color(60, 220, 255); // Спокойный лазурный визор
        } else if (behaviorState == AIBehaviorState::ALERT) {
            eyeColor = sf::Color(255, 225, 0);   // Золотой визор тревоги
        } else if (behaviorState == AIBehaviorState::CHASE || behaviorState == AIBehaviorState::COMBAT) {
            eyeColor = sf::Color(255, 50, 60);   // Алый визор атаки
        }
        eye.setFillColor(eyeColor);
        window.draw(eye);
    }

    // Отрисовка взмаха атаки
    if (attackState == EnemyAttackState::ATTACKING) {
        sf::RectangleShape slash({attackHitbox.size.x, attackHitbox.size.y});
        slash.setPosition(attackHitbox.position);
        if (enemyType == EnemyType::DUELIST) {
            slash.setFillColor(sf::Color(220, 40, 255, 160));
            slash.setOutlineColor(sf::Color(255, 225, 80, 230));
        } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
            slash.setFillColor(sf::Color(40, 210, 255, 160));
            slash.setOutlineColor(sf::Color(140, 245, 255, 230));
        } else if (enemyType == EnemyType::TANK) {
            slash.setFillColor(sf::Color(255, 45, 30, 170));
            slash.setOutlineColor(sf::Color(255, 210, 60, 230));
        } else {
            slash.setFillColor(sf::Color(180, 185, 195, 160));
            slash.setOutlineColor(sf::Color(230, 235, 245, 220));
        }
        slash.setOutlineThickness(1.5f);
        window.draw(slash);
    }

    // Отрисовка стойки парирования бота
    if (isCounterStanceActive()) {
        sf::CircleShape shield(26.0f * scale);
        shield.setOrigin({26.0f * scale, 26.0f * scale});
        shield.setPosition(position);
        shield.setFillColor(sf::Color(0, 210, 255, 60));
        shield.setOutlineThickness(2.5f);
        shield.setOutlineColor(sf::Color(120, 240, 255, 230));
        window.draw(shield);

        if (_cachedParryText) {
            _cachedParryText->setPosition({position.x - 18.0f, position.y - halfH - 30.0f});
            window.draw(*_cachedParryText);
        }
    }

    // Отрисовка постоянного тренировочного HUD манекена над головой (урон, таймер 5с, фрейм-гэп)
    if (enemyType == EnemyType::DUMMY) {
        float hudW = 136.0f;
        float hudH = 32.0f;
        float hudX = position.x - hudW / 2.0f;
        float hudY = position.y - halfH - 42.0f;

        // Плашка фона HUD
        sf::RectangleShape hudBg({hudW, hudH});
        hudBg.setPosition({hudX, hudY});
        hudBg.setFillColor(sf::Color(16, 20, 28, 230));
        hudBg.setOutlineThickness(1.5f);
        hudBg.setOutlineColor(sf::Color(220, 175, 75, 230));
        window.draw(hudBg);

        // Полоска таймера сброса (5 секунд)
        if (dummyResetTimer > 0.0f) {
            float timerProgress = std::clamp(dummyResetTimer / 5.0f, 0.0f, 1.0f);
            sf::RectangleShape resetBar({(hudW - 4.0f) * timerProgress, 3.0f});
            resetBar.setPosition({hudX + 2.0f, hudY + hudH - 5.0f});
            resetBar.setFillColor(sf::Color(255, 215, 0));
            window.draw(resetBar);
        }

        if (font) {
            if (!_cachedDummyHudText) {
                _cachedDummyHudText.emplace(*font, "", 10);
            }
            // Текст 1: [ МАНЕКЕН ]
            _cachedDummyHudText->setCharacterSize(9);
            _cachedDummyHudText->setString(toSfString("[ МАНЕКЕН ]"));
            _cachedDummyHudText->setFillColor(sf::Color(220, 175, 75));
            _cachedDummyHudText->setPosition({hudX + 5.0f, hudY + 2.0f});
            window.draw(*_cachedDummyHudText);

            // Текст 2: Накопленный урон + таймер сброса (5с) + последний фрейм-гэп
            std::string statsStr = "УРОН: " + std::to_string(static_cast<int>(std::round(dummyAccumDamage)));
            if (dummyResetTimer > 0.0f) {
                char tBuf[16];
                std::snprintf(tBuf, sizeof(tBuf), " (%.1fc)", dummyResetTimer);
                statsStr += tBuf;
            }
            statsStr += " | ГЭП: " + std::to_string(dummyLastGapFrames) + "f";

            _cachedDummyHudText->setCharacterSize(9);
            _cachedDummyHudText->setString(toSfString(statsStr));
            _cachedDummyHudText->setFillColor(dummyResetTimer > 0.0f ? sf::Color(255, 230, 110) : sf::Color(170, 175, 185));
            _cachedDummyHudText->setPosition({hudX + 5.0f, hudY + 14.0f});
            window.draw(*_cachedDummyHudText);

            // Отрисовка вылетающих и затухающих надписей над головой
            if (!_cachedDummyPopupText) {
                _cachedDummyPopupText.emplace(*font, "", 12);
                _cachedDummyPopupText->setStyle(sf::Text::Bold);
            }
            for (const auto& pop : dummyPopups) {
                float alpha = std::clamp((pop.timer / pop.maxTimer) * 255.0f, 0.0f, 255.0f);
                sf::Color c = pop.color;
                c.a = static_cast<std::uint8_t>(alpha);

                sf::Vector2f popPos = position + pop.offset;
                _cachedDummyPopupText->setString(toSfString(pop.text));
                sf::FloatRect bounds = _cachedDummyPopupText->getLocalBounds();
                _cachedDummyPopupText->setOrigin({bounds.size.x / 2.0f, bounds.size.y / 2.0f});

                // Тень текста
                _cachedDummyPopupText->setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(alpha * 0.85f)));
                _cachedDummyPopupText->setPosition({popPos.x + 1.0f, popPos.y + 1.0f});
                window.draw(*_cachedDummyPopupText);

                // Основной цвет
                _cachedDummyPopupText->setFillColor(c);
                _cachedDummyPopupText->setPosition(popPos);
                window.draw(*_cachedDummyPopupText);
            }
        }
        return; // Манекен отрисован, стандартный HUD врагов пропускается
    }

    // 2. Полоска здоровья над противником
    float barW = (enemyType == EnemyType::TANK) ? 70.0f : 50.0f;
    float barH = 5.0f;
    float barX = position.x - barW / 2.0f;
    float barY = position.y - halfH - 14.0f;

    // Телеграф замаха атаки (восклицательный знак над головой)
    if (attackState == EnemyAttackState::WINDUP) {
        sf::CircleShape warn(6.0f);
        warn.setOrigin({6.0f, 6.0f});
        warn.setPosition({position.x, barY - 10.0f});
        warn.setFillColor(sf::Color(255, 45, 45));
        window.draw(warn);

        if (_cachedWarnExText) {
            _cachedWarnExText->setPosition({position.x - 2.5f, barY - 19.0f});
            window.draw(*_cachedWarnExText);
        }
    }

    sf::RectangleShape border({barW + 2.0f, barH + 2.0f});
    border.setPosition({barX - 1.0f, barY - 1.0f});
    border.setFillColor(sf::Color(20, 20, 25, 200));
    border.setOutlineThickness(1.0f);
    border.setOutlineColor(sf::Color(60, 60, 70, 220));
    window.draw(border);

    sf::RectangleShape bg({barW, barH});
    bg.setPosition({barX, barY});
    bg.setFillColor(sf::Color(50, 15, 15));
    window.draw(bg);

    if (displayedHp > hp && maxHp > 0.0f) {
        float delayW = std::clamp(barW * (displayedHp / maxHp), 0.0f, barW);
        sf::RectangleShape delayBar({delayW, barH});
        delayBar.setPosition({barX, barY});
        delayBar.setFillColor(sf::Color(255, 195, 55));
        window.draw(delayBar);
    }

    if (hp > 0.0f && maxHp > 0.0f) {
        float curW = std::clamp(barW * (hp / maxHp), 0.0f, barW);
        sf::RectangleShape hpBar({curW, barH});
        hpBar.setPosition({barX, barY});
        hpBar.setFillColor(sf::Color(215, 45, 45));
        window.draw(hpBar);
    }

    if (font && _cachedNameText) {
        std::string label = "NORMAL";
        sf::Color labelColor = sf::Color(190, 195, 205);
        if (enemyType == EnemyType::TANK) {
            label = "TANK [HEAVY]";
            labelColor = sf::Color(255, 90, 90);
        } else if (enemyType == EnemyType::DUELIST) {
            label = "DUELIST [AGGRESSIVE]";
            labelColor = sf::Color(215, 120, 255);
        } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
            label = "DUELIST [PASSIVE]";
            labelColor = sf::Color(80, 220, 255);
        }
        if (label != _cachedLabel) {
            _cachedLabel = label;
            _cachedNameText->setString(toSfString(label));
        }
        _cachedNameText->setFillColor(labelColor);
        _cachedNameText->setPosition({barX + 2.0f, barY - 13.0f});
        window.draw(*_cachedNameText);

        // Индикатор паниша стана
        if (punishBannerTimer > 0.0f) {
            if (!_cachedPunishText) {
                _cachedPunishText.emplace(*font, toSfString("! PUNISH !"), 10);
                _cachedPunishText->setStyle(sf::Text::Bold);
                _cachedPunishText->setFillColor(sf::Color(255, 215, 0));
            }
            _cachedPunishText->setPosition({position.x - 24.0f, barY - 26.0f});
            window.draw(*_cachedPunishText);
        }

        // Индикатор уворота
        if (isDodging && _cachedDodgeText) {
            _cachedDodgeText->setPosition({position.x - 18.0f, barY - 26.0f});
            window.draw(*_cachedDodgeText);
        }

        // Индикатор состояния восприятия
        if (!isDead) {
            if (behaviorState == AIBehaviorState::ALERT || alertEffectTimer > 0.0f) {
                // Восклицательный знак тревоги [ ! ] ОБНАРУЖЕН!
                sf::CircleShape alertBadge(8.0f);
                alertBadge.setOrigin({8.0f, 8.0f});
                alertBadge.setPosition({position.x, barY - 22.0f});
                alertBadge.setFillColor(sf::Color(255, 215, 0));
                alertBadge.setOutlineThickness(1.5f);
                alertBadge.setOutlineColor(sf::Color(255, 80, 0));
                window.draw(alertBadge);

                if (_cachedExText) {
                    _cachedExText->setPosition({position.x - 3.0f, barY - 30.0f});
                    window.draw(*_cachedExText);
                }
                if (_cachedAlertText) {
                    _cachedAlertText->setPosition({position.x - 28.0f, barY - 42.0f});
                    window.draw(*_cachedAlertText);
                }
            } else if (behaviorState == AIBehaviorState::PATROL) {
                if (_cachedPatText) {
                    _cachedPatText->setPosition({barX + barW - 48.0f, barY - 13.0f});
                    window.draw(*_cachedPatText);
                }
            } else if (behaviorState == AIBehaviorState::CHASE) {
                if (lostSightTimer < EnemyConfig::get().perception.lostSightTimeout - 0.5f) {
                    if (_cachedSearchBadge) {
                        _cachedSearchBadge->setPosition({barX + barW - 46.0f, barY - 13.0f});
                        window.draw(*_cachedSearchBadge);
                    }
                } else {
                    if (_cachedChaseBadge) {
                        _cachedChaseBadge->setPosition({barX + barW - 50.0f, barY - 13.0f});
                        window.draw(*_cachedChaseBadge);
                    }
                }
            }
        }

        if (isSmartAI && !currentComboName.empty() && _cachedAiBanner) {
            if (currentComboName != _cachedComboName) {
                _cachedComboName = currentComboName;
                _cachedAiBanner->setString(toSfString(currentComboName));
            }
            _cachedAiBanner->setPosition({position.x - 75.0f, barY - 28.0f});
            window.draw(*_cachedAiBanner);
        }
    }
}

