#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <random>
#include <optional>
#include "core/Common.h"
#include "combat/CombatConfig.h"

class Player;
class Enemy;

struct DamageNumber {
    sf::Vector2f position;
    std::string text;
    float alpha = 255.0f;
    float lifetime = 0.65f;
    sf::Color color;
};

struct HitParticle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float alpha = 255.0f;
    float lifetime = 0.28f;
    sf::Color color;
    float size = 3.5f;
};

enum class AttackInput {
    LIGHT, // 1 (ЛКМ / J)
    HEAVY  // 2 (ПКМ / K)
};

class CombatSystem {
public:
    CombatSystem();

    void handleInput(const std::optional<sf::Event>& event, Player& player, const sf::Vector2f& mouseWorld);
    void update(float dt, Player& player, Enemy& enemy);
    void drawWorld(sf::RenderWindow& window, const Player& player);
    void drawUI(sf::RenderWindow& window, const Player* player = nullptr);
    void reset();

    sf::Vector2f getScreenShake() const { return _shakeOffset; }
    const std::string& getCurrentComboName() const { return _comboDisplayName; }
    float getCooldown() const { return _attackCooldown; }
    float getAttackTimer() const { return _attackTimer; }
    bool isAttacking() const { return _attackTimer > 0.0f; }
    float getComboResetTimer() const { return _comboResetTimer; }
    float getComboRepeatTimer() const { return _comboRepeatTimer; }
    const sf::FloatRect& getAttackHitbox() const { return _attackHitbox; }
    const std::vector<std::string>& getComboHistory() const { return _comboHistory; }
    const sf::Font* getFont() const { return _fontLoaded ? &_font : nullptr; }
    bool hasHitThisSwing() const { return _hasHitThisSwing; }

    enum class StunReason { NONE, WHIFF, SPAM, ENEMY_HIT, COUNTER_WHIFF };
    StunReason getLastStunReason() const { return _lastStunReason; }

private:
    // История ввода комбо: {"1", "1", "2"}, {"1", "2", "d1"}, {"a1", "a2"}, {"a2", "ad1"}
    std::vector<std::string> _comboHistory;
    float _comboResetTimer = 0.0f;
    std::string _lastFinishedCombo = "";
    std::string _pendingBaseCombo = "";
    int _repeatComboCount = 0;
    float _comboRepeatTimer = 0.0f; // Окно учета спама комбо (3.0с после завершения первого)

    std::optional<AttackInput> _bufferedAttackInput;
    float _bufferedInputTimer = 0.0f;

    std::string _currentAttackType = "";  // "1", "2", "d1", "d2", "a1", "a2", "ad1", "ad2"
    std::string _comboDisplayName = "";
    float _comboDisplayTimer = 0.0f;
    sf::Color _comboDisplayColor{255, 255, 255};

    bool _isAirAttack = false;
    float _airSuspendedRemaining = 0.0f;
    float _attackTimer = 0.0f;
    float _attackCooldown = 0.0f;
    float _currentWhiffStun = 0.40f;
    bool _hasHitThisSwing = false;
    StunReason _lastStunReason = StunReason::NONE;

    // Параметры текущего удара
    float _currentDamage = 2.0f;
    float _knockX = 0.0f;
    float _knockY = 0.0f;
    float _enemyStunTime = 0.18f;
    sf::FloatRect _attackHitbox;
    sf::Color _attackColor{255, 255, 255};
    int _attackDirection = 1; // 1 = право, -1 = лево

    // Тряска экрана
    float _shakeTimer = 0.0f;
    float _shakeIntensity = 0.0f;
    sf::Vector2f _shakeOffset{0.0f, 0.0f};

    std::vector<DamageNumber> _damageNumbers;
    std::vector<HitParticle> _hitParticles;

    sf::Font _font;
    bool _fontLoaded = false;
    std::mt19937 _rng{777};

    void triggerAttack(AttackInput input, Player& player, const sf::Vector2f& mouseWorld);
};
