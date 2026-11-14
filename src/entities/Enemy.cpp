#include "entities/Enemy.h"
#include "world/Arena.h"
#include <iostream>

Enemy::Enemy(sf::Vector2f spawnPos)
    : position(spawnPos), spawnPosition(spawnPos)
{
}

sf::FloatRect Enemy::getBounds() const {
    return sf::FloatRect({position.x - HALF_W, position.y - HALF_H}, 
                         {HALF_W * 2.0f, HALF_H * 2.0f});
}

bool Enemy::takeDamage(float amount, float knockX, float knockY, float stunTime) {
    if (isDead) return false;

    hp -= amount;
    hurtTimer = stunTime;
    velocity.x = knockX;
    velocity.y = knockY;
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackCooldown = 1.6f;
    aiComboState = AIComboState::IDLE;
    currentComboName = "";
    isAirSuspended = false;
    counterStanceTimer = 0.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});

    if (hp <= 0.0f) {
        hp = 0.0f;
        isDead = true;
        respawnTimer = 2.0f;
    }
    return true;
}

void Enemy::reset() {
    position = spawnPosition;
    velocity = {0.0f, 0.0f};
    hp = maxHp;
    displayedHp = maxHp;
    isDead = false;
    hurtTimer = 0.0f;
    diveSlamBounce = false;
    diveSlamDir = 1.0f;
    respawnTimer = 0.0f;
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
    currentHbw = 55.0f;
    currentHbh = 42.0f;
    currentHbyOffset = -18.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
}

void Enemy::triggerAttack() {
    if (isDead || hurtTimer > 0.0f || attackState != EnemyAttackState::IDLE) return;
    const auto& ecfg = EnemyConfig::get();
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = ecfg.stats.dummyWindup; // Замах перед ударом
    attackHasHit = false;
    currentAttackDamage = ecfg.stats.dummyDamage;
    currentAttackKnockX = isFacingLeft ? -ecfg.stats.dummyKnockX : ecfg.stats.dummyKnockX;
    currentAttackKnockY = ecfg.stats.dummyKnockY;
    currentAttackStun = ecfg.stats.dummyStun;
    currentAttackIsDiveSlam = false;
    currentHbw = ecfg.stats.dummyHbw;
    currentHbh = ecfg.stats.dummyHbh;
    currentHbyOffset = ecfg.stats.dummyHbyOffset;
}

