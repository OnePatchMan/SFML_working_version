#include "entities/Enemy.h"
#include "world/Arena.h"
#include "audio/SoundManager.h"
#include "combat/CombatSystem.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

Enemy::Enemy(sf::Vector2f spawnPos, EnemyType type)
    : position(spawnPos), spawnPosition(spawnPos), enemyType(type), patrolStartX(spawnPos.x)
{
    setType(type);
    if (enemyId == "lurker" || enemyId == "burrow") {
        isBurrowed = true;
        isErupting = false;
        hasSurfaceY = false;
    }
}

void Enemy::setEnemyId(const std::string& id) {
    enemyId = id;
    if (enemyId == "lurker" || enemyId == "burrow") {
        isBurrowed = true;
        isErupting = false;
        hasSurfaceY = false;
    }
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
        canCounter = true;
        canBlock = true;
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
        dummyNeedsBlockAfterStun = false;
        dummyBlockHoldTimer = 0.0f;
        if (dummyBlockMode == DummyBlockMode::HIGH) {
            blockType = BlockType::HIGH;
            isBlocking = true;
        } else if (dummyBlockMode == DummyBlockMode::LOW) {
            blockType = BlockType::LOW;
            isBlocking = true;
        } else {
            blockType = BlockType::NONE;
            isBlocking = false;
        }
    } else if (enemyType == EnemyType::BOSS_1) {
        maxHp = 100.0f;
        scale = 3.3f;
        halfW = 120.0f; // 4 тайла ширина
        halfH = 92.0f;  // 3 тайла высота
        stunMultiplier = 0.0f; // Полноценный poise (unflinching)
        knockMultiplier = 0.0f; // Полный иммунитет к форсе!
        damageMultiplier = 1.0f; // Сниженный сбалансированный урон
        hitboxScale = 2.0f;
        windupMultiplier = 1.0f;
        canCounter = false; // "Данный босс не может ставить блок контр атаку и применять уворот"
        canBlock = false;
        canDodge = false;
        aggressiveness = 0.85f;
        intelligence = 0.80f;
        maxComboHits = 4;
        allowAirFollowup = false;
        isSmartAI = false;
        bossComboCooldown = 1.0f;
        bossSpecialAttackCooldown = 3.5f;
        bossSpecialState = Boss1SpecialState::NONE;
        bossWallStunTimer = 0.0f;
    }
    hp = maxHp;
    displayedHp = maxHp;

    if (enemyType == EnemyType::DUELIST || enemyType == EnemyType::DUELIST_PASSIVE) {
        maxGuardHp = 50.0f;
    } else if (enemyType == EnemyType::TANK) {
        maxGuardHp = 90.0f;
    } else if (enemyType == EnemyType::DUMMY) {
        maxGuardHp = 70.0f;
    } else if (enemyType == EnemyType::BOSS_1) {
        maxGuardHp = 9999.0f;
    } else {
        maxGuardHp = 60.0f;
    }
    guardHp = maxGuardHp;
    guardBrokenTimer = 0.0f;
    guardRegenDelay = 0.0f;
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
    if (isBurrowWaiting()) {
        return sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    }
    return sf::FloatRect({position.x - halfW, position.y - halfH}, 
                         {halfW * 2.0f, halfH * 2.0f});
}

void Enemy::triggerBurrowErupt(const sf::Vector2f& targetPos) {
    if (isErupting) return;
    isBurrowed = false; // Выходит на поверхность!
    isErupting = true;
    isEruptTelegraph = false;
    eruptDelayTimer = 0.0f;
    eruptTimer = 0.65f;
    isFacingLeft = (targetPos.x < position.x);
    float dir = isFacingLeft ? -1.0f : 1.0f;
    if (hasSurfaceY) {
        position.y = surfaceY;
    }
    velocity.y = -420.0f;
    velocity.x = dir * 140.0f;
    isOnGround = false;
    attackState = EnemyAttackState::ATTACKING;
    attackStateTimer = 0.35f;
    attackHasHit = false;
    currentHbw = 54.0f;
    currentHbh = 54.0f;
    currentHbyOffset = -12.0f;
    behaviorState = AIBehaviorState::COMBAT;
    hasDetectedPlayer = true;

    // Скрытень проводит длинное комбо из 3 ударов при выходе из засады!
    isSmartAI = true;
    maxComboHits = 3;
    allowAirFollowup = true;
    triggerComboSequence(static_cast<int>(AIStarterType::STARTER_112), 3);
    currentComboName = "LURKER: 3-HIT AMBUSH COMBO!";

    SoundManager::get().playAttack("1", "", false, 75.0f);
    if (CombatSystem::getActive()) {
        CombatSystem::getActive()->triggerShake(0.16f, 4.5f);
    }
}

void Enemy::cancelBurrowAndStun(float stunTime) {
    isBurrowed = false;
    isErupting = false;
    isEruptTelegraph = false;
    eruptDelayTimer = 0.0f;
    eruptTimer = 0.0f;
    if (hasSurfaceY) {
        position.y = surfaceY;
    }
    isOnGround = true;
    velocity = {0.0f, 0.0f};
    attackState = EnemyAttackState::IDLE;
    attackStateTimer = 0.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    dropCombo();
    hurtTimer = std::max(hurtTimer, stunTime);
    behaviorState = AIBehaviorState::CHASE;
    hasDetectedPlayer = true;
    currentComboName = "PARRIED OUT OF GROUND!";
    SoundManager::get().play("adw2", 60.0f);
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

        // Проверка блока манекена (для тренировки пробития верхнего/нижнего блока и ударов в спину)
        bool defenderFacingLeft = isFacingLeft;
        bool hitFromBehind = isBackHit;
        if (attackerPos != sf::Vector2f(0.0f, 0.0f)) {
            if (defenderFacingLeft && attackerPos.x > position.x + 5.0f) {
                hitFromBehind = true;
            } else if (!defenderFacingLeft && attackerPos.x < position.x - 5.0f) {
                hitFromBehind = true;
            }
        }

        bool canBlockNow = isOnGround && (isBlocking || blockType != BlockType::NONE) && (hurtTimer <= 0.0f || blockStunTimer > 0.0f);
        bool blockDefended = false;
        BlockType curBlock = getBlockType();

        if (canBlockNow && !hitFromBehind) {
            if (hitLevel == HitLevel::MID) {
                blockDefended = true; // Нейтральные атаки блокируются и верхним, и нижним блоком
            } else if (hitLevel == HitLevel::OVERHEAD) {
                if (curBlock == BlockType::HIGH) blockDefended = true;
            } else if (hitLevel == HitLevel::LOW) {
                if (curBlock == BlockType::LOW) blockDefended = true;
            }
        }

        float effectiveAmount = blockDefended ? (amount * 0.20f) : amount;
        float effectiveKnockX = blockDefended ? (knockX * 0.25f) : knockX;
        float effectiveKnockY = blockDefended ? 0.0f : knockY;
        float effectiveStun = blockDefended ? 0.14f : (stunTime * stunMultiplier);
        if (blockDefended) {
            blockStunTimer = 0.14f;
            if (dummyBlockMode == DummyBlockMode::AFTER_HIT) {
                dummyBlockHoldTimer = 1.0f;
            }
        } else {
            if (dummyBlockMode == DummyBlockMode::AFTER_HIT) {
                // Чистое попадание / пробитие: сбрасываем стойку и взводим флаг
                // "нажать контратаку ровно по окончании этого стана"
                isBlocking = false;
                blockType = BlockType::NONE;
                counterStanceTimer = 0.0f;
                dummyBlockHoldTimer = 0.0f;
                dummyNeedsBlockAfterStun = true;
            }
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

        dummyAccumDamage += effectiveAmount;
        dummyResetTimer = 2.0f; // сбрасывается каждые 2.0 секунды после прекращения ударов
        dummyOutOfStunTimer = 0.0f;

        // Всплывающее уведомление над головой (вылетает и плавно исчезает)
        DummyFloatingPopup popup;
        popup.timer = 1.3f;
        popup.maxTimer = 1.3f;
        popup.offset = {0.0f, -45.0f};
        if (blockDefended) {
            popup.text = "BLOCKED: +" + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) / 10) +
                         "." + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) % 10);
            popup.color = sf::Color(100, 220, 255);
        } else if (dummyLastGapFrames == 0) {
            popup.text = "+" + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) / 10) +
                         "." + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) % 10) + " (0f TRUE COMBO!)";
            popup.color = sf::Color(100, 255, 120);
        } else {
            popup.text = "+" + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) / 10) +
                         "." + std::to_string(static_cast<int>(std::round(effectiveAmount * 10.0f)) % 10) + " (GAP: " + std::to_string(dummyLastGapFrames) + "f)";
            popup.color = sf::Color(255, 180, 50);
        }
        dummyPopups.push_back(popup);

        hurtTimer = effectiveStun;
        velocity.x = effectiveKnockX * knockMultiplier;
        velocity.y = effectiveKnockY * knockMultiplier;
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

    bool canBlockNow = isOnGround && (isBlocking || blockType != BlockType::NONE) && (hurtTimer <= 0.0f || blockStunTimer > 0.0f);
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

    if (isBurrowed || isErupting || isEruptTelegraph) {
        cancelBurrowAndStun(stunTime * stunMultiplier);
    }

    if (enemyType == EnemyType::BOSS_1) {
        hp -= amount;
        velocity.x = 0.0f; // Полный иммунитет к форсе
        velocity.y = 0.0f;
        // Во время стана о стену босс остается оглушенным и получает урон
        if (bossWallStunTimer > 0.0f) {
            hurtTimer = bossWallStunTimer;
        } else {
            hurtTimer = 0.0f; // Unflinching poise (не сбивается обычными атаками)
            // Если босс в IDLE и игрок атакует его сверху или вблизи — босс моментально реагирует встречной атакой!
            if (attackState == EnemyAttackState::IDLE && bossComboCooldown <= 0.0f) {
                triggerBossCombo(bossNextComboIndex % 5);
                bossNextComboIndex++;
            }
        }
        if (hp <= 0.0f) {
            hp = 0.0f;
            isDead = true;
            deathFadeTimer = 1.5f;
            respawnTimer = 0.0f;
            if (Arena::getActive()) {
                Arena::getActive()->endBossFight();
            }
        }
        return true;
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
    bossSpecialState = Boss1SpecialState::NONE;
    bossSpecialTimer = 0.0f;
    bossWallStunTimer = 0.0f;
    bossComboCooldown = 1.0f;
    bossSpecialAttackCooldown = 3.5f;
    bossDustWaves.clear();
    if (enemyId == "lurker" || enemyId == "burrow") {
        isBurrowed = true;
        isErupting = false;
        isEruptTelegraph = false;
        eruptDelayTimer = 0.0f;
        eruptTimer = 0.0f;
        tendrilTimer = 0.0f;
        hasSurfaceY = false;
    } else {
        isBurrowed = false;
        isErupting = false;
        isEruptTelegraph = false;
        eruptDelayTimer = 0.0f;
        eruptTimer = 0.0f;
    }
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
    behaviorState = hasDetectedPlayer ? AIBehaviorState::CHASE : AIBehaviorState::PATROL;
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
    behaviorState = hasDetectedPlayer ? AIBehaviorState::CHASE : AIBehaviorState::PATROL;
}

