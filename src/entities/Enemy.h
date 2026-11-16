#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include "core/Common.h"
#include "entities/EnemyConfig.h"

class Arena;

enum class EnemyAttackState {
    IDLE,
    WINDUP,     // Замах / телеграф удара
    ATTACKING,  // Активная фаза удара
    RECOVERY    // Откат после удара
};

enum class AIBehaviorState {
    PATROL,     // Патрулирование (игрок не обнаружен, спокойное движение по маршруту)
    ALERT,      // Обнаружение / тревога ([!] восклицательный знак, фиксация цели)
    CHASE,      // Погоня за игроком (сокращение дистанции, прыжки по платформам)
    COMBAT      // Бой / комбо-серия (проведение ударов)
};

enum class EnemyType {
    NORMAL,           // Обычный: серый цвет, малое ХП (45), легкая броня, средняя скорость, дает 1 комбо, нет контратаки
    TANK,             // Жирный: красный цвет, больше в 1.5 раза, много ХП (110), тяжелая броня, бьет дальше и больнее, медленный
    DUELIST,          // Агрессивный дуэлянт: фиолетовый цвет, ХП 65, быстрый, длинный зацеп с притягиванием, бешеный напор
    DUELIST_PASSIVE   // Пассивный дуэлянт: голубой цвет, ХП 65, выверенный спейсинг, блок/парирование, невидимость на 1с раз в 7с, паниш станов
};

enum class AIStarterType {
    STARTER_1F2D1,        // 1 -> f2 -> d1 (диагональный выпад и лаунчер)
    STARTER_112,          // 1 -> 1 -> 2 (быстрые джебы и апперкот)
    STARTER_21DB1,        // 2 -> 1 -> db1 (тяжелый замах, рывок за спину и вертикальная подсечка)
    STARTER_GRAPPLE_PULL  // Длинный зацеп с притягиванием (Grapple Pull) -> лаунчер или толчок
};

enum class AIComboState {
    IDLE,
    APPROACH,
    // Стартер 1: Комбо на земле 1f2d1 (Диагональный лаунчер)
    LAUNCHER_1_WINDUP,
    LAUNCHER_1_ACTIVE,
    LAUNCHER_1_RECOVERY,
    LAUNCHER_F2_ACTIVE,
    LAUNCHER_F2_RECOVERY,
    LAUNCHER_D1_ACTIVE,
    LAUNCHER_D1_RECOVERY,

    // Стартер 2: Комбо на земле 112 (Быстрый лаунчер)
    STARTER_112_1_WINDUP,
    STARTER_112_1_ACTIVE,
    STARTER_112_1_RECOVERY,
    STARTER_112_2ND_1_ACTIVE,
    STARTER_112_2ND_1_RECOVERY,
    STARTER_112_2_ACTIVE,
    STARTER_112_2_RECOVERY,

    // Стартер 3: Комбо на земле 21db1 (Обход со спины и вертикальная подсечка)
    STARTER_21DB1_2_WINDUP,
    STARTER_21DB1_2_ACTIVE,
    STARTER_21DB1_2_RECOVERY,
    STARTER_21DB1_1_ACTIVE,
    STARTER_21DB1_1_RECOVERY,
    STARTER_21DB1_DASH_BEHIND,
    STARTER_21DB1_SWEEP_ACTIVE,
    STARTER_21DB1_SWEEP_RECOVERY,

    // Стартер 4: Длинный зацеп с притягиванием и разветвлением (Grapple Pull)
    STARTER_GRAPPLE_WINDUP,
    STARTER_GRAPPLE_ACTIVE,
    STARTER_GRAPPLE_RECOVERY,
    STARTER_GRAPPLE_LAUNCHER_ACTIVE,
    STARTER_GRAPPLE_LAUNCHER_RECOVERY,
    STARTER_GRAPPLE_SHOVE_ACTIVE,
    STARTER_GRAPPLE_SHOVE_RECOVERY,

    // Воздушная ветка (общая для всех стартеров):
    CHASE_JUMP_1,
    AIR_1_A1_ACTIVE,
    AIR_1_A1_RECOVERY,
    AIR_1_ADW2_ACTIVE,
    AIR_1_ADW2_RECOVERY,
    // Шаг 3: Нагон отскочившего противника на земле (компенсация knockX или прыжок на месте при 21db1)
    CHASE_GROUND_ADVANCE,
    // Шаг 4: Второй прыжок и комбо a1a1adw2 (Дайв-слэм)
    CHASE_JUMP_2,
    AIR_2_A1_1_ACTIVE,
    AIR_2_A1_1_RECOVERY,
    AIR_2_A1_2_ACTIVE,
    AIR_2_A1_2_RECOVERY,
    AIR_2_ADW2_ACTIVE,
    AIR_2_ADW2_RECOVERY,
    COMBO_FINISHED
};

