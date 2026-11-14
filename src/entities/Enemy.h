#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "core/Common.h"
#include "entities/EnemyConfig.h"

class Arena;

enum class EnemyAttackState {
    IDLE,
    WINDUP,     // Замах / телеграф удара
    ATTACKING,  // Активная фаза удара
    RECOVERY    // Откат после удара
};

enum class AIStarterType {
    STARTER_1F2D1, // 1 -> f2 -> d1 (диагональный выпад и лаунчер)
    STARTER_112,   // 1 -> 1 -> 2 (быстрые джебы и апперкот)
    STARTER_21DB1  // 2 -> 1 -> db1 (тяжелый замах, рывок за спину и вертикальная подсечка)
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
    float hp = 100.0f;
    float maxHp = 100.0f;
    float displayedHp = 100.0f;
    bool isDead = false;
    float hurtTimer = 0.0f;
    float respawnTimer = 0.0f;
    sf::Vector2f spawnPosition;

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

    Enemy(sf::Vector2f spawnPos);

    sf::FloatRect getBounds() const;
    bool takeDamage(float amount, float knockX, float knockY, float stunTime = 0.18f);
    void reset();
    void update(float dt, const Arena& arena, const sf::Vector2f& playerPos = {0.0f, 0.0f});
    void draw(sf::RenderWindow& window, const sf::Font* font = nullptr);

    void setSmartAI(bool smart) { isSmartAI = smart; }
    void triggerAttack();
    void triggerComboSequence(int forcedStarter = -1);
    void dropCombo();
    bool isCounterStanceActive() const { return counterStanceTimer > 0.0f; }
    void triggerCounterStance();
    void endCounterSuccess();
    bool isAttacking() const { return attackState == EnemyAttackState::ATTACKING; }
    bool isWindingUp() const { return attackState == EnemyAttackState::WINDUP; }
    bool isComboSequenceActive() const { return aiComboState != AIComboState::IDLE; }
    const sf::FloatRect& getAttackHitbox() const { return attackHitbox; }
    void stagger(float duration);

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
    SmartEnemy(sf::Vector2f spawnPos) : Enemy(spawnPos) {
        setSmartAI(true);
    }
};

