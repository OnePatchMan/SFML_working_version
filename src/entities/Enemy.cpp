#include "entities/Enemy.h"
#include "world/Arena.h"
#include "audio/SoundManager.h"
#include <iostream>

Enemy::Enemy(sf::Vector2f spawnPos, EnemyType type)
    : position(spawnPos), spawnPosition(spawnPos), patrolStartX(spawnPos.x), enemyType(type)
{
    setType(type);
}

void Enemy::setType(EnemyType type) {
    enemyType = type;
    const auto& ecfg = EnemyConfig::get();
    if (enemyType == EnemyType::NORMAL) {
        maxHp = ecfg.normal.hp;
        scale = ecfg.normal.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = ecfg.normal.damageMult;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = ecfg.normal.canCounter;
        canBlock = ecfg.normal.canBlock;
        canDodge = ecfg.normal.canDodge;
        aggressiveness = 0.5f;
        intelligence = 0.5f;
        maxComboHits = 1; // Обычный враг дает 1 комбо
        allowAirFollowup = false;
    } else if (enemyType == EnemyType::TANK) {
        maxHp = ecfg.tank.hp;
        scale = ecfg.tank.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = ecfg.tank.stunMult;
        knockMultiplier = ecfg.tank.knockMult;
        damageMultiplier = ecfg.tank.damageMult;
        hitboxScale = ecfg.tank.hitboxScale;
        windupMultiplier = ecfg.tank.windupMult;
        canCounter = ecfg.tank.canCounter;
        canBlock = ecfg.tank.canBlock;
        canDodge = false;
        aggressiveness = 0.55f;
        intelligence = 0.5f;
        maxComboHits = 2;
        allowAirFollowup = false;
    } else if (enemyType == EnemyType::DUELIST) {
        maxHp = ecfg.duelist.hp;
        scale = ecfg.duelist.scale;
        halfW = HALF_W * scale;
        halfH = HALF_H * scale;
        stunMultiplier = 1.0f;
        knockMultiplier = 1.0f;
        damageMultiplier = ecfg.duelist.damageMult;
        hitboxScale = 1.0f;
        windupMultiplier = 1.0f;
        canCounter = ecfg.duelist.canCounter;
        canBlock = ecfg.duelist.canBlock;
        canDodge = ecfg.duelist.canDodge;
        aggressiveness = ecfg.duelist.aggressiveness;
        intelligence = ecfg.duelist.intelligence;
        maxComboHits = (intelligence >= 0.7f) ? 3 : 2;
        allowAirFollowup = (intelligence >= 0.6f);
    }
    hp = maxHp;
    displayedHp = maxHp;
}