class Enemy {
public:
    sf::Vector2f position;
    sf::Vector2f velocity{0.0f, 0.0f};
    float hp = 45.0f;
    float maxHp = 45.0f;
    float displayedHp = 45.0f;
    bool isDead = false;
    float hurtTimer = 0.0f;
    float respawnTimer = 0.0f;
    float deathFadeTimer = 0.0f;
    float spinTimer = 0.0f;         // Таймер вращения вокруг своей оси при попадании булавы
    float spinAngle = 0.0f;         // Текущий угол вращения в градусах
    float spinSpeed = 0.0f;         // Скорость вращения (град/с)
    sf::Vector2f spawnPosition;

    EnemyType enemyType = EnemyType::NORMAL;
    float scale = 1.0f;
    float halfW = 16.0f;
    float halfH = 28.0f;
    float stunMultiplier = 1.0f;
    float knockMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float hitboxScale = 1.0f;
    float windupMultiplier = 1.0f;

    bool canCounter = false;
    bool canBlock = false;
    bool canDodge = false;
    float aggressiveness = 0.65f;
    float intelligence = 0.85f;

    // Параметры уворота / доджа дуэлянта
    bool isDodging = false;
    float dodgeTimer = 0.0f;
    float dodgeCooldownTimer = 0.0f;
    float dodgeDirection = 0.0f; // -1, 1 или 0 (на месте)
    bool isPerfectDodged = false;
    float perfectDodgeEffectTimer = 0.0f;

    // Параметры выжидания и маневрирования (spacing)
    float spacingTimer = 0.0f;
    float blockHoldTimer = 0.0f;

    EnemyAttackState attackState = EnemyAttackState::IDLE;
    float attackStateTimer = 0.0f;
    float attackCooldown = 2.0f;
    sf::FloatRect attackHitbox;
    bool isFacingLeft = true;
    bool attackHasHit = false;
    bool diveSlamBounce = false;
    float diveSlamDir = 1.0f;

    float counterStanceTimer = 0.0f;
    float counterCooldownTimer = 0.0f;

    // Система защиты / блока бота (снижает входящий урон на 80%)
    bool isBlocking = false;
    float blockStunTimer = 0.0f;
    bool isBlockStunned() const { return blockStunTimer > 0.0f; }

    static constexpr float HALF_W = 16.0f;
    static constexpr float HALF_H = 28.0f;
    static constexpr float GRAVITY = 2600.0f;

    bool isSmartAI = false;
    AIComboState aiComboState = AIComboState::IDLE;
    float aiStateTimer = 0.0f;
    std::string currentComboName = "";
    float currentAttackDamage = 2.0f;
    float currentAttackKnockX = 0.0f;
    float currentAttackKnockY = 0.0f;
    float currentAttackStun = 0.18f;
    bool currentAttackIsDiveSlam = false;
    float currentHbw = 55.0f;
    float currentHbh = 42.0f;
    float currentHbyOffset = -18.0f;
    bool isAirSuspended = false;
    AIStarterType currentStarterType = AIStarterType::STARTER_1F2D1;
    int nextStarterChoice = 0;
    int maxComboHits = 3;
    bool allowAirFollowup = true;

    // Система восприятия, патрулирования и прыжков
    AIBehaviorState behaviorState = AIBehaviorState::PATROL;
    float alertTimer = 0.0f;
    float lostSightTimer = 0.0f;
    float patrolWaitTimer = 0.0f;
    float jumpCooldown = 0.0f;
    float patrolStartX = 0.0f;
    bool isPatrolMovingRight = false;
    sf::Vector2f lastKnownPlayerPos{0.0f, 0.0f};
    bool hasDetectedPlayer = false;
    float alertEffectTimer = 0.0f;

    // Параметры невидимости (пассивный дуэлянт: 1 сек раз в 7 сек)
    float invisActiveTimer = 0.0f;
    float invisCooldownTimer = 0.0f;
    bool isInvisible() const { return invisActiveTimer > 0.0f; }

    // Параметры наказания станов (паниш стана игрока)
    float stunPunishCooldown = 0.0f;
    float punishBannerTimer = 0.0f;
    bool isPunishing() const { return punishBannerTimer > 0.0f; }

    // Ветка продолжения зацепа (0 = Sky Launcher, 1 = Heavy Shove)
    int grappleBranchChoice = 0;

    // Таймер бездействия игрока (для агрессивного дуэлянта)
    float playerIdleTimer = 0.0f;