void Enemy::triggerCounterStance() {
    if (isDead || hurtTimer > 0.0f || counterStanceTimer > 0.0f || counterCooldownTimer > 0.0f) return;
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

void Enemy::triggerComboSequence(int forcedStarter) {
    if (isDead || hurtTimer > 0.0f || aiComboState != AIComboState::IDLE) return;

    if (forcedStarter >= 0) {
        currentStarterType = static_cast<AIStarterType>(forcedStarter % 3);
    } else {
        currentStarterType = static_cast<AIStarterType>(nextStarterChoice % 3);
        nextStarterChoice = (nextStarterChoice + 1) % 3;
    }

    aiComboState = AIComboState::APPROACH;
    aiStateTimer = 0.0f;
    attackCooldown = 0.0f;
    attackHasHit = false;

    switch (currentStarterType) {
    case AIStarterType::STARTER_1F2D1:
        currentComboName = "AI: 1-f2-d1 (DIAGONAL LAUNCHER)";
        break;
    case AIStarterType::STARTER_112:
        currentComboName = "AI: 1-1-2 (RAPID COMBO LAUNCHER)";
        break;
    case AIStarterType::STARTER_21DB1:
        currentComboName = "AI: 2-1-db1 (REVERSE SWEEP LAUNCHER)";
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

void Enemy::stagger(float duration) {
    hurtTimer = duration;
    dropCombo();
}

void Enemy::update(float dt, const Arena& arena, const sf::Vector2f& playerPos) {
    if (isDead) {
        respawnTimer -= dt;
        if (respawnTimer <= 0.0f) {
            reset();
        }
        return;
    }

    // Плавное догоняние полоски урона
    displayedHp += (hp - displayedHp) * std::min(1.0f, dt * 10.0f);

    if (counterCooldownTimer > 0.0f) counterCooldownTimer -= dt;

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
    } else if (isSmartAI) {
        // =========================================================
        // УМНЫЙ ИИ: СВЯЗКА КОМБО ЧЕРЕЗ ФОРСУ
        // (1f2d1 -> Jump a1adw2 -> Advance +72px -> Jump a1a1adw2)
        // =========================================================
        float dir = isFacingLeft ? -1.0f : 1.0f;
        if (isCounterStanceActive()) {
            velocity.x = 0.0f; // Во время стойки парирования ждем удара игрока
        } else if (aiComboState == AIComboState::IDLE) {
            if (attackCooldown > 0.0f) attackCooldown -= dt;
            if (playerPos.x != 0.0f || playerPos.y != 0.0f) {
                isFacingLeft = (playerPos.x < position.x);
                dir = isFacingLeft ? -1.0f : 1.0f;
                float distToPlayer = std::abs(playerPos.x - position.x);
                float vertDist = std::abs(playerPos.y - position.y);

                const auto& ecfg = EnemyConfig::get();

                // Тактическое использование стойки контр-атаки / парирования Умного ИИ
                if (counterCooldownTimer <= 0.0f && distToPlayer <= ecfg.counter.triggerDistance && vertDist < 50.0f) {
                    triggerCounterStance();
                } else {
                    // АКТИВНОЕ ПРЕСЛЕДОВАНИЕ: ИИ никогда не стоит на месте!
                    // Даже во время отката ИИ преследует игрока и сокращает дистанцию
                    if (distToPlayer > 48.0f) {
                        float stalkSpeed = (attackCooldown > 0.0f) ? 220.0f : 290.0f;
                        velocity.x = dir * stalkSpeed;
                    } else {
                        velocity.x = 0.0f;
                    }

                    // В радиусе атаки автоматически начинаем комбо-серию
                    if (distToPlayer < 260.0f && vertDist < 80.0f && attackCooldown <= 0.0f) {
                        triggerComboSequence();
                    }
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
            if (distToPlayer > targetDist) {
                velocity.x = dir * ecfg.stats.approachSpeed; // Стремительный рывок на сближение
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
                attackStateTimer = ecfg.starter1.hit1.activeTime;
                attackHasHit = false;
                currentAttackDamage = ecfg.starter1.hit1.damage;
                currentAttackKnockX = dir * ecfg.starter1.hit1.knockX;
                currentAttackKnockY = ecfg.starter1.hit1.knockY;
                currentAttackStun = ecfg.starter1.hit1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter1.hit1.hbw; currentHbh = ecfg.starter1.hit1.hbh; currentHbyOffset = ecfg.starter1.hit1.hbyOffset;
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
                // Удар 2: f2 (выпад вперед с сильным толчком)
                aiComboState = AIComboState::LAUNCHER_F2_ACTIVE;
                aiStateTimer = ecfg.starter1.hitF2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.starter1.hitF2.activeTime;
                attackHasHit = false;
                position.x += dir * ecfg.starter1.hitF2.lunge; // Выпад вперед
                currentAttackDamage = ecfg.starter1.hitF2.damage;
                currentAttackKnockX = dir * ecfg.starter1.hitF2.knockX;
                currentAttackKnockY = ecfg.starter1.hitF2.knockY;
                currentAttackStun = ecfg.starter1.hitF2.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter1.hitF2.hbw; currentHbh = ecfg.starter1.hitF2.hbh; currentHbyOffset = ecfg.starter1.hitF2.hbyOffset;
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
                // Удар 3: d1 (рывок и апперкот-лаунчер на высоту прыжка + 13px)
                aiComboState = AIComboState::LAUNCHER_D1_ACTIVE;
                aiStateTimer = ecfg.starter1.hitD1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.starter1.hitD1.activeTime;
                attackHasHit = false;
                position.x += dir * ecfg.starter1.hitD1.lunge; // Рывок вперед
                currentAttackDamage = ecfg.starter1.hitD1.damage;
                currentAttackKnockX = dir * ecfg.starter1.hitD1.knockX;
                currentAttackKnockY = ecfg.starter1.hitD1.knockY; // Запуск вверх на высоту прыжка + 13px!
                currentAttackStun = ecfg.starter1.hitD1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter1.hitD1.hbw; currentHbh = ecfg.starter1.hitD1.hbh; currentHbyOffset = ecfg.starter1.hitD1.hbyOffset;
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
                // Если игрок не подброшен вверх — дропаем комбо
                if (playerPos.y > position.y - 20.0f) {
                    dropCombo();
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
                attackStateTimer = ecfg.starter2.hit1.activeTime;
                attackHasHit = false;
                currentAttackDamage = ecfg.starter2.hit1.damage;
                currentAttackKnockX = dir * ecfg.starter2.hit1.knockX;
                currentAttackKnockY = ecfg.starter2.hit1.knockY;
                currentAttackStun = ecfg.starter2.hit1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter2.hit1.hbw; currentHbh = ecfg.starter2.hit1.hbh; currentHbyOffset = ecfg.starter2.hit1.hbyOffset;
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
                aiComboState = AIComboState::STARTER_112_2ND_1_ACTIVE;
                aiStateTimer = ecfg.starter2.hit2nd1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.starter2.hit2nd1.activeTime;
                attackHasHit = false;
                position.x += dir * ecfg.starter2.hit2nd1.lunge; // Шаг вперед
                currentAttackDamage = ecfg.starter2.hit2nd1.damage;
                currentAttackKnockX = dir * ecfg.starter2.hit2nd1.knockX;
                currentAttackKnockY = ecfg.starter2.hit2nd1.knockY;
                currentAttackStun = ecfg.starter2.hit2nd1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter2.hit2nd1.hbw; currentHbh = ecfg.starter2.hit2nd1.hbh; currentHbyOffset = ecfg.starter2.hit2nd1.hbyOffset;
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
                // Удар 3: 2 (сильный лаунчер вверх)
                aiComboState = AIComboState::STARTER_112_2_ACTIVE;
                aiStateTimer = ecfg.starter2.hit2.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.starter2.hit2.activeTime;
                attackHasHit = false;
                position.x += dir * ecfg.starter2.hit2.lunge; // Выпад
                currentAttackDamage = ecfg.starter2.hit2.damage;
                currentAttackKnockX = dir * ecfg.starter2.hit2.knockX;
                currentAttackKnockY = ecfg.starter2.hit2.knockY; // Лаунчер вверх на высоту прыжка + 13px!
                currentAttackStun = ecfg.starter2.hit2.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter2.hit2.hbw; currentHbh = ecfg.starter2.hit2.hbh; currentHbyOffset = ecfg.starter2.hit2.hbyOffset;
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
                if (playerPos.y > position.y - 20.0f) {
                    dropCombo();
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
                attackStateTimer = ecfg.starter3.hit2.activeTime;
                attackHasHit = false;
                currentAttackDamage = ecfg.starter3.hit2.damage;
                currentAttackKnockX = dir * ecfg.starter3.hit2.knockX;
                currentAttackKnockY = ecfg.starter3.hit2.knockY;
                currentAttackStun = ecfg.starter3.hit2.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter3.hit2.hbw; currentHbh = ecfg.starter3.hit2.hbh; currentHbyOffset = ecfg.starter3.hit2.hbyOffset;
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
                aiComboState = AIComboState::STARTER_21DB1_1_ACTIVE;
                aiStateTimer = ecfg.starter3.hit1.activeTime;
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.starter3.hit1.activeTime;
                attackHasHit = false;
                position.x += dir * ecfg.starter3.hit1.lunge; // Шаг вперед вдогонку
                currentAttackDamage = ecfg.starter3.hit1.damage;
                currentAttackKnockX = dir * ecfg.starter3.hit1.knockX;
                currentAttackKnockY = ecfg.starter3.hit1.knockY;
                currentAttackStun = ecfg.starter3.hit1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter3.hit1.hbw; currentHbh = ecfg.starter3.hit1.hbh; currentHbyOffset = ecfg.starter3.hit1.hbyOffset;
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
                // Рывок за спину игрока
                aiComboState = AIComboState::STARTER_21DB1_DASH_BEHIND;
                aiStateTimer = ecfg.starter3.dashBehindDuration;
                attackState = EnemyAttackState::IDLE;
                // Перемещаемся за спину игрока
                position.x = playerPos.x + dir * ecfg.starter3.dashBehindOffset;
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
                attackStateTimer = ecfg.starter3.hitSweep.activeTime;
                attackHasHit = false;
                currentAttackDamage = ecfg.starter3.hitSweep.damage;
                currentAttackKnockX = ecfg.starter3.hitSweep.knockX; // СТРОГО ВЕРТИКАЛЬНЫЙ ПОДЪЕМ!
                currentAttackKnockY = ecfg.starter3.hitSweep.knockY; // Подъем на высоту прыжка + 13px!
                currentAttackStun = ecfg.starter3.hitSweep.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.starter3.hitSweep.hbw; currentHbh = ecfg.starter3.hitSweep.hbh; currentHbyOffset = ecfg.starter3.hitSweep.hbyOffset;
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
                if (playerPos.y > position.y - 20.0f) {
                    dropCombo();
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
                attackStateTimer = ecfg.air1.hitA1.activeTime;
                attackHasHit = false;
                isAirSuspended = true; // Зависаем в воздухе во время жонглирования!
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                currentAttackDamage = ecfg.air1.hitA1.damage;
                currentAttackKnockX = (currentStarterType == AIStarterType::STARTER_21DB1)
                    ? 0.0f
                    : ((isFacingLeft ? -1.0f : 1.0f) * ecfg.air1.hitA1.knockX);
                currentAttackKnockY = ecfg.air1.hitA1.knockY;
                currentAttackStun = ecfg.air1.hitA1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.air1.hitA1.hbw; currentHbh = ecfg.air1.hitA1.hbh; currentHbyOffset = ecfg.air1.hitA1.hbyOffset;
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
                attackStateTimer = ecfg.air1.hitAdw2.activeTime;
                attackHasHit = false;
                isAirSuspended = false;
                velocity.y = 450.0f; // Стремительный удар вниз
                float curDir = isFacingLeft ? -1.0f : 1.0f;
                if (currentStarterType == AIStarterType::STARTER_21DB1) {
                    velocity.x = 0.0f;
                    currentAttackKnockX = 0.0f; // Вбивание строго вниз на месте!
                } else {
                    float diffX = playerPos.x - position.x;
                    velocity.x = std::clamp(diffX * 4.0f, -250.0f, 250.0f);
                    currentAttackKnockX = curDir * ecfg.air1.hitAdw2.knockX; // Форса по X
                }
                currentAttackDamage = ecfg.air1.hitAdw2.damage;
                currentAttackKnockY = ecfg.air1.hitAdw2.knockY; // Вбивание в пол -> Ground Bounce
                currentAttackStun = ecfg.air1.hitAdw2.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.air1.hitAdw2.hbw; currentHbh = ecfg.air1.hitAdw2.hbh; currentHbyOffset = ecfg.air1.hitAdw2.hbyOffset;
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
            bool grounded = arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, HALF_H, HALF_H);
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
                attackStateTimer = ecfg.air2.hitA1_1.activeTime;
                attackHasHit = false;
                isAirSuspended = true; // Зависаем в воздухе
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                currentAttackDamage = ecfg.air2.hitA1_1.damage;
                currentAttackKnockX = (currentStarterType == AIStarterType::STARTER_21DB1)
                    ? 0.0f
                    : ((isFacingLeft ? -1.0f : 1.0f) * ecfg.air2.hitA1_1.knockX);
                currentAttackKnockY = ecfg.air2.hitA1_1.knockY;
                currentAttackStun = ecfg.air2.hitA1_1.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.air2.hitA1_1.hbw; currentHbh = ecfg.air2.hitA1_1.hbh; currentHbyOffset = ecfg.air2.hitA1_1.hbyOffset;
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
                attackStateTimer = ecfg.air2.hitA1_2.activeTime;
                attackHasHit = false;
                isAirSuspended = true;
                velocity.y = 0.0f;
                velocity.x = 0.0f;
                currentAttackDamage = ecfg.air2.hitA1_2.damage;
                currentAttackKnockX = (currentStarterType == AIStarterType::STARTER_21DB1)
                    ? 0.0f
                    : ((isFacingLeft ? -1.0f : 1.0f) * ecfg.air2.hitA1_2.knockX);
                currentAttackKnockY = ecfg.air2.hitA1_2.knockY;
                currentAttackStun = ecfg.air2.hitA1_2.stun;
                currentAttackIsDiveSlam = false;
                currentHbw = ecfg.air2.hitA1_2.hbw; currentHbh = ecfg.air2.hitA1_2.hbh; currentHbyOffset = ecfg.air2.hitA1_2.hbyOffset;
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
                attackStateTimer = ecfg.air2.hitAdw2.activeTime;
                attackHasHit = false;
                isAirSuspended = false;
                float curDir = isFacingLeft ? -1.0f : 1.0f;
                currentAttackDamage = ecfg.air2.hitAdw2.damage;
                currentAttackKnockX = (currentStarterType == AIStarterType::STARTER_21DB1)
                    ? (curDir * ecfg.air2.finalKnockXVertical)
                    : (curDir * ecfg.air2.hitAdw2.knockX); // Финальный отскок
                currentAttackKnockY = ecfg.air2.hitAdw2.knockY;
                currentAttackStun = ecfg.air2.hitAdw2.stun;
                currentAttackIsDiveSlam = true;
                velocity.y = ecfg.air2.diveSpeedY; // Пикируют вместе вниз
                currentHbw = ecfg.air2.hitAdw2.hbw; currentHbh = ecfg.air2.hitAdw2.hbh; currentHbyOffset = ecfg.air2.hitAdw2.hbyOffset;
            }
            break;
        }

        case AIComboState::AIR_2_ADW2_ACTIVE: {
            const auto& ecfg = EnemyConfig::get();
            aiStateTimer -= dt;
            bool grounded = arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, HALF_H, HALF_H);
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
            }
            break;
        }
        }
    } else {
        // Логика атаки обычного манекена (для тестирования уворотов и идеального уворота)
        if (attackCooldown > 0.0f) attackCooldown -= dt;

        if (playerPos.x != 0.0f || playerPos.y != 0.0f) {
            isFacingLeft = (playerPos.x < position.x);
            float distToPlayer = std::abs(playerPos.x - position.x);
            float vertDist = std::abs(playerPos.y - position.y);
            // Если игрок рядом и на одной высоте, периодически атакуем
            if (distToPlayer < 120.0f && vertDist < 50.0f && attackCooldown <= 0.0f && attackState == EnemyAttackState::IDLE) {
                triggerAttack();
            }
        }

        const auto& ecfg = EnemyConfig::get();
        if (attackState == EnemyAttackState::WINDUP) {
            attackStateTimer -= dt;
            if (attackStateTimer <= 0.0f) {
                attackState = EnemyAttackState::ATTACKING;
                attackStateTimer = ecfg.stats.dummyActiveTime;
                attackHasHit = false;

                currentHbw = ecfg.stats.dummyHbw;
                currentHbh = ecfg.stats.dummyHbh;
                currentHbyOffset = ecfg.stats.dummyHbyOffset;
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
            }
        }
    }

    // Динамическое обновление хитбокса атаки каждый кадр (привязано к актуальной позиции врага)
    if (attackState == EnemyAttackState::ATTACKING) {
        float hbx = isFacingLeft ? (position.x - HALF_W - currentHbw) : (position.x + HALF_W);
        attackHitbox = sf::FloatRect({hbx, position.y + currentHbyOffset}, {currentHbw, currentHbh});
    } else {
        attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    }

    // Гравитация (при жонглировании в воздухе во время стана гравитация ослаблена,
    // при чистом горизонтальном толчке на уровне дэша — враг фиксируется на уровне дэша)
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
    bool isGrounded = arena.overlapsSolid(position.x, position.y + 1.0f, HALF_W, HALF_H, HALF_H);

    // Горизонтальное затухание скорости:
    // На земле или при чистом горизонтальном толчке на уровне дэша (std::abs(velocity.y) <= 1.0f) —
    // быстрое торможение (10.0f), чтобы толчок перемещал ровно на дальность рывка (~115px).
    // При диагональных запусках в воздух (|velocity.y| > 1.0f) — плавное сопротивление (4.0f).
    // Движение по X с коллизией
    float deltaX = velocity.x * dt;
    if (deltaX != 0.0f) {
        float nextX = position.x + deltaX;
        if (!arena.overlapsSolid(nextX, position.y, HALF_W, HALF_H, HALF_H)) {
            position.x = nextX;
        } else {
            velocity.x = 0.0f;
        }
    }

    // Горизонтальное затухание скорости (плавное экспоненциальное трение):
    // На земле или при толчке на уровне дэша — быстрое торможение (10.0f).
    // При диагональных запусках в воздух — плавное сопротивление (4.0f).
    float friction = ((isGrounded && velocity.y >= 0.0f) || (hurtTimer > 0.0f && std::abs(velocity.y) <= 1.0f)) ? 10.0f : 4.0f;
    velocity.x *= std::exp(-friction * dt);
    if (std::abs(velocity.x) < 0.1f) velocity.x = 0.0f;

    // Движение по Y с коллизией
    float deltaY = velocity.y * dt;
    if (deltaY != 0.0f) {
        float nextY = position.y + deltaY;
        if (deltaY > 0.0f) {
            // Падение
            if (arena.overlapsSolid(position.x, nextY, HALF_W, HALF_H, HALF_H)) {
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
                position.y = std::floor((nextY + HALF_H) / TILE_SIZE) * TILE_SIZE - HALF_H - 0.01f;
            } else {
                position.y = nextY;
            }
        } else {
            // Подброс в воздух
            if (arena.overlapsSolid(position.x, nextY, HALF_W, HALF_H, HALF_H)) {
                velocity.y = 0.0f;
            } else {
                position.y = nextY;
            }
        }
    }

}

void Enemy::draw(sf::RenderWindow& window, const sf::Font* font) {
    // 1. Простая и четкая модель манекена в виде квадрата/прямоугольника (32x56)
    sf::RectangleShape body({HALF_W * 2.0f, HALF_H * 2.0f});
    body.setOrigin({HALF_W, HALF_H});
    body.setPosition(position);
    body.setOutlineThickness(2.0f);
    body.setOutlineColor(sf::Color(25, 25, 30, 230));

    if (isDead) {
        float alpha = std::clamp((respawnTimer / 2.0f) * 160.0f, 0.0f, 255.0f);
        body.setFillColor(sf::Color(100, 100, 115, static_cast<std::uint8_t>(alpha)));
        body.setOutlineColor(sf::Color(40, 40, 45, static_cast<std::uint8_t>(alpha)));
        window.draw(body);
        return;
    }

    if (isSmartAI) {
        if (isCounterStanceActive()) {
            body.setFillColor(sf::Color(0, 230, 255));   // Ярко-лазурная стойка парирования бота
            body.setOutlineColor(sf::Color(255, 255, 255));
        } else if (hurtTimer > 0.0f) {
            body.setFillColor(sf::Color(255, 120, 140)); // Вспышка урона
        } else if (attackState == EnemyAttackState::ATTACKING) {
            body.setFillColor(sf::Color(255, 60, 180));  // Неоново-пурпурная фаза атаки
        } else if (attackState == EnemyAttackState::WINDUP) {
            body.setFillColor(sf::Color(255, 130, 40));  // Оранжевый замах
        } else {
            body.setFillColor(sf::Color(110, 45, 150));  // Темно-пурпурный стиль умного ИИ
        }
    } else {
        if (hurtTimer > 0.0f) {
            body.setFillColor(sf::Color(255, 130, 130)); // Вспышка при получении урона
        } else if (attackState == EnemyAttackState::WINDUP) {
            body.setFillColor(sf::Color(255, 105, 50));  // Оранжевый замах перед ударом
        } else if (attackState == EnemyAttackState::ATTACKING) {
            body.setFillColor(sf::Color(255, 185, 70));  // Активная фаза удара
        } else {
            body.setFillColor(sf::Color(220, 75, 55));   // Базовый цвет манекена (красно-оранжевый)
        }
    }
    window.draw(body);

    // Внутренний квадрат-мишень на теле манекена
    sf::RectangleShape target({14.0f, 14.0f});
    target.setOrigin({7.0f, 7.0f});
    target.setPosition(position);
    target.setFillColor(isSmartAI ? sf::Color(170, 50, 220, 220) : sf::Color(150, 40, 25, 210));
    window.draw(target);

    // Простой глаз/визор, указывающий направление взгляда
    float eyeX = isFacingLeft ? (position.x - 7.0f) : (position.x + 3.0f);
    sf::RectangleShape eye({4.0f, 4.0f});
    eye.setPosition({eyeX, position.y - 16.0f});
    eye.setFillColor(isSmartAI ? sf::Color(255, 70, 70) : sf::Color(255, 245, 220));
    window.draw(eye);

    // Отрисовка взмаха атаки
    if (attackState == EnemyAttackState::ATTACKING) {
        sf::RectangleShape slash({attackHitbox.size.x, attackHitbox.size.y});
        slash.setPosition(attackHitbox.position);
        if (isSmartAI) {
            slash.setFillColor(sf::Color(220, 40, 255, 160));
            slash.setOutlineColor(sf::Color(255, 225, 80, 230));
        } else {
            slash.setFillColor(sf::Color(255, 60, 40, 150));
            slash.setOutlineColor(sf::Color(255, 210, 60, 220));
        }
        slash.setOutlineThickness(1.5f);
        window.draw(slash);
    }

    // Отрисовка стойки парирования бота
    if (isCounterStanceActive()) {
        sf::CircleShape shield(26.0f);
        shield.setOrigin({26.0f, 26.0f});
        shield.setPosition(position);
        shield.setFillColor(sf::Color(0, 210, 255, 60));
        shield.setOutlineThickness(2.5f);
        shield.setOutlineColor(sf::Color(120, 240, 255, 230));
        window.draw(shield);

        if (font) {
            sf::Text pText(*font, "PARRY", 11);
            pText.setFillColor(sf::Color(0, 240, 255));
            pText.setPosition({position.x - 18.0f, position.y - HALF_H - 30.0f});
            window.draw(pText);
        }
    }

    // 2. Полоска здоровья над противником
    float barW = 50.0f;
    float barH = 5.0f;
    float barX = position.x - barW / 2.0f;
    float barY = position.y - HALF_H - 14.0f;

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
        std::string label = isSmartAI ? "SMART AI" : "DUMMY";
        sf::Text nameText(*font, toSfString(label), 10);
        nameText.setFillColor(isSmartAI ? sf::Color(255, 120, 240) : sf::Color(200, 200, 210, 180));
        nameText.setPosition({barX + 2.0f, barY - 13.0f});
        window.draw(nameText);

        if (isSmartAI && !currentComboName.empty()) {
            sf::Text aiBanner(*font, toSfString(currentComboName), 12);
            aiBanner.setFillColor(sf::Color(255, 230, 90));
            aiBanner.setPosition({position.x - 75.0f, barY - 28.0f});
            window.draw(aiBanner);
        }
    }
}