void Enemy::triggerDodge(float dir) {
    if (isDead || hurtTimer > 0.0f || isDodging) return;
    const auto& ecfg = EnemyConfig::get();
    isDodging = true;
    dodgeTimer = 0.24f;
    dodgeCooldownTimer = ecfg.duelist.dodgeCooldown;
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

bool Enemy::takeDamage(float amount, float knockX, float knockY, float stunTime) {
    if (isDead) return false;

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
    if (isBlocking) {
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

    hp -= amount;
    hurtTimer = stunTime * stunMultiplier;
    velocity.x = knockX * knockMultiplier;
    velocity.y = knockY * knockMultiplier;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackCooldown = 1.6f;
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
    isDodging = false;
    dodgeTimer = 0.0f;
    dodgeCooldownTimer = 0.0f;
    isPerfectDodged = false;
    perfectDodgeEffectTimer = 0.0f;
    spacingTimer = 0.0f;
    blockHoldTimer = 0.0f;
    diveSlamBounce = false;
    diveSlamDir = 1.0f;
    respawnTimer = 0.0f;
    deathFadeTimer = 0.0f;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackCooldown = 1.8f;
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
    currentAttackIsDiveSlam = s.isDiveSlam;
    currentHbw = s.hbw * hitboxScale;
    currentHbh = s.hbh * hitboxScale;
    currentHbyOffset = s.hbyOffset * scale;
}

void Enemy::triggerAttack() {
    if (isDead || hurtTimer > 0.0f || attackState != EnemyAttackState::IDLE) return;
    const auto& ecfg = EnemyConfig::get();
    behaviorState = AIBehaviorState::COMBAT;
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = ecfg.stats.dummyWindup * windupMultiplier; // Замах перед ударом
    attackHasHit = false;
    currentAttackDamage = ecfg.stats.dummyDamage * damageMultiplier;
    currentAttackKnockX = isFacingLeft ? -ecfg.stats.dummyKnockX * knockMultiplier : ecfg.stats.dummyKnockX * knockMultiplier;
    currentAttackKnockY = ecfg.stats.dummyKnockY;
    currentAttackStun = ecfg.stats.dummyStun;
    currentAttackIsDiveSlam = false;
    currentHbw = ecfg.stats.dummyHbw * hitboxScale;
    currentHbh = ecfg.stats.dummyHbh * hitboxScale;
    currentHbyOffset = ecfg.stats.dummyHbyOffset * scale;
}

void Enemy::triggerCounterStance() {
    if (!canCounter || isDead || hurtTimer > 0.0f || counterStanceTimer > 0.0f || counterCooldownTimer > 0.0f) return;
    const auto& ecfg = EnemyConfig::get();
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
        currentStarterType = static_cast<AIStarterType>(forcedStarter % 3);
        if (forcedHitLimit > 0) {
            maxComboHits = forcedHitLimit;
            allowAirFollowup = (forcedHitLimit >= 3);
        } else {
            maxComboHits = 3;
            allowAirFollowup = true;
        }
    } else {
        currentStarterType = static_cast<AIStarterType>(nextStarterChoice % 3);
        nextStarterChoice = (nextStarterChoice + 1) % 3;

        if (forcedHitLimit > 0) {
            maxComboHits = forcedHitLimit;
            allowAirFollowup = (forcedHitLimit >= 3);
        } else {
            if (enemyType == EnemyType::NORMAL) {
                // Обычный враг дает 1 комбо
                maxComboHits = 1;
                allowAirFollowup = false;
            } else if (enemyType == EnemyType::TANK) {
                // Жирный враг делает связку из 1-2 ударов
                maxComboHits = 2;
                allowAirFollowup = false;
            } else if (enemyType == EnemyType::DUELIST) {
                // Дуэлянт: интеллект (intelligence) определяет сложность комбо
                if (intelligence >= 0.75f) {
                    maxComboHits = 3;
                    allowAirFollowup = true; // Полный лаунчер и воздушное жонглирование
                } else if (intelligence >= 0.5f) {
                    maxComboHits = (nextStarterChoice % 2 == 0) ? 3 : 2;
                    allowAirFollowup = (intelligence >= 0.6f);
                } else {
                    maxComboHits = 1;
                    allowAirFollowup = false;
                }
            } else {
                // Вариативность серий ИИ по умолчанию:
                static std::mt19937 comboRng(12345);
                std::uniform_int_distribution<int> dist(1, 100);
                int roll = dist(comboRng);
                if (roll <= 25) {
                    maxComboHits = 1;
                    allowAirFollowup = false;
                } else if (roll <= 60) {
                    maxComboHits = 2;
                    allowAirFollowup = false;
                } else {
                    maxComboHits = 3;
                    allowAirFollowup = (dist(comboRng) <= 60);
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
    }
}

void Enemy::dropCombo() {
    const auto& ecfg = EnemyConfig::get();
    aiComboState = AIComboState::IDLE;
    attackState = EnemyAttackState::RECOVERY;
    attackStateTimer = ecfg.stats.whiffRecovery; // Увеличенный стан-лок/восстановление при промахе (Whiff Recovery)
    attackCooldown = 0.65f; // Быстро возвращается к бою (высокая агрессивность)
    currentComboName = "";
    isAirSuspended = false;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
}

void Enemy::finishCombo(bool hitConfirmed) {
    const auto& ecfg = EnemyConfig::get();
    aiComboState = AIComboState::IDLE;
    attackState = EnemyAttackState::RECOVERY;
    attackStateTimer = hitConfirmed ? ecfg.stats.hitRecovery : ecfg.stats.whiffRecovery;
    attackCooldown = hitConfirmed ? 0.45f : 0.65f;
    currentComboName = "";
    isAirSuspended = false;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
}

void Enemy::stagger(float duration) {
    hurtTimer = duration;
    dropCombo();
}

void Enemy::update(float dt, const Arena& arena, const sf::Vector2f& playerPos) {
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
    if (isDodging) {
        dodgeTimer -= dt;
        if (dodgeTimer <= 0.0f) {
            isDodging = false;
            dodgeTimer = 0.0f;
        }
    }
    if (isBlocking && blockHoldTimer > 0.0f) {
        blockHoldTimer -= dt;
        if (blockHoldTimer <= 0.0f) {
            isBlocking = false;
            blockHoldTimer = 0.0f;
        }
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
            const auto& ecfg = EnemyConfig::get();
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
    } else {
        // =========================================================
        // СИСТЕМА ВОСПРИЯТИЯ, ОБНАРУЖЕНИЯ И ПАТРУЛИРОВАНИЯ
        // =========================================================
        if (jumpCooldown > 0.0f) jumpCooldown -= dt;
        if (alertEffectTimer > 0.0f) alertEffectTimer -= dt;

        const auto& pcfg = EnemyConfig::get().perception;
        const auto& ecfg = EnemyConfig::get();

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
            } else if (aiComboState == AIComboState::IDLE) {
                if (attackCooldown > 0.0f) attackCooldown -= dt;

                // Если игрок обнаружен (CHASE или ALERT)
                if (behaviorState == AIBehaviorState::CHASE || behaviorState == AIBehaviorState::ALERT) {
                    sf::Vector2f targetPos = canSeePlayer ? playerPos : lastKnownPlayerPos;
                    isFacingLeft = (targetPos.x < position.x);
                    dir = isFacingLeft ? -1.0f : 1.0f;
                    float distToPlayer = std::abs(targetPos.x - position.x);
                    float vertDist = std::abs(targetPos.y - position.y);

                    // Базовые параметры движения по архетипам
                    float baseSpeed = 220.0f;
                    float spacingDist = 48.0f;
                    if (enemyType == EnemyType::NORMAL) {
                        baseSpeed = ecfg.normal.speed;
                        spacingDist = 48.0f;
                    } else if (enemyType == EnemyType::TANK) {
                        baseSpeed = ecfg.tank.speed;
                        spacingDist = 65.0f;
                    } else if (enemyType == EnemyType::DUELIST) {
                        baseSpeed = ecfg.duelist.speed;
                        spacingDist = ecfg.duelist.spacingDistance;
                    }

                    // 1. ПОВЕДЕНИЕ ДУЭЛЯНТА (DUELIST):
                    // Умеет доджить, контратаковать, держать блок, выжидать (footsies)
                    if (enemyType == EnemyType::DUELIST) {
                        if (distToPlayer <= spacingDist && vertDist < 50.0f) {
                            if (aggressiveness < 0.60f) {
                                // ПАССИВНЫЙ ДУЭЛЯНТ: выжидает в блоке, отступает и ловит на контратаку / додж
                                if (canBlock && !isBlocking && blockHoldTimer <= 0.0f) {
                                    isBlocking = true;
                                    blockHoldTimer = 0.40f;
                                }
                                if (distToPlayer < 75.0f) {
                                    velocity.x = -dir * (baseSpeed * 0.65f); // Отход назад для дистанции
                                } else {
                                    velocity.x = 0.0f;
                                }

                                if (distToPlayer <= ecfg.counter.triggerDistance) {
                                    if (canCounter && counterCooldownTimer <= 0.0f) {
                                        triggerCounterStance();
                                    } else if (canDodge && dodgeCooldownTimer <= 0.0f) {
                                        // Spot dodge на месте или уворот назад в зависимости от интеллекта
                                        float dDir = (intelligence >= 0.7f && distToPlayer < 50.0f) ? 0.0f : -dir;
                                        triggerDodge(dDir);
                                    }
                                }
                            } else {
                                // АГРЕССИВНЫЙ ДУЭЛЯНТ: напористо идет на врага, применяет додж сквозь игрока
                                if (canDodge && dodgeCooldownTimer <= 0.0f && distToPlayer < 65.0f && intelligence >= 0.6f && (nextStarterChoice % 3 == 0)) {
                                    triggerDodge(dir); // Додж вперед сквозь игрока!
                                } else if (canCounter && counterCooldownTimer <= 0.0f && distToPlayer <= ecfg.counter.triggerDistance) {
                                    triggerCounterStance();
                                } else if (distToPlayer > 48.0f) {
                                    velocity.x = dir * baseSpeed;
                                } else {
                                    velocity.x = 0.0f;
                                }
                            }
                        } else {
                            // Обычное преследование
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.8f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        }
                    }
                    // 2. ПОВЕДЕНИЕ ЖИРНОГО ВРАГА (TANK):
                    // Медленный, тяжелая броня, при близком контакте может закрыться блоком
                    else if (enemyType == EnemyType::TANK) {
                        if (distToPlayer <= 65.0f && vertDist < 50.0f && canBlock && blockHoldTimer <= 0.0f && (nextStarterChoice % 2 == 0)) {
                            isBlocking = true;
                            blockHoldTimer = 0.45f;
                        }
                        if (distToPlayer > 55.0f) {
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.75f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        } else {
                            velocity.x = 0.0f;
                        }
                    }
                    // 3. ПОВЕДЕНИЕ ОБЫЧНОГО ВРАГА (NORMAL):
                    else {
                        if (distToPlayer > 48.0f) {
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.8f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        } else {
                            velocity.x = 0.0f;
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
                    float attackTriggerDist = (enemyType == EnemyType::TANK) ? 280.0f : 260.0f;
                    if (distToPlayer < attackTriggerDist && vertDist < 80.0f && attackCooldown <= 0.0f) {
                        behaviorState = AIBehaviorState::COMBAT;
                        triggerComboSequence();
                    }
                }
            }

        switch (aiComboState) {
        case AIComboState::IDLE:
            break;

        case AIComboState::APPROACH: {
            const auto& ecfg = EnemyConfig::get();
            isFacingLeft = (playerPos.x < position.x);
            dir = isFacingLeft ? -1.0f : 1.0f;
            float distToPlayer = std::abs(playerPos.x - position.x);
            float targetDist = (currentStarterType == AIStarterType::STARTER_21DB1) ? 54.0f : 48.0f;
            if (enemyType == EnemyType::TANK) targetDist *= 1.3f;
            if (distToPlayer > targetDist) {
                float curApproach = ecfg.stats.approachSpeed;
                if (enemyType == EnemyType::NORMAL) curApproach = ecfg.normal.approachSpeed;
                else if (enemyType == EnemyType::TANK) curApproach = ecfg.tank.approachSpeed;
                else if (enemyType == EnemyType::DUELIST) curApproach = ecfg.duelist.approachSpeed;
                velocity.x = dir * curApproach; // Стремительный рывок на сближение
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
                }
            }
            break;
        }

        // =========================================================
        // СТАРТЕР 1: 1-f2-d1 (Диагональный лаунчер)
        // =========================================================
        case AIComboState::LAUNCHER_1_WINDUP: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
                currentAttackKnockX = ecfg.starter3.hitSweep.knockX; // СТРОГО ВЕРТИКАЛЬНЫЙ ПОДЪЕМ!
            }
            break;
        }

        case AIComboState::STARTER_21DB1_SWEEP_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
        // ОБЩАЯ ВОЗДУШНАЯ СВЯЗКА (СОВМЕЩЕНИЕ ЧЕРЕЗ ФОРСУ)
        // =========================================================
        case AIComboState::CHASE_JUMP_1: {
            const auto& ecfg = EnemyConfig::get();
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
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_1_A1_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_2_A1_1_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
                if (currentStarterType == AIStarterType::STARTER_21DB1) currentAttackKnockX = 0.0f;
            }
            break;
        }

        case AIComboState::AIR_2_A1_2_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
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
                if (currentStarterType == AIStarterType::STARTER_21DB1) {
                    currentAttackKnockX = curDir * ecfg.air2.finalKnockXVertical;
                }
                currentAttackIsDiveSlam = true;
                velocity.y = ecfg.air2.diveSpeedY; // Пикируют вместе вниз
            }
            break;
        }

        case AIComboState::AIR_2_ADW2_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
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
            const auto& ecfg = EnemyConfig::get();
            aiStateTimer -= dt;
            if (aiStateTimer <= 0.0f) {
                aiComboState = AIComboState::IDLE;
                attackState = EnemyAttackState::IDLE;
                attackCooldown = ecfg.stats.attackCooldown; // Высокая агрессивность (~0.75с между сериями)
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

            if (attackState == EnemyAttackState::IDLE) {
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

        const auto& ecfg = EnemyConfig::get();
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
        while (remaining > 0.0f) {
            float step = std::min(remaining, 1.0f) * stepSign;
            float nextX = position.x + step;
            if (!arena.overlapsSolid(nextX, position.y, halfW, halfH, halfH)) {
                // Если противник в режиме патрулирования, он ни в коем случае не должен делать шаг,
                // если его передний край теряет твердый пол (чтобы не падать вниз на нижний блок!)
                if (behaviorState == AIBehaviorState::PATROL && attackState == EnemyAttackState::IDLE) {
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
                velocity.x = 0.0f;
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
    // 1. Модель противника (с учетом размера архетипа: Танк в 1.5 раза больше)
    sf::RectangleShape body({halfW * 2.0f, halfH * 2.0f});
    body.setOrigin({halfW, halfH});
    body.setPosition(position);
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
    // Дуэлянт — фиолетовый
    sf::Color baseColor;
    if (enemyType == EnemyType::NORMAL) {
        baseColor = sf::Color(135, 140, 150); // Серый цвет
    } else if (enemyType == EnemyType::TANK) {
        baseColor = sf::Color(215, 38, 48);   // Красный цвет
    } else if (enemyType == EnemyType::DUELIST) {
        baseColor = sf::Color(145, 45, 215);  // Фиолетовый цвет
    } else {
        baseColor = isSmartAI ? sf::Color(110, 45, 150) : sf::Color(220, 75, 55);
    }

    if (isCounterStanceActive()) {
        body.setFillColor(sf::Color(0, 230, 255));   // Ярко-лазурная стойка парирования
        body.setOutlineColor(sf::Color(255, 255, 255));
    } else if (hurtTimer > 0.0f) {
        body.setFillColor(sf::Color(255, 130, 145)); // Вспышка при получении урона
    } else if (attackState == EnemyAttackState::ATTACKING) {
        if (enemyType == EnemyType::TANK) body.setFillColor(sf::Color(255, 60, 50));
        else if (enemyType == EnemyType::DUELIST) body.setFillColor(sf::Color(255, 50, 220));
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

    // Энергетический щит защиты / блока бота
    if (isBlocking) {
        sf::RectangleShape shieldBar({6.0f, halfH * 2.2f});
        shieldBar.setOrigin({3.0f, halfH * 1.1f});
        float shieldX = isFacingLeft ? (position.x - halfW - 6.0f) : (position.x + halfW + 6.0f);
        shieldBar.setPosition({shieldX, position.y});
        shieldBar.setFillColor(sf::Color(80, 200, 255, 190));
        shieldBar.setOutlineThickness(1.5f);
        shieldBar.setOutlineColor(sf::Color(255, 255, 255, 230));
        window.draw(shieldBar);
    }

    // Внутренний квадрат-мишень на теле манекена
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

    // Отрисовка взмаха атаки
    if (attackState == EnemyAttackState::ATTACKING) {
        sf::RectangleShape slash({attackHitbox.size.x, attackHitbox.size.y});
        slash.setPosition(attackHitbox.position);
        if (enemyType == EnemyType::DUELIST) {
            slash.setFillColor(sf::Color(220, 40, 255, 160));
            slash.setOutlineColor(sf::Color(255, 225, 80, 230));
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

        if (font) {
            sf::Text pText(*font, "PARRY", 11);
            pText.setFillColor(sf::Color(0, 240, 255));
            pText.setPosition({position.x - 18.0f, position.y - halfH - 30.0f});
            window.draw(pText);
        }
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

        if (font) {
            sf::Text exText(*font, "!", 13);
            exText.setFillColor(sf::Color::White);
            exText.setPosition({position.x - 2.5f, barY - 19.0f});
            window.draw(exText);
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

    if (font) {
        std::string label = "NORMAL";
        sf::Color labelColor = sf::Color(190, 195, 205);
        if (enemyType == EnemyType::TANK) {
            label = "TANK [HEAVY]";
            labelColor = sf::Color(255, 90, 90);
        } else if (enemyType == EnemyType::DUELIST) {
            label = "DUELIST";
            labelColor = sf::Color(215, 120, 255);
        }
        sf::Text nameText(*font, toSfString(label), 10);
        nameText.setFillColor(labelColor);
        nameText.setPosition({barX + 2.0f, barY - 13.0f});
        window.draw(nameText);

        // Индикатор уворота
        if (isDodging) {
            sf::Text dodgeText(*font, toSfString("DODGE!"), 10);
            dodgeText.setFillColor(sf::Color(255, 230, 80));
            dodgeText.setPosition({position.x - 18.0f, barY - 26.0f});
            window.draw(dodgeText);
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

                sf::Text exText(*font, "!", 12);
                exText.setStyle(sf::Text::Bold);
                exText.setFillColor(sf::Color(30, 20, 0));
                exText.setPosition({position.x - 3.0f, barY - 30.0f});
                window.draw(exText);

                sf::Text alertText(*font, toSfString("ОБНАРУЖЕН!"), 9);
                alertText.setFillColor(sf::Color(255, 225, 60));
                alertText.setPosition({position.x - 28.0f, barY - 42.0f});
                window.draw(alertText);
            } else if (behaviorState == AIBehaviorState::PATROL) {
                sf::Text patText(*font, toSfString("[ПАТРУЛЬ]"), 9);
                patText.setFillColor(sf::Color(100, 210, 255, 200));
                patText.setPosition({barX + barW - 48.0f, barY - 13.0f});
                window.draw(patText);
            } else if (behaviorState == AIBehaviorState::CHASE) {
                if (lostSightTimer < EnemyConfig::get().perception.lostSightTimeout - 0.5f) {
                    sf::Text searchBadge(*font, toSfString("[?] ПОИСК"), 9);
                    searchBadge.setFillColor(sf::Color(255, 195, 60));
                    searchBadge.setPosition({barX + barW - 46.0f, barY - 13.0f});
                    window.draw(searchBadge);
                } else {
                    sf::Text chaseBadge(*font, toSfString("[!] ПОГОНЯ"), 9);
                    chaseBadge.setFillColor(sf::Color(255, 80, 80));
                    chaseBadge.setPosition({barX + barW - 50.0f, barY - 13.0f});
                    window.draw(chaseBadge);
                }
            }
        }

        if (isSmartAI && !currentComboName.empty()) {
            sf::Text aiBanner(*font, toSfString(currentComboName), 12);
            aiBanner.setFillColor(sf::Color(255, 230, 90));
            aiBanner.setPosition({position.x - 75.0f, barY - 28.0f});
            window.draw(aiBanner);
        }
    }
}