void Enemy::stagger(float duration) {
    hurtTimer = duration;
    dropCombo();
}

void Enemy::update(float dt, const Arena& arena, const sf::Vector2f& playerPos, bool playerIsStunned, bool playerIsAttacking, bool playerIsInvulnerable) {
    if (isDead) {
        if (deathFadeTimer > 0.0f) {
            deathFadeTimer -= dt;
            if (deathFadeTimer < 0.0f) deathFadeTimer = 0.0f;
        }
        return; // Мертвые враги не воскресают автоматически
    }

    if (enemyType == EnemyType::BOSS_1) {
        updateBoss1(dt, arena, playerPos, playerIsStunned, playerIsAttacking);
        return;
    }

    // Логика затаившегося врага (Lurker / Скрытень с усиками)
    if (isBurrowed) {
        if (!hasSurfaceY) {
            surfaceY = position.y;
            hasSurfaceY = true;
        }
        if (!isErupting) {
            // Погружен неподвижно под землю
            position.y = surfaceY + halfH * 1.5f;
            velocity = {0.0f, 0.0f};

            // Фаза задержки / предупреждения перед вылезанием:
            if (isEruptTelegraph) {
                eruptDelayTimer -= dt;
                tendrilTimer += dt * 5.0f; // Бешеное колыхание/вибрация усиков
                if (eruptDelayTimer <= 0.0f) {
                    isEruptTelegraph = false;
                    eruptDelayTimer = 0.0f;
                    triggerBurrowErupt(eruptTargetPos);
                }
                return; // Во время задержки остается под землей
            }

            tendrilTimer += dt;

            // Проверка наступания игрока на усики
            bool hasPlayer = (playerPos.x != 0.0f || playerPos.y != 0.0f);
            if (hasPlayer && !isDead && hurtTimer <= 0.0f) {
                float distToTendrilX = std::abs(playerPos.x - position.x);
                float distToSurfaceY = std::abs(playerPos.y - surfaceY);
                // Зона усиков: ширина tendrilTriggerWidth, по вертикали уровень пола
                if (distToTendrilX <= (tendrilTriggerWidth * 0.5f + 16.0f) && distToSurfaceY <= (halfH + 24.0f)) {
                    if (playerIsInvulnerable) {
                        // Игрок в кадрах неуязвимости (i-frames): усики НЕ трогают его!
                    } else {
                        // Наступил без неуязвимости -> запускаем задержку перед выходом с предупреждением!
                        isEruptTelegraph = true;
                        eruptDelayTimer = 0.42f; // Задержка 0.42с для реакции и парирования контратакой [Q]
                        eruptTargetPos = playerPos;
                        SoundManager::get().play("adw2", 45.0f);
                        if (CombatSystem::getActive()) {
                            CombatSystem::getActive()->triggerShake(0.35f, 2.5f);
                        }
                    }
                }
            }
            return; // Под землей враг неподвижен и не обновляет наземный ИИ
        } else {
            // Враг в фазе вылезания и атаки
            eruptTimer -= dt;
            if (eruptTimer <= 0.0f) {
                isBurrowed = false;
                isErupting = false;
                eruptTimer = 0.0f;
                if (aiComboState == AIComboState::IDLE) {
                    attackState = EnemyAttackState::RECOVERY;
                    attackStateTimer = 0.20f;
                    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                }
            }
        }
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

    if (guardBrokenTimer > 0.0f) {
        guardBrokenTimer -= dt;
        isBlocking = false;
        blockType = BlockType::NONE;
        if (guardBrokenTimer <= 0.0f) {
            guardHp = maxGuardHp * 0.5f;
        }
    } else {
        if (!isBlocking && guardHp < maxGuardHp) {
            if (guardRegenDelay > 0.0f) {
                guardRegenDelay -= dt;
            } else {
                guardHp = std::min(maxGuardHp, guardHp + 30.0f * dt);
            }
        }
    }

    // Логика тренировочного манекена (Dummy): блок, сброс урона через 5 секунд и вылетающие надписи
    if (enemyType == EnemyType::DUMMY) {
        if (isOnGround && !isGuardBroken()) {
            if (dummyBlockMode == DummyBlockMode::HIGH) {
                blockType = BlockType::HIGH;
                isBlocking = true;
            } else if (dummyBlockMode == DummyBlockMode::LOW) {
                blockType = BlockType::LOW;
                isBlocking = true;
            } else if (dummyBlockMode == DummyBlockMode::AUTO) {
                isFacingLeft = (playerPos.x < position.x);
                if (playerPos.y < position.y - 18.0f) {
                    blockType = BlockType::HIGH;
                } else if (playerPos.y > position.y + 15.0f) {
                    blockType = BlockType::LOW;
                } else {
                    blockType = BlockType::HIGH;
                }
                isBlocking = true;
            } else if (dummyBlockMode == DummyBlockMode::AFTER_HIT) {
                // Режим "Контратака после удара / в окно"
                isBlocking = false;
                blockType = BlockType::NONE;
                if (hurtTimer > 0.0f) {
                    // Пока манекен находится в стане от полученного удара — контратака не активна
                    counterStanceTimer = 0.0f;
                    dummyBlockHoldTimer = 0.0f;
                    dummyNeedsBlockAfterStun = true;
                } else {
                    // Стан закончился — манекен жмет контратаку на 1 секунду!
                    if (dummyNeedsBlockAfterStun) {
                        dummyNeedsBlockAfterStun = false;
                        canCounter = true;
                        counterCooldownTimer = 0.0f;
                        triggerCounterStance();
                        counterStanceTimer = 1.0f;
                        dummyBlockHoldTimer = 1.0f;
                        isFacingLeft = (playerPos.x < position.x);
                        currentComboName = "DUMMY: REVERSAL COUNTER!";
                    }

                    if (dummyBlockHoldTimer > 0.0f) {
                        dummyBlockHoldTimer -= dt;
                        if (dummyBlockHoldTimer <= 0.0f) {
                            dummyBlockHoldTimer = 0.0f;
                        }
                    }
                }
            } else {
                blockType = BlockType::NONE;
                isBlocking = false;
            }
        } else {
            // В воздухе блок строго запрещен
            blockType = BlockType::NONE;
            isBlocking = false;
            if (hurtTimer > 0.0f) {
                dummyNeedsBlockAfterStun = true;
            }
        }

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
            counterStanceTimer = 0.0f;
            if (enemyType == EnemyType::DUMMY) {
                attackState = EnemyAttackState::IDLE;
                attackStateTimer = 0.0f;
                attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                currentComboName = "DUMMY: IDLE";
            } else {
                // ПРОМАХ СТОЙКИ ПАРИРОВАНИЯ БОТА (WHIFF)!
                // Игрок не ударил бота во время стойки — бот получает ДОЛГИЙ СТАН!
                const auto& ecfg = getEConfig();
                hurtTimer = ecfg.counter.whiffStun; // Долгий стан бота (1.2с)!
                attackState = EnemyAttackState::IDLE;
                attackStateTimer = 0.0f;
                attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                currentComboName = "BOT COUNTER WHIFFED! [STUNNED]";
            }
        }
    }

    if (hurtTimer > 0.0f) {
        hurtTimer -= dt;
        if (hurtTimer <= 0.0f) {
            hurtTimer = 0.0f;
            if (enemyType == EnemyType::DUMMY && dummyBlockMode == DummyBlockMode::AFTER_HIT && isOnGround && dummyNeedsBlockAfterStun) {
                dummyNeedsBlockAfterStun = false;
                canCounter = true;
                counterCooldownTimer = 0.0f;
                triggerCounterStance();
                counterStanceTimer = 1.0f;
                dummyBlockHoldTimer = 1.0f;
                isBlocking = false;
                blockType = BlockType::NONE;
                isFacingLeft = (playerPos.x < position.x);
                currentComboName = "DUMMY: REVERSAL COUNTER!";
            }
        }
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
            } else if (behaviorState != AIBehaviorState::COMBAT || (aiComboState == AIComboState::IDLE && attackState == EnemyAttackState::IDLE)) {
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
                } else if (behaviorState != AIBehaviorState::COMBAT || (aiComboState == AIComboState::IDLE && attackState == EnemyAttackState::IDLE)) {
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

                // Проверка стены/препятствия впереди (на наклонных лестницах подъем выполняется шагом)
                bool nearSlope = arena.isSlopeNear(position.x, position.y + HALF_H - 4.0f, 96.0f);
                float checkWallX = position.x + pDir * (HALF_W + 12.0f);
                bool wallAhead = !nearSlope && arena.overlapsSolid(checkWallX, position.y - 8.0f, 6.0f, HALF_H - 12.0f, HALF_H - 12.0f);

                // Проверка обрыва/пропасти впереди (учитываем наклонный спуск по ступеням лестницы)
                bool floorAhead = nearSlope ||
                                  arena.overlapsSolid(checkWallX, position.y + HALF_H + 8.0f, 6.0f, 4.0f, 4.0f) ||
                                  arena.overlapsSolid(checkWallX, position.y + HALF_H + 34.0f, 6.0f, 8.0f, 8.0f);
                bool footFloor = nearSlope ||
                                 arena.overlapsSolid(position.x + pDir * HALF_W, position.y + HALF_H + 8.0f, 4.0f, 4.0f, 4.0f) ||
                                 arena.overlapsSolid(position.x + pDir * HALF_W, position.y + HALF_H + 34.0f, 4.0f, 8.0f, 8.0f);

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
                        behaviorState = hasDetectedPlayer ? AIBehaviorState::CHASE : AIBehaviorState::PATROL;
                    }
                }
                if (attackCooldown > 0.0f) attackCooldown -= dt;

                // Если игрок обнаружен (CHASE, ALERT или COMBAT в фазе готовности)
                if (behaviorState == AIBehaviorState::CHASE || behaviorState == AIBehaviorState::ALERT || behaviorState == AIBehaviorState::COMBAT) {
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
                            } else if (canBlock && isOnGround && blockHoldTimer <= 0.0f && playerIsAttacking && distToPlayer < 110.0f) {
                                if (playerPos.y < position.y - 18.0f) {
                                    blockType = BlockType::HIGH; // Атака сверху/в прыжке (оверхэд) -> верхний блок
                                } else if (playerPos.y > position.y + 15.0f) {
                                    blockType = BlockType::LOW;  // Атака снизу -> нижний блок
                                } else {
                                    blockType = (rand() % 100 < 35) ? BlockType::LOW : BlockType::HIGH;
                                }
                                isBlocking = true;
                                blockHoldTimer = 0.35f;
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

                                if (isOnGround && canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f) {
                                    if (playerPos.y < position.y - 18.0f) {
                                        blockType = BlockType::HIGH;
                                    } else if (playerPos.y > position.y + 15.0f) {
                                        blockType = BlockType::LOW;
                                    } else {
                                        blockType = (rand() % 100 < 30) ? BlockType::LOW : BlockType::HIGH;
                                    }
                                    isBlocking = true;
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
                                if (isOnGround && canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f && (rand() % 100 < 20)) {
                                    if (playerPos.y < position.y - 18.0f) {
                                        blockType = BlockType::HIGH;
                                    } else if (playerPos.y > position.y + 15.0f) {
                                        blockType = BlockType::LOW;
                                    } else {
                                        blockType = (rand() % 100 < 30) ? BlockType::LOW : BlockType::HIGH;
                                    }
                                    isBlocking = true;
                                    blockHoldTimer = 0.35f;
                                }
                            }
                        }
                    }
                    // 3. ПОВЕДЕНИЕ ЖИРНОГО ВРАГА (TANK):
                    // Медленный, тяжелая броня, при близком контакте может закрыться блоком
                    else if (enemyType == EnemyType::TANK) {
                        if (isOnGround && distToPlayer <= 65.0f && vertDist < 50.0f && canBlock && blockHoldTimer <= 0.0f && (nextStarterChoice % 2 == 0)) {
                            if (playerPos.y < position.y - 18.0f) {
                                blockType = BlockType::HIGH;
                            } else if (playerPos.y > position.y + 15.0f) {
                                blockType = BlockType::LOW;
                            } else {
                                blockType = (rand() % 100 < 30) ? BlockType::LOW : BlockType::HIGH;
                            }
                            isBlocking = true;
                            blockHoldTimer = 0.45f;
                        }
                        if (distToPlayer > 55.0f) {
                            float stalkSpeed = (attackCooldown > 0.0f) ? (baseSpeed * 0.75f) : baseSpeed;
                            velocity.x = dir * stalkSpeed;
                        } else {
                            if (attackCooldown > 0.0f) {
                                if (isOnGround && canBlock && blockType == BlockType::NONE && blockHoldTimer <= 0.0f) {
                                    if (playerPos.y < position.y - 18.0f) {
                                        blockType = BlockType::HIGH;
                                    } else if (playerPos.y > position.y + 15.0f) {
                                        blockType = BlockType::LOW;
                                    } else {
                                        blockType = (rand() % 100 < 30) ? BlockType::LOW : BlockType::HIGH;
                                    }
                                    isBlocking = true;
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
                    bool onOrNearSlope = arena.isSlopeNear(position.x, position.y + halfH - 4.0f, 96.0f);
                    if (isGrounded && jumpCooldown <= 0.0f && !onOrNearSlope) {
                        float frontX = position.x + dir * (halfW + 24.0f);
                        float farFrontX = position.x + dir * (halfW + 48.0f);
                        // Проверяем реальную вертикальную стену на уровне тела, а не наклонный подъем лестницы:
                        bool blockedAhead = arena.overlapsSolid(frontX, position.y - 10.0f, 8.0f, 16.0f, 16.0f) ||
                                            arena.overlapsSolid(farFrontX, position.y - 10.0f, 8.0f, 16.0f, 16.0f);
                        bool spaceAbove = !arena.overlapsSolid(frontX, position.y - 50.0f, halfW, 16.0f, 16.0f) &&
                                          !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 12.0f, 12.0f);
                        float floorCheckX = position.x + dir * (halfW + 24.0f);
                        // Проверяем наличие пола с учетом наклонного спуска лестницы до 34px вниз:
                        bool hasFloorAhead = arena.overlapsSolid(floorCheckX, position.y + halfH + 12.0f, 8.0f, 4.0f, 4.0f) ||
                                             arena.overlapsSolid(floorCheckX, position.y + halfH + 34.0f, 8.0f, 8.0f, 8.0f);
                        bool noFloorAhead = !hasFloorAhead;
                        bool playerAbove = (targetPos.y < position.y - 45.0f) && (distToPlayer < 70.0f);
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

        default:
            break;
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
                bool onOrNearSlope = arena.isSlopeNear(position.x, position.y + halfH - 4.0f, 96.0f);
                if (isGrounded && jumpCooldown <= 0.0f && !onOrNearSlope) {
                    float frontX = position.x + dir * (halfW + 24.0f);
                    float farFrontX = position.x + dir * (halfW + 48.0f);
                    bool blockedAhead = arena.overlapsSolid(frontX, position.y - 10.0f, 8.0f, 16.0f, 16.0f) ||
                                        arena.overlapsSolid(farFrontX, position.y - 10.0f, 8.0f, 16.0f, 16.0f);
                    bool spaceAbove = !arena.overlapsSolid(frontX, position.y - 50.0f, halfW, 16.0f, 16.0f) &&
                                      !arena.overlapsSolid(position.x, position.y - 64.0f, halfW, 12.0f, 12.0f);
                    float floorCheckX = position.x + dir * (halfW + 24.0f);
                    bool hasFloorAhead = arena.overlapsSolid(floorCheckX, position.y + halfH + 12.0f, 8.0f, 4.0f, 4.0f) ||
                                         arena.overlapsSolid(floorCheckX, position.y + halfH + 34.0f, 8.0f, 8.0f, 8.0f);
                    bool noFloorAhead = !hasFloorAhead;
                    bool playerAbove = (targetPos.y < position.y - 45.0f) && (distToPlayer < 70.0f);
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
        if (isErupting) {
            float hbx = position.x - halfW - 8.0f;
            float hby = position.y - halfH - 12.0f;
            attackHitbox = sf::FloatRect({hbx, hby}, {halfW * 2.0f + 16.0f, halfH * 2.0f + 16.0f});
        } else {
            float hbx = isFacingLeft ? (position.x - halfW - currentHbw) : (position.x + halfW);
            attackHitbox = sf::FloatRect({hbx, position.y + currentHbyOffset}, {currentHbw, currentHbh});
        }
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
            // В первую очередь выталкиваем ВВЕРХ (из пола или наклонной лестницы), не сдвигая врага вбок
            if (!arena.overlapsSolid(position.x, position.y - dist, halfW, halfH, halfH)) {
                position.y -= dist;
                if (velocity.y > 0.0f) velocity.y = 0.0f;
                break;
            }
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
            if (!arena.overlapsSolid(position.x, position.y + dist, halfW, halfH, halfH)) {
                position.y += dist;
                velocity.y = 0.0f;
                break;
            }
        }
    }

    // Движение по X с пошаговой коллизией и поддержкой подъема по ступеням
    float deltaX = velocity.x * dt;
    bool wasGroundedBeforeX = isGrounded || isOnGround;
    if (deltaX != 0.0f) {
        float stepSign = (deltaX > 0.0f) ? 1.0f : -1.0f;
        float remaining = std::abs(deltaX);
        float preCollisionSpeedX = std::abs(velocity.x);
        while (remaining > 0.0f) {
            float step = std::min(remaining, 1.0f) * stepSign;
            float nextX = position.x + step;
            if (!arena.overlapsSolid(nextX, position.y, halfW, halfH, halfH)) {
                // Если противник в режиме патрулирования, он ни в коем случае не должен делать шаг,
                // если его передний край теряет твердый пол (с учетом наклонного спуска до 34px)
                // Во время получения урона / отталкивания (hurtTimer > 0) враг и манекен отлетают свободно без блокировки обрыва!
                if (hurtTimer <= 0.0f && behaviorState == AIBehaviorState::PATROL && attackState == EnemyAttackState::IDLE && enemyType != EnemyType::DUMMY) {
                    bool hasFloor = arena.overlapsSolid(nextX + stepSign * (halfW - 2.0f), position.y + halfH + 8.0f, 4.0f, 4.0f, 4.0f) ||
                                    arena.overlapsSolid(nextX + stepSign * (halfW - 2.0f), position.y + halfH + 34.0f, 4.0f, 8.0f, 8.0f);
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
                // Плавный подъем на полублоки и наклонные лестницы при ходьбе по земле
                bool steppedUp = false;
                if (wasGroundedBeforeX) {
                    constexpr float maxStepUp = 32.0f;
                    for (float up = 1.0f; up <= maxStepUp; up += 1.0f) {
                        if (!arena.overlapsSolid(nextX, position.y - up, halfW, halfH, halfH)) {
                            position.x = nextX;
                            position.y -= up;
                            remaining -= std::abs(step);
                            steppedUp = true;
                            break;
                        }
                    }
                }
                if (steppedUp) continue;

                // Столкновение со стеной!
                if (hurtTimer > 0.0f) {
                    if (auto* act = Arena::getActive()) {
                        act->checkAndBreakDoorOverlap(getBounds(), velocity, true);
                    }
                }
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

    // Плавный спуск по ступеням лестницы при движении по земле
    if (wasGroundedBeforeX && velocity.y >= 0.0f && std::abs(deltaX) > 0.001f) {
        if (!arena.overlapsSolid(position.x, position.y + 1.0f, halfW, halfH, halfH)) {
            constexpr float maxStepDown = 32.0f;
            for (float down = 1.0f; down <= maxStepDown; down += 1.0f) {
                if (arena.overlapsSolid(position.x, position.y + down, halfW, halfH, halfH)) {
                    float low = down - 1.0f;
                    float high = down;
                    for (int iter = 0; iter < 4; ++iter) {
                        float mid = (low + high) * 0.5f;
                        if (arena.overlapsSolid(position.x, position.y + mid, halfW, halfH, halfH)) {
                            high = mid;
                        } else {
                            low = mid;
                        }
                    }
                    position.y += low;
                    isGrounded = true;
                    isOnGround = true;
                    break;
                }
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
                // Находим точную поверхность пола/склона через бинарный поиск
                float low = position.y;
                float high = nextY;
                for (int iter = 0; iter < 6; ++iter) {
                    float mid = (low + high) * 0.5f;
                    if (arena.overlapsSolid(position.x, mid, halfW, halfH, halfH)) {
                        high = mid;
                    } else {
                        low = mid;
                    }
                }
                position.y = low;
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

    // Финальная проверка нахождения на земле и строгий запрет блока в воздухе
    isOnGround = arena.overlapsSolid(position.x, position.y + 1.5f, halfW, halfH, halfH);
    if (!isOnGround) {
        isBlocking = false;
        blockType = BlockType::NONE;
        blockHoldTimer = 0.0f;
    } else {
        if (enemyType == EnemyType::DUMMY && dummyBlockMode == DummyBlockMode::AFTER_HIT && dummyNeedsBlockAfterStun && hurtTimer <= 0.0f) {
            dummyNeedsBlockAfterStun = false;
            canCounter = true;
            counterCooldownTimer = 0.0f;
            triggerCounterStance();
            counterStanceTimer = 1.0f;
            dummyBlockHoldTimer = 1.0f;
            isBlocking = false;
            blockType = BlockType::NONE;
            isFacingLeft = (playerPos.x < position.x);
            currentComboName = "DUMMY: REVERSAL COUNTER!";
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

    if (isBurrowWaiting()) {
        // Отрисовка затаившегося Скрытня: бугорок земли и колышущиеся усики на поверхности
        float groundY = (hasSurfaceY ? surfaceY : position.y) + halfH;
        
        // 1. Землистый бугорок засады
        sf::RectangleShape mound({32.0f, 6.0f});
        mound.setOrigin({16.0f, 6.0f});
        if (isEruptTelegraph) {
            float shakeX = ((rand() % 100) / 50.0f - 1.0f) * 1.5f;
            mound.setPosition({position.x + shakeX, groundY});
            mound.setFillColor(sf::Color(80, 35, 30));
            mound.setOutlineColor(sf::Color(255, 60, 40));
        } else {
            mound.setPosition({position.x, groundY});
            mound.setFillColor(sf::Color(45, 45, 52));
            mound.setOutlineColor(sf::Color(25, 25, 30));
        }
        mound.setOutlineThickness(1.0f);
        window.draw(mound);

        // 2. Колышущиеся усики (3 штуки)
        for (int i = -1; i <= 1; ++i) {
            float xOffset = i * 8.0f;
            float sway = isEruptTelegraph ? 
                (std::sin(tendrilTimer * 8.0f + i * 2.0f) * 7.5f) : 
                (std::sin(tendrilTimer * 3.5f + i * 1.4f) * 5.0f);
            float h = 15.0f + std::abs(i) * -2.0f;
            
            // Стебель усика
            sf::Color stemColor1 = isEruptTelegraph ? sf::Color(160, 40, 40) : sf::Color(100, 70, 130);
            sf::Color stemColor2 = isEruptTelegraph ? sf::Color(220, 80, 50) : sf::Color(140, 95, 180);
            sf::Color stemColor3 = isEruptTelegraph ? sf::Color(255, 120, 70) : sf::Color(180, 120, 230);
            sf::Vertex line[] = {
                sf::Vertex{sf::Vector2f{position.x + xOffset, groundY}, stemColor1},
                sf::Vertex{sf::Vector2f{position.x + xOffset + sway * 0.5f, groundY - h * 0.5f}, stemColor2},
                sf::Vertex{sf::Vector2f{position.x + xOffset + sway, groundY - h}, stemColor3}
            };
            window.draw(line, 3, sf::PrimitiveType::LineStrip);

            // Светящаяся сенсорная луковица на кончике усика
            sf::CircleShape tip(isEruptTelegraph ? 3.0f : 2.2f);
            tip.setOrigin({tip.getRadius(), tip.getRadius()});
            tip.setPosition({position.x + xOffset + sway, groundY - h});
            if (isEruptTelegraph) {
                float pulse = 0.5f + 0.5f * std::sin(tendrilTimer * 12.0f + i * 2.0f);
                tip.setFillColor(sf::Color(255, static_cast<std::uint8_t>(50 + 100 * pulse), 40));
            } else {
                float pulse = 0.7f + 0.3f * std::sin(tendrilTimer * 5.0f + i * 1.5f);
                tip.setFillColor(sf::Color(
                    static_cast<std::uint8_t>(200 * pulse),
                    static_cast<std::uint8_t>(130 * pulse),
                    static_cast<std::uint8_t>(255 * pulse)
                ));
            }
            window.draw(tip);
        }

        // Предупреждающий маркер тревоги [ ! ] во время задержки перед выходом
        if (isEruptTelegraph) {
            sf::CircleShape warnBadge(7.5f);
            warnBadge.setOrigin({7.5f, 7.5f});
            warnBadge.setPosition({position.x, groundY - 26.0f});
            warnBadge.setFillColor(sf::Color(255, 40, 30));
            warnBadge.setOutlineThickness(1.5f);
            warnBadge.setOutlineColor(sf::Color(255, 220, 60));
            window.draw(warnBadge);

            if (_cachedWarnExText) {
                _cachedWarnExText->setPosition({position.x - 2.5f, groundY - 35.0f});
                window.draw(*_cachedWarnExText);
            }
        }
        return; // Тело скрыто под землей
    }

    // ЦВЕТОВЫЕ ОТЛИЧИЯ АРХЕТИПОВ:
    // Подземный (Скрытень) — ярко-оранжевый
    // Обычный — серый
    // Жирный (Танк) — красный
    // Агрессивный дуэлянт — фиолетовый
    // Пассивный дуэлянт — голубой
    sf::Color baseColor;
    if (enemyId == "lurker" || enemyId == "burrow") {
        baseColor = sf::Color(255, 140, 25); // Ярко-оранжевый цвет для Скрытня
    } else if (enemyType == EnemyType::NORMAL) {
        baseColor = sf::Color(135, 140, 150); // Серый цвет
    } else if (enemyType == EnemyType::TANK) {
        baseColor = sf::Color(215, 38, 48);   // Красный цвет
    } else if (enemyType == EnemyType::DUELIST) {
        baseColor = sf::Color(155, 45, 225);  // Фиолетовый цвет (агрессивный)
    } else if (enemyType == EnemyType::DUELIST_PASSIVE) {
        baseColor = sf::Color(45, 185, 245);  // Голубой цвет (пассивный)
    } else if (enemyType == EnemyType::DUMMY) {
        baseColor = sf::Color(220, 175, 75);  // Золотисто-древесный цвет (манекен)
    } else if (enemyType == EnemyType::BOSS_1) {
        baseColor = sf::Color(255, 30, 30);   // Ярко-красный цвет (БОСС 1)
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
        if (enemyId == "lurker" || enemyId == "burrow") body.setFillColor(sf::Color(255, 100, 20));
        else if (enemyType == EnemyType::TANK) body.setFillColor(sf::Color(255, 60, 50));
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

    // Энергетический щит защиты / блока бота (Верхний и Нижний блок, строго на земле)
    if (blockType != BlockType::NONE && isOnGround) {
        float shieldX = isFacingLeft ? (position.x - halfW - 6.0f) : (position.x + halfW + 6.0f);
        if (blockType == BlockType::HIGH) {
            // Верхний блок: щит на уровне корпуса и головы
            sf::RectangleShape shieldBar({6.0f, halfH * 2.0f});
            shieldBar.setOrigin({3.0f, halfH * 1.0f});
            shieldBar.setPosition({shieldX, position.y - 4.0f});
            if (blockStunTimer > 0.0f) {
                shieldBar.setFillColor(sf::Color(255, 255, 255, 240));
                shieldBar.setOutlineColor(sf::Color(80, 220, 255, 255));
            } else {
                shieldBar.setFillColor(sf::Color(80, 200, 255, 200));
                shieldBar.setOutlineColor(sf::Color(255, 255, 255, 230));
            }
            shieldBar.setOutlineThickness(1.5f);
            window.draw(shieldBar);
        } else {
            // Нижний блок: приземистый щит на уровне ног и пола
            sf::RectangleShape shieldBar({8.0f, halfH * 1.3f});
            shieldBar.setOrigin({4.0f, halfH * 0.4f});
            shieldBar.setPosition({shieldX, position.y + 10.0f});
            if (blockStunTimer > 0.0f) {
                shieldBar.setFillColor(sf::Color(255, 255, 255, 240));
                shieldBar.setOutlineColor(sf::Color(255, 180, 0, 255));
            } else {
                shieldBar.setFillColor(sf::Color(255, 150, 50, 200));
                shieldBar.setOutlineColor(sf::Color(255, 220, 100, 230));
            }
            shieldBar.setOutlineThickness(1.5f);
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

    // Предупреждение перед атакой Босса 1 (заменяет замах: красный бейдж [ ! ] и подсвечиваемая зона удара)
    if (enemyType == EnemyType::BOSS_1) {
        bool isTelegraphing = (attackState == EnemyAttackState::WINDUP) ||
                              (bossSpecialState == Boss1SpecialState::BULL_CHARGE_TELEGRAPH) ||
                              (bossSpecialState == Boss1SpecialState::EARTHQUAKE_TELEGRAPH);
        if (isTelegraphing) {
            // 1. Светящийся бейдж предупреждения [ ! ] над головой босса
            sf::CircleShape warnBadge(15.0f);
            warnBadge.setOrigin({15.0f, 15.0f});
            warnBadge.setPosition({position.x, position.y - halfH - 28.0f});
            warnBadge.setFillColor(sf::Color(255, 30, 30));
            warnBadge.setOutlineThickness(2.5f);
            warnBadge.setOutlineColor(sf::Color(255, 230, 40));
            window.draw(warnBadge);

            if (_cachedWarnExText) {
                _cachedWarnExText->setCharacterSize(18);
                _cachedWarnExText->setStyle(sf::Text::Bold);
                _cachedWarnExText->setFillColor(sf::Color::White);
                _cachedWarnExText->setPosition({position.x - 4.0f, position.y - halfH - 40.0f});
                window.draw(*_cachedWarnExText);
            }

            // 2. Зона поражения предупреждения (Danger Telegraph Box)
            float forwardReach = 160.0f;
            float totalW = halfW * 2.0f + forwardReach;
            float hbx = isFacingLeft ? (position.x + halfW - totalW) : (position.x - halfW);
            float hby = position.y - halfH - 65.0f;
            float hbh = halfH * 2.0f + 85.0f;

            if (bossSpecialState == Boss1SpecialState::EARTHQUAKE_TELEGRAPH) {
                hbx = position.x - 220.0f;
                totalW = 440.0f;
                hby = position.y - halfH - 20.0f;
                hbh = halfH * 2.0f + 40.0f;
            }

            sf::RectangleShape warnBox({totalW, hbh});
            warnBox.setPosition({hbx, hby});
            float pulse = 0.6f + 0.4f * std::sin(attackStateTimer * 28.0f);
            warnBox.setFillColor(sf::Color(255, 40, 20, static_cast<std::uint8_t>(45 + 45 * pulse)));
            warnBox.setOutlineThickness(2.0f);
            warnBox.setOutlineColor(sf::Color(255, 220, 40, static_cast<std::uint8_t>(160 + 80 * pulse)));
            window.draw(warnBox);
        }
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

        // Полоска таймера сброса (2 секунды)
        if (dummyResetTimer > 0.0f) {
            float timerProgress = std::clamp(dummyResetTimer / 2.0f, 0.0f, 1.0f);
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

            // Текст 2: Накопленный урон + таймер сброса (2с) + последний фрейм-гэп
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

    // Для Босса 1 не рисуем маленькую полоску ХП над головой (у него большой верхний ХП-бар на экране)
    if (enemyType == EnemyType::BOSS_1) {
        // Отрисовка спец-эффектов Босса 1
        // 1. Отрисовка волн пыли на земле (Earthquake Dust Waves)
        for (const auto& wave : bossDustWaves) {
            if (!wave.active) continue;
            sf::CircleShape dust(14.0f);
            dust.setOrigin({14.0f, 14.0f});
            dust.setPosition({wave.x, wave.y});
            dust.setFillColor(sf::Color(180, 140, 95, static_cast<std::uint8_t>(std::clamp(wave.lifetime * 255.0f, 0.0f, 255.0f))));
            window.draw(dust);

            sf::CircleShape inner(8.0f);
            inner.setOrigin({8.0f, 8.0f});
            inner.setPosition({wave.x, wave.y});
            inner.setFillColor(sf::Color(240, 200, 140, static_cast<std::uint8_t>(std::clamp(wave.lifetime * 255.0f, 0.0f, 255.0f))));
            window.draw(inner);
        }

        // 2. Звездочки стана при ударе о стену
        if (isBossWallStunned()) {
            static float stunStarAngle = 0.0f;
            stunStarAngle += 8.0f;
            for (int s = 0; s < 4; ++s) {
                float a = (stunStarAngle + s * 90.0f) * 3.14159f / 180.0f;
                sf::CircleShape star(5.0f);
                star.setOrigin({5.0f, 5.0f});
                star.setPosition({position.x + std::cos(a) * 45.0f, position.y - halfH - 22.0f + std::sin(a) * 12.0f});
                star.setFillColor(sf::Color(255, 230, 60));
                star.setOutlineThickness(1.0f);
                star.setOutlineColor(sf::Color(255, 120, 20));
                window.draw(star);
            }
        }
        return;
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

    // Шкала прочности блока (Guard / Poise) над головой врага:
    if ((canBlock || guardHp < maxGuardHp || isGuardBroken()) && maxGuardHp > 0.0f && enemyType != EnemyType::BOSS_1) {
        float gBarW = barW;
        float gBarH = 3.0f;
        float gBarY = barY + barH + 2.0f;

        sf::RectangleShape gBorder({gBarW + 2.0f, gBarH + 2.0f});
        gBorder.setPosition({barX - 1.0f, gBarY - 1.0f});
        gBorder.setFillColor(sf::Color(15, 20, 25, 200));
        gBorder.setOutlineThickness(1.0f);
        gBorder.setOutlineColor(sf::Color(50, 70, 90, 180));
        window.draw(gBorder);

        sf::RectangleShape gBg({gBarW, gBarH});
        gBg.setPosition({barX, gBarY});
        gBg.setFillColor(sf::Color(25, 30, 40));
        window.draw(gBg);

        float curGuardRatio = std::clamp(guardHp / maxGuardHp, 0.0f, 1.0f);
        sf::RectangleShape gBar({gBarW * curGuardRatio, gBarH});
        gBar.setPosition({barX, gBarY});
        if (isGuardBroken()) {
            gBar.setFillColor(sf::Color(255, 60, 60));
        } else if (curGuardRatio < 0.35f) {
            gBar.setFillColor(sf::Color(255, 170, 40));
        } else {
            gBar.setFillColor(sf::Color(60, 185, 245));
        }
        window.draw(gBar);
    }

    if (font && _cachedNameText) {
        std::string label = "NORMAL";
        sf::Color labelColor = sf::Color(190, 195, 205);
        if (enemyId == "lurker" || enemyId == "burrow") {
            label = "LURKER";
            labelColor = sf::Color(255, 160, 40);
        } else if (enemyType == EnemyType::TANK) {
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
            _cachedPunishText->setPosition({position.x - 24.0f, barY - 37.0f});
            window.draw(*_cachedPunishText);
        }

        // Индикатор уворота
        if (isDodging && _cachedDodgeText) {
            _cachedDodgeText->setPosition({position.x - 18.0f, barY - 37.0f});
            window.draw(*_cachedDodgeText);
        }

        // Индикатор состояния восприятия (отображается над именем врага, не перекрывая его)
        if (!isDead) {
            if (behaviorState == AIBehaviorState::ALERT || alertEffectTimer > 0.0f) {
                // Восклицательный знак тревоги [ ! ] ОБНАРУЖЕН!
                sf::CircleShape alertBadge(8.0f);
                alertBadge.setOrigin({8.0f, 8.0f});
                alertBadge.setPosition({position.x, barY - 26.0f});
                alertBadge.setFillColor(sf::Color(255, 215, 0));
                alertBadge.setOutlineThickness(1.5f);
                alertBadge.setOutlineColor(sf::Color(255, 80, 0));
                window.draw(alertBadge);

                if (_cachedExText) {
                    _cachedExText->setPosition({position.x - 3.0f, barY - 34.0f});
                    window.draw(*_cachedExText);
                }
                if (_cachedAlertText) {
                    _cachedAlertText->setPosition({position.x - 28.0f, barY - 46.0f});
                    window.draw(*_cachedAlertText);
                }
            } else if (behaviorState == AIBehaviorState::PATROL) {
                if (_cachedPatText) {
                    _cachedPatText->setPosition({barX + 2.0f, barY - 24.0f});
                    window.draw(*_cachedPatText);
                }
            } else if (behaviorState == AIBehaviorState::CHASE) {
                if (lostSightTimer < EnemyConfig::get().perception.lostSightTimeout - 0.5f) {
                    if (_cachedSearchBadge) {
                        _cachedSearchBadge->setPosition({barX + 2.0f, barY - 24.0f});
                        window.draw(*_cachedSearchBadge);
                    }
                } else {
                    if (_cachedChaseBadge) {
                        _cachedChaseBadge->setPosition({barX + 2.0f, barY - 24.0f});
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

std::vector<EnemyTypeDef> Enemy::scanEnemyFolders(const std::string& baseDir) {
    std::vector<EnemyTypeDef> result;

    std::string resolvedDir = resolveAssetPath(baseDir);
    if (!std::filesystem::exists(resolvedDir)) {
        if (std::filesystem::exists("../" + baseDir)) resolvedDir = "../" + baseDir;
        else if (std::filesystem::exists("assets/sprites/enemies")) resolvedDir = "assets/sprites/enemies";
    }

    std::vector<std::string> discoveredFolders;
    if (std::filesystem::exists(resolvedDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(resolvedDir)) {
            if (entry.is_directory()) {
                discoveredFolders.push_back(entry.path().filename().string());
            }
        }
    }

    // Всегда гарантируем наличие 5 базовых врагов, даже если папки еще пусты
    auto ensureFolderInList = [&](const std::string& name) {
        if (std::find(discoveredFolders.begin(), discoveredFolders.end(), name) == discoveredFolders.end()) {
            discoveredFolders.push_back(name);
        }
    };
    ensureFolderInList("normal");
    ensureFolderInList("tank");
    ensureFolderInList("duelist_aggressive");
    ensureFolderInList("duelist_passive");
    ensureFolderInList("dummy");
    ensureFolderInList("lurker");

    std::sort(discoveredFolders.begin(), discoveredFolders.end());

    for (const auto& folderName : discoveredFolders) {
        EnemyTypeDef def;
        def.id = folderName;

        // Определяем один из 3 основных типов (архетипов)
        if (folderName.find("lurker") != std::string::npos || folderName.find("burrow") != std::string::npos) {
            def.archetype = EnemyArchetype::NORMAL;
            def.legacyType = 0;
            def.scale = 1.0f;
            def.maxHp = 45.0f;
            def.displayName = "Скрытень (Засада)";
        } else if (folderName.find("tank") != std::string::npos || folderName.find("heavy") != std::string::npos) {
            def.archetype = EnemyArchetype::HEAVY;
            def.legacyType = 1;
            def.scale = 1.5f;
            def.maxHp = 110.0f;
            def.displayName = "Танк (Тяжелый)";
        } else if (folderName.find("duelist") != std::string::npos) {
            def.archetype = EnemyArchetype::DUELIST;
            def.scale = 1.0f;
            def.maxHp = 65.0f;
            if (folderName.find("pass") != std::string::npos) {
                def.legacyType = 3;
                def.displayName = "Дуэлянт (Пасс)";
            } else {
                def.legacyType = 2;
                def.displayName = "Дуэлянт (Агр)";
            }
        } else if (folderName.find("dummy") != std::string::npos) {
            def.archetype = EnemyArchetype::DUMMY;
            def.legacyType = 4;
            def.scale = 1.0f;
            def.maxHp = 99999.0f;
            def.displayName = "Манекен (Тренировка)";
        } else {
            def.archetype = EnemyArchetype::NORMAL;
            def.legacyType = 0;
            def.scale = 1.0f;
            def.maxHp = 45.0f;
            def.displayName = "Обычный";
        }

        // Определение ранга: Обычный / Элитный / Босс
        if (folderName.find("boss") != std::string::npos) {
            def.rank = EnemyRank::BOSS;
            def.scale *= 1.35f;
            def.maxHp *= 3.0f;
            def.displayName = "БОСС: " + def.displayName;
        } else if (folderName.find("elite") != std::string::npos) {
            def.rank = EnemyRank::ELITE;
            def.scale *= 1.15f;
            def.maxHp *= 1.6f;
            def.displayName = "Элитный " + def.displayName;
        }

        // Проверяем наличие файла preview.png в папке врага
        std::filesystem::path pPath = std::filesystem::path(resolvedDir) / folderName / "preview.png";
        if (std::filesystem::exists(pPath)) {
            auto tex = std::make_shared<sf::Texture>();
            if (tex->loadFromFile(pPath.string())) {
                tex->setSmooth(true);
                def.previewTexture = tex;
            }
        }

        result.push_back(std::move(def));
    }

    return result;
}

// =========================================================================
//                  РЕАЛИЗАЦИЯ БОССА 1 ("БОСС 1")
// =========================================================================

namespace {
struct BossAttackStep {
    float windupTime = 0.50f;
    float activeTime = 0.16f;
    float recoveryTime = 0.28f;
    float damage = 18.0f;
    float knockX = 350.0f;
    float knockY = -180.0f;
    float stunTime = 0.45f;
    HitLevel hitLevel = HitLevel::MID;
    float hbw = 140.0f;
    float hbh = 90.0f;
    float hbyOffset = 0.0f;
    float lungeSpeed = 60.0f;
    std::string sfx = "heavy";
};

struct BossComboDef {
    std::string name;
    std::vector<BossAttackStep> steps;
};

static const BossComboDef s_bossCombos[5] = {
    // 0: ЗАТЯЖНОЙ ТЯЖЕЛЫЙ СМЭШ (3 удара)
    {
        "БОСС 1: ЗАТЯЖНОЙ СМЭШ [3 УДАРА]",
        {
            {0.65f, 0.18f, 0.28f,  5.0f, 380.0f, -200.0f, 0.45f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 80.0f, "heavy"},
            {0.40f, 0.16f, 0.24f,  6.0f, 350.0f, -140.0f, 0.40f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 90.0f, "heavy"},
            {0.70f, 0.20f, 0.38f,  8.0f, 480.0f, -280.0f, 0.60f, HitLevel::MID, 185.0f, 190.0f, 0.0f, 120.0f, "heavy"}
        }
    },
    // 1: ДВОЙНОЙ КРЕСТОВОЙ РАССЕКАТЕЛЬ (3 удара)
    {
        "БОСС 1: КРЕСТОВОЙ РАССЕКАТЕЛЬ [3 УДАРА]",
        {
            {0.45f, 0.16f, 0.22f,  4.5f, 320.0f, -130.0f, 0.35f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 90.0f, "heavy"},
            {0.42f, 0.16f, 0.22f,  5.5f, 340.0f, -130.0f, 0.38f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 90.0f, "heavy"},
            {0.60f, 0.18f, 0.35f,  7.5f, 440.0f, -180.0f, 0.50f, HitLevel::MID, 185.0f, 190.0f, 0.0f, 140.0f, "heavy"}
        }
    },
    // 2: НЕИСТОВАЯ ЯРОСТЬ БЕРСЕРКА (4 удара)
    {
        "БОСС 1: ЯРОСТЬ БЕРСЕРКА [4 УДАРА]",
        {
            {0.38f, 0.15f, 0.20f,  4.0f, 280.0f, -110.0f, 0.32f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 75.0f, "heavy"},
            {0.32f, 0.15f, 0.18f,  4.0f, 280.0f, -110.0f, 0.32f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 75.0f, "heavy"},
            {0.36f, 0.16f, 0.22f,  5.5f, 320.0f, -130.0f, 0.38f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 85.0f, "heavy"},
            {0.65f, 0.20f, 0.38f,  8.0f, 480.0f, -250.0f, 0.55f, HitLevel::MID, 185.0f, 190.0f, 0.0f, 110.0f, "heavy"}
        }
    },
    // 3: СОКРУШИТЕЛЬ ПОЗЫ (2 удара)
    {
        "БОСС 1: СОКРУШИТЕЛЬ ПОЗЫ [2 УДАРА]",
        {
            {0.85f, 0.22f, 0.35f,  8.5f, 520.0f, -320.0f, 0.65f, HitLevel::MID, 185.0f, 190.0f, 0.0f, 130.0f, "heavy"},
            {0.42f, 0.18f, 0.30f,  6.0f, 360.0f, -160.0f, 0.40f, HitLevel::MID, 175.0f, 190.0f, 0.0f,  90.0f, "heavy"}
        }
    },
    // 4: СМЕРЧ УДАРОВ (4 удара)
    {
        "БОСС 1: СМЕРЧ УДАРОВ [4 УДАРА]",
        {
            {0.36f, 0.16f, 0.20f,  4.0f, 290.0f, -110.0f, 0.32f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 80.0f, "heavy"},
            {0.42f, 0.16f, 0.20f,  5.0f, 310.0f, -120.0f, 0.35f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 85.0f, "heavy"},
            {0.40f, 0.16f, 0.22f,  6.0f, 340.0f, -140.0f, 0.38f, HitLevel::MID, 175.0f, 190.0f, 0.0f, 90.0f, "heavy"},
            {0.62f, 0.22f, 0.40f,  8.0f, 500.0f, -260.0f, 0.58f, HitLevel::MID, 185.0f, 190.0f, 0.0f, 120.0f, "heavy"}
        }
    }
};
}

void Enemy::triggerBossBullCharge() {
    bossSpecialState = Boss1SpecialState::BULL_CHARGE_TELEGRAPH;
    bossSpecialTimer = 0.6f;
    velocity = {0.0f, 0.0f};
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = 0.6f;
    currentComboName = "БОСС 1: СОКРУШИТЕЛЬНЫЙ РАЗБЕГ!";
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    SoundManager::get().play("heavy");
}

void Enemy::triggerBossEarthquake() {
    bossSpecialState = Boss1SpecialState::EARTHQUAKE_TELEGRAPH;
    bossSpecialTimer = 0.8f;
    velocity = {0.0f, 0.0f};
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = 0.8f;
    currentComboName = "БОСС 1: ДРОЖЬ ЗЕМЛИ (УДАР ПО ПОЛУ)!";
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    SoundManager::get().play("heavy");
}

void Enemy::triggerBossCombo(int comboIndex) {
    int idx = std::clamp(comboIndex, 0, 4);
    bossNextComboIndex = idx;
    bossCurrentComboHit = 0;
    bossComboTotalHits = static_cast<int>(s_bossCombos[idx].steps.size());
    currentComboName = s_bossCombos[idx].name;

    const auto& step = s_bossCombos[idx].steps[0];
    attackState = EnemyAttackState::WINDUP;
    attackStateTimer = step.windupTime;
    currentAttackDamage = step.damage;
    currentAttackKnockX = step.knockX;
    currentAttackKnockY = step.knockY;
    currentAttackStun = step.stunTime;
    currentAttackHitLevel = step.hitLevel;
    currentHbw = std::max(step.hbw, 175.0f);
    currentHbh = halfH * 2.0f + 85.0f;
    currentHbyOffset = -65.0f;
    attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
    attackHasHit = false;
}

void Enemy::updateBossComboSequence(float dt, const sf::Vector2f& playerPos) {
    if (bossNextComboIndex < 0 || bossNextComboIndex >= 5) bossNextComboIndex = 0;
    const auto& combo = s_bossCombos[bossNextComboIndex];
    if (bossCurrentComboHit < 0 || bossCurrentComboHit >= static_cast<int>(combo.steps.size())) {
        attackState = EnemyAttackState::IDLE;
        bossComboCooldown = 2.0f;
        currentComboName = "";
        return;
    }

    const auto& step = combo.steps[bossCurrentComboHit];
    float dir = isFacingLeft ? -1.0f : 1.0f;

    if (attackState == EnemyAttackState::WINDUP) {
        attackStateTimer -= dt;
        isFacingLeft = (playerPos.x < position.x);
        velocity.x = 0.0f;

        if (attackStateTimer <= 0.0f) {
            attackState = EnemyAttackState::ATTACKING;
            attackStateTimer = step.activeTime;
            attackHasHit = false;
            velocity.x = dir * step.lungeSpeed;

            float forwardReach = std::max(step.hbw, 140.0f);
            float totalW = halfW * 2.0f + forwardReach;
            float hbx = isFacingLeft ? (position.x + halfW - totalW) : (position.x - halfW);
            float hbh = halfH * 2.0f + 85.0f; // Задевает и низ и верх одновременно от пола до 65px над головой босса!
            float hby = position.y - halfH - 65.0f;
            attackHitbox = sf::FloatRect({hbx, hby}, {totalW, hbh});
            SoundManager::get().playAttack(step.sfx, "", false, 90.0f);
        }
    } else if (attackState == EnemyAttackState::ATTACKING) {
        attackStateTimer -= dt;
        float forwardReach = std::max(step.hbw, 140.0f);
        float totalW = halfW * 2.0f + forwardReach;
        float hbx = isFacingLeft ? (position.x + halfW - totalW) : (position.x - halfW);
        float hbh = halfH * 2.0f + 85.0f; // Задевает и низ и верх одновременно!
        float hby = position.y - halfH - 65.0f;
        attackHitbox = sf::FloatRect({hbx, hby}, {totalW, hbh});

        if (attackStateTimer <= 0.0f) {
            attackState = EnemyAttackState::RECOVERY;
            attackStateTimer = step.recoveryTime;
            attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
            velocity.x = 0.0f;
        }
    } else if (attackState == EnemyAttackState::RECOVERY) {
        attackStateTimer -= dt;
        velocity.x = 0.0f;

        if (attackStateTimer <= 0.0f) {
            if (bossCurrentComboHit + 1 < static_cast<int>(combo.steps.size())) {
                // Следующий удар текущего комбо
                bossCurrentComboHit++;
                const auto& nextStep = combo.steps[bossCurrentComboHit];
                attackState = EnemyAttackState::WINDUP;
                attackStateTimer = nextStep.windupTime;
                currentAttackDamage = nextStep.damage;
                currentAttackKnockX = nextStep.knockX;
                currentAttackKnockY = nextStep.knockY;
                currentAttackStun = nextStep.stunTime;
                currentAttackHitLevel = nextStep.hitLevel;
                currentHbw = std::max(nextStep.hbw, 175.0f);
                currentHbh = halfH * 2.0f + 25.0f;
                currentHbyOffset = 0.0f;
                attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                attackHasHit = false;
            } else {
                // Комбо полностью завершено! Входим в кулдаун 2.0 секунды (запрет связки двух комбо)
                attackState = EnemyAttackState::IDLE;
                bossComboCooldown = 2.0f;
                currentComboName = "";
                attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
                velocity.x = 0.0f;
            }
        }
    }
}

void Enemy::updateBoss1(float dt, const Arena& arena, const sf::Vector2f& playerPos, bool playerIsStunned, bool playerIsAttacking) {
    if (isDead) {
        if (deathFadeTimer > 0.0f) {
            deathFadeTimer -= dt;
            if (deathFadeTimer < 0.0f) deathFadeTimer = 0.0f;
        }
        return;
    }

    // 1. Полный иммунитет к форсе от ударов игрока (горизонтальная скорость от атак = 0),
    // но полноценная гравитация и физика падения на пол!
    velocity.x = 0.0f;

    // Проверяем нахождение на полу (overlapsSolid по габаритам босса)
    bool isGrounded = arena.overlapsSolid(position.x, position.y + 2.0f, halfW, halfH, halfH);
    if (!isGrounded) {
        velocity.y += GRAVITY * dt;
        if (velocity.y > 1800.0f) velocity.y = 1800.0f;
    } else {
        if (velocity.y > 0.0f) velocity.y = 0.0f;
    }

    // Движение по вертикали с коллизией
    float deltaY = velocity.y * dt;
    if (deltaY != 0.0f) {
        float nextY = position.y + deltaY;
        if (deltaY > 0.0f) {
            // Падение вниз
            if (arena.overlapsSolid(position.x, nextY, halfW, halfH, halfH)) {
                float low = position.y;
                float high = nextY;
                for (int iter = 0; iter < 6; ++iter) {
                    float mid = (low + high) * 0.5f;
                    if (arena.overlapsSolid(position.x, mid, halfW, halfH, halfH)) {
                        high = mid;
                    } else {
                        low = mid;
                    }
                }
                position.y = low;
                velocity.y = 0.0f;
                isGrounded = true;
            } else {
                position.y = nextY;
            }
        } else {
            if (arena.overlapsSolid(position.x, nextY, halfW, halfH, halfH)) {
                velocity.y = 0.0f;
            } else {
                position.y = nextY;
            }
        }
    }

    isOnGround = isGrounded || arena.overlapsSolid(position.x, position.y + 2.0f, halfW, halfH, halfH);

    // Защита от застревания в полу/стенах (выталкивание наверх)
    if (arena.overlapsSolid(position.x, position.y, halfW, halfH, halfH)) {
        for (float dist = 1.0f; dist <= 64.0f; dist += 1.0f) {
            if (!arena.overlapsSolid(position.x, position.y - dist, halfW, halfH, halfH)) {
                position.y -= dist;
                if (velocity.y > 0.0f) velocity.y = 0.0f;
                isOnGround = true;
                break;
            }
        }
    }

    // 2. Проверка активности боссфайта:
    // Босс НЕ агрится до того, как игрок пересечет триггер и зайдет на арену!
    // Если боссфайт не активирован, босс мирно стоит на месте (IDLE).
    bool fightActive = arena.isBossFightActive();
    float distToPlayer = std::abs(playerPos.x - position.x);

    if (!fightActive && hp >= maxHp && distToPlayer > 220.0f) {
        attackState = EnemyAttackState::IDLE;
        attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        velocity.x = 0.0f;
        isFacingLeft = (playerPos.x < position.x);
        bossSpecialState = Boss1SpecialState::NONE;
        return;
    }

    // 2. Обновление волн пыли от удара о землю
    for (auto& wave : bossDustWaves) {
        if (!wave.active) continue;
        float moveDelta = wave.speed * wave.dir * dt;
        wave.x += moveDelta;
        wave.currentDist += std::abs(moveDelta);
        wave.lifetime -= dt;

        if (arena.isPointSolid(wave.x, wave.y) || wave.currentDist >= wave.maxDist || wave.lifetime <= 0.0f) {
            wave.active = false;
        }
    }
    bossDustWaves.erase(std::remove_if(bossDustWaves.begin(), bossDustWaves.end(),
        [](const GroundDustWave& w) { return !w.active; }), bossDustWaves.end());

    // 3. Состояние оглушения о стену (Wall Stun 2.8с)
    if (bossWallStunTimer > 0.0f) {
        bossWallStunTimer -= dt;
        hurtTimer = bossWallStunTimer;
        attackState = EnemyAttackState::IDLE;
        attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        velocity.x = 0.0f;
        if (bossWallStunTimer <= 0.0f) {
            bossWallStunTimer = 0.0f;
            hurtTimer = 0.0f;
            bossSpecialState = Boss1SpecialState::NONE;
            bossComboCooldown = 1.0f; // Передышка после стана
        }
        return;
    }

    // Хелпер перемещения босса с проверкой твердых блоков и стен (босс не ходит сквозь блоки!)
    auto tryBossMoveX = [&](float dx) -> bool {
        if (std::abs(dx) < 0.0001f) return true;
        float targetX = position.x + dx;
        float checkX = (dx > 0.0f) ? (targetX + halfW) : (targetX - halfW);
        // Проверяем несколько точек по высоте босса (от низа до верха)
        for (float fy = -halfH + 6.0f; fy <= halfH - 6.0f; fy += halfH * 0.45f) {
            if (arena.isPointSolid(checkX, position.y + fy)) {
                return false;
            }
        }
        if (checkX <= 24.0f || checkX >= arena.getWidth() * 64.0f - 24.0f) {
            return false;
        }
        position.x = targetX;
        return true;
    };

    // 4. Обработка специальных атак
    if (bossSpecialState == Boss1SpecialState::BULL_CHARGE_TELEGRAPH) {
        bossSpecialTimer -= dt;
        isFacingLeft = (playerPos.x < position.x);
        velocity.x = 0.0f;
        if (bossSpecialTimer <= 0.0f) {
            bossSpecialState = Boss1SpecialState::BULL_CHARGE_RUN;
            bossSpecialTimer = 4.5f;
            SoundManager::get().playAttack("heavy", "", false, 95.0f);
        }
        return;
    } else if (bossSpecialState == Boss1SpecialState::BULL_CHARGE_RUN) {
        float dir = isFacingLeft ? -1.0f : 1.0f;
        float chargeSpeed = 580.0f;
        velocity.x = dir * chargeSpeed;

        currentAttackDamage = 10.0f; // Сниженный сбалансированный урон
        currentAttackKnockX = dir * 650.0f;
        currentAttackKnockY = -320.0f;
        currentAttackStun = 0.8f;
        currentAttackHitLevel = HitLevel::MID;
        float forwardReach = halfW * 1.6f;
        float totalW = halfW * 2.0f + forwardReach;
        float hbx = isFacingLeft ? (position.x + halfW - totalW) : (position.x - halfW);
        currentHbh = halfH * 2.0f + 85.0f; // Задевает и низ и верх одновременно (от пола до 65px над головой)!
        currentHbyOffset = -65.0f;
        attackHitbox = sf::FloatRect({hbx, position.y - halfH + currentHbyOffset}, {totalW, currentHbh});
        attackState = EnemyAttackState::ATTACKING;

        // Перемещение с проверкой коллизии с блоками
        bool moved = tryBossMoveX(velocity.x * dt);

        bossSpecialTimer -= dt;
        if (!moved || bossSpecialTimer <= 0.0f) {
            // ВРЕЗАНИЕ В СТЕНУ ИЛИ БЛОК! Стан на 2.8 секунды!
            bossSpecialState = Boss1SpecialState::BULL_CHARGE_WALL_STUN;
            bossWallStunTimer = 2.8f;
            hurtTimer = 2.8f;
            velocity.x = 0.0f;
            attackState = EnemyAttackState::IDLE;
            attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
            currentComboName = "БОСС 1: ОГЛУШЕНИЕ О СТЕНУ (2.8с)!";
            SoundManager::get().play("wall_slam", 95.0f);
            if (CombatSystem::getActive()) {
                CombatSystem::getActive()->triggerShake(0.35f, 9.0f);
            }
        }
        return;
    } else if (bossSpecialState == Boss1SpecialState::EARTHQUAKE_TELEGRAPH) {
        bossSpecialTimer -= dt;
        isFacingLeft = (playerPos.x < position.x);
        velocity.x = 0.0f;
        if (bossSpecialTimer <= 0.0f) {
            bossSpecialState = Boss1SpecialState::EARTHQUAKE_SLAM;
            bossSpecialTimer = 0.45f;

            float groundY = position.y + halfH;
            GroundDustWave waveL{position.x - 40.0f, groundY - 14.0f, -1.0f, 420.0f, 0.0f, 380.0f, true, 1.0f};
            GroundDustWave waveR{position.x + 40.0f, groundY - 14.0f,  1.0f, 420.0f, 0.0f, 380.0f, true, 1.0f};
            bossDustWaves.push_back(waveL);
            bossDustWaves.push_back(waveR);

            SoundManager::get().playAttack("heavy", "", false, 95.0f);
            if (CombatSystem::getActive()) {
                CombatSystem::getActive()->triggerShake(0.28f, 7.5f);
            }
        }
        return;
    } else if (bossSpecialState == Boss1SpecialState::EARTHQUAKE_SLAM) {
        bossSpecialTimer -= dt;
        velocity.x = 0.0f;
        if (bossSpecialTimer <= 0.0f) {
            bossSpecialState = Boss1SpecialState::NONE;
            bossComboCooldown = 2.0f;
            currentComboName = "";
        }
        return;
    }

    // 5. Кулдауны между действиями
    if (bossSpecialAttackCooldown > 0.0f) {
        bossSpecialAttackCooldown -= dt;
    }
    if (bossComboCooldown > 0.0f) {
        bossComboCooldown -= dt;
        float distToPlayer = std::abs(playerPos.x - position.x);
        isFacingLeft = (playerPos.x < position.x);
        if (distToPlayer > 180.0f) {
            float dir = isFacingLeft ? -1.0f : 1.0f;
            velocity.x = dir * 110.0f;
            tryBossMoveX(velocity.x * dt);
        }
        attackState = EnemyAttackState::IDLE;
        attackHitbox = sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        return;
    }

    // 6. Выбор следующего действия
    distToPlayer = std::abs(playerPos.x - position.x);
    isFacingLeft = (playerPos.x < position.x);

    if (bossSpecialAttackCooldown <= 0.0f) {
        bossSpecialAttackCooldown = 5.5f + static_cast<float>(rand() % 3);
        int choice = rand() % 2;
        if (choice == 0 && distToPlayer > 220.0f) {
            triggerBossBullCharge();
            return;
        } else {
            triggerBossEarthquake();
            return;
        }
    }

    if (attackState == EnemyAttackState::IDLE) {
        bool playerAboveOrNear = (distToPlayer <= 260.0f) || (playerPos.y < position.y - 15.0f && distToPlayer <= 280.0f);
        if (playerAboveOrNear) {
            triggerBossCombo(bossNextComboIndex % 5);
            bossNextComboIndex++;
        } else {
            float dir = isFacingLeft ? -1.0f : 1.0f;
            velocity.x = dir * 140.0f;
            tryBossMoveX(velocity.x * dt);
        }
    } else {
        updateBossComboSequence(dt, playerPos);
    }
}