    // Кешированные текстовые элементы UI врага (исключают пересоздание sf::Text каждый кадр)
    mutable std::optional<sf::Text> _cachedNameText;
    mutable std::optional<sf::Text> _cachedDodgeText;
    mutable std::optional<sf::Text> _cachedExText;
    mutable std::optional<sf::Text> _cachedAlertText;
    mutable std::optional<sf::Text> _cachedPatText;
    mutable std::optional<sf::Text> _cachedSearchBadge;
    mutable std::optional<sf::Text> _cachedChaseBadge;
    mutable std::optional<sf::Text> _cachedAiBanner;
    mutable std::optional<sf::Text> _cachedParryText;
    mutable std::optional<sf::Text> _cachedWarnExText;
    mutable std::optional<sf::Text> _cachedPunishText;
    mutable const sf::Font* _cachedFont = nullptr;
    mutable std::string _cachedLabel;
    mutable std::string _cachedComboName;

    Enemy(sf::Vector2f spawnPos, EnemyType type = EnemyType::NORMAL);

    void setType(EnemyType type);
    EnemyType getType() const { return enemyType; }
    void triggerDodge(float dir = 0.0f);
    bool isInvulnerable() const { return isDodging; }

    sf::FloatRect getBounds() const;
    bool takeDamage(float amount, float knockX, float knockY, float stunTime = 0.18f);
    void triggerStun(float dur) { hurtTimer = std::max(hurtTimer, dur * stunMultiplier); }
    void triggerSpin(float duration = 0.65f, float speed = 900.0f) {
        spinTimer = duration;
        spinSpeed = speed;
    }
    float getStunTimer() const { return hurtTimer; }
    bool isStunned() const { return hurtTimer > 0.0f; }
    void reset();
    void update(float dt, const Arena& arena, const sf::Vector2f& playerPos = {0.0f, 0.0f}, bool playerIsStunned = false, bool playerIsAttacking = false);
    void draw(sf::RenderWindow& window, const sf::Font* font = nullptr);
    void applyAttackStats(const EnemyAttackStats& s, float dirX);

    void setSmartAI(bool smart) { isSmartAI = smart; }
    void triggerAttack();
    void triggerComboSequence(int forcedStarter = -1, int forcedHitLimit = -1);
    void dropCombo();
    void finishCombo(bool hitConfirmed);
    int getMaxComboHits() const { return maxComboHits; }
    bool getAllowAirFollowup() const { return allowAirFollowup; }
    void setComboLimits(int hits, bool airFollow) { maxComboHits = hits; allowAirFollowup = airFollow; }
    bool isCounterStanceActive() const { return counterStanceTimer > 0.0f; }
    void triggerCounterStance();
    void endCounterSuccess();
    bool isAttacking() const { return attackState == EnemyAttackState::ATTACKING; }
    bool isWindingUp() const { return attackState == EnemyAttackState::WINDUP; }
    bool isComboSequenceActive() const { return aiComboState != AIComboState::IDLE; }
    const sf::FloatRect& getAttackHitbox() const { return attackHitbox; }
    void stagger(float duration);

    AIBehaviorState getBehaviorState() const { return behaviorState; }
    void setBehaviorState(AIBehaviorState state) { behaviorState = state; }
    bool isPlayerDetected() const { return hasDetectedPlayer; }
    void forceDetectPlayer(sf::Vector2f pos);
    bool jump(float speedY = 850.0f);

    float getCurrentDamage() const { return currentAttackDamage; }
    float getCurrentKnockX() const { return currentAttackKnockX; }
    float getCurrentKnockY() const { return currentAttackKnockY; }
    float getCurrentStunTime() const { return currentAttackStun; }
    bool isCurrentAttackDiveSlam() const { return currentAttackIsDiveSlam; }
    const std::string& getCurrentComboName() const { return currentComboName; }
    AIStarterType getCurrentStarterType() const { return currentStarterType; }
};

// Синоним / обертка для удобного использования нового умного врага
class SmartEnemy : public Enemy {
public:
    SmartEnemy(sf::Vector2f spawnPos, EnemyType type = EnemyType::DUELIST) : Enemy(spawnPos, type) {
        setSmartAI(true);
    }
};

class TankEnemy : public Enemy {
public:
    TankEnemy(sf::Vector2f spawnPos) : Enemy(spawnPos, EnemyType::TANK) {
        setSmartAI(true);
    }
};

class DuelistEnemy : public Enemy {
public:
    DuelistEnemy(sf::Vector2f spawnPos) : Enemy(spawnPos, EnemyType::DUELIST) {
        setSmartAI(true);
    }
};

class DuelistPassiveEnemy : public Enemy {
public:
    DuelistPassiveEnemy(sf::Vector2f spawnPos) : Enemy(spawnPos, EnemyType::DUELIST_PASSIVE) {
        setSmartAI(true);
    }
};

