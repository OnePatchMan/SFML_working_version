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
enum class AttackAnimType;

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
    void update(float dt, Player& player, std::vector<Enemy>& enemies);
    void drawWorld(sf::RenderWindow& window, const Player& player);
    void drawUI(sf::RenderWindow& window, const Player* player = nullptr);
    void reset();

    sf::Vector2f getScreenShake() const { return _shakeOffset; }
    const std::string& getCurrentComboName() const { return _comboDisplayName; }
    float getCooldown() const { return _attackCooldown; }
    float getAttackTimer() const { return _attackTimer; }
    bool isAttacking() const { return _attackTimer > 0.0f; }
    float getComboResetTimer() const { return _comboResetTimer; }
    float getComboRepeatTimer() const {
        float maxCd = 0.0f;
        for (const auto& kv : _comboCooldowns) {
            if (kv.second > maxCd) maxCd = kv.second;
        }
        return maxCd;
    }
    // Особый комбо-маркер (Special Combo Marker)
    struct SpecialComboMarker {
        sf::Vector2f position;
        std::string text;
        sf::Color color;
        float lifetime = 0.85f;
        float maxLifetime = 0.85f;
        float scale = 1.0f;
        float alpha = 255.0f;
    };

    // Расширяющаяся ударная волна (Shockwave Ring) для усиленных атак и критических попаданий
    struct EmpoweredShockwave {
        sf::Vector2f position;
        float radius = 12.0f;
        float maxRadius = 85.0f;
        float lifetime = 0.35f;
        float maxLifetime = 0.35f;
        sf::Color color{255, 215, 0};
    };

    // Брошенная булава / летающий снаряд оружия
    struct ThrownMace {
        sf::Vector2f position;
        sf::Vector2f velocity;
        float rotation = 0.0f;          // Угол вращения в градусах
        float angularVelocity = 0.0f;   // Скорость вращения (град/с)
        bool active = true;             // Активен ли в мире
        bool grounded = false;          // Упал ли на землю
        float hitCooldown = 0.0f;       // Кулдаун между ударами по врагам
        float lifetime = 25.0f;         // Время жизни до автовозврата
        float pickupCooldown = 0.22f;   // Задержка перед подбором игроком
    };

    void throwWeapon(Player& player, const sf::Vector2f& targetPoint = {0.0f, 0.0f});
    bool tryRetrieveWeapon(Player& player);
    const std::vector<ThrownMace>& getThrownMaces() const { return _thrownMaces; }
    void clearThrownMaces() { _thrownMaces.clear(); }

    int getComboHitCount() const { return _comboHitCount; }
    float getDamageScaling() const { return _currentDamageScaling; }
    float getHitstunDecay() const { return _currentHitstunDecay; }
    bool isEmpoweredStrikeActive() const { return _isEmpoweredStrike; }
    const std::vector<SpecialComboMarker>& getSpecialComboMarkers() const { return _specialComboMarkers; }

    const sf::FloatRect& getAttackHitbox() const { return _attackHitbox; }
    const std::vector<std::string>& getComboHistory() const { return _comboHistory; }
    const sf::Font* getFont() const { return _fontLoaded ? &_font : nullptr; }
    bool hasHitThisSwing() const { return _hasHitThisSwing; }

    static AttackAnimType resolveAttackAnimType(const std::string& attackType, const std::string& comboName);

    enum class StunReason { NONE, WHIFF, SPAM, ENEMY_HIT, COUNTER_WHIFF };
    StunReason getLastStunReason() const { return _lastStunReason; }
    float getComboCooldown(const std::string& comboId) const {
        auto it = _comboCooldowns.find(comboId);
        return (it != _comboCooldowns.end()) ? it->second : 0.0f;
    }

private:
    // История ввода комбо: {"1", "1", "2"}, {"1", "2", "d1"}, {"a1", "a2"}, {"a2", "ad1"}
    std::vector<std::string> _comboHistory;
    float _comboResetTimer = 0.0f;
    std::string _pendingBaseCombo = "";

    // Анти-спам система: каждое завершенное комбо получает таймер 2.5с.
    // Если в течение 2.5с дается то же самое комбо (не важно, что было между ними) — стан-лок!
    std::unordered_map<std::string, float> _comboCooldowns;
    static constexpr float COMBO_SPAM_COOLDOWN = 2.5f;

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
    std::vector<SpecialComboMarker> _specialComboMarkers;
    std::vector<EmpoweredShockwave> _empoweredShockwaves;
    std::vector<ThrownMace> _thrownMaces;

    sf::Texture _maceTexture;
    bool _maceTextureLoaded = false;

    // Скейлинг урона и снижение длительности стана в длинных комбо
    int _comboHitCount = 0;
    float _currentDamageScaling = 1.0f;
    float _currentHitstunDecay = 1.0f;
    bool _isEmpoweredStrike = false;

    sf::Font _font;
    bool _fontLoaded = false;
    std::mt19937 _rng{777};

    void updateInternal(float dt, Player& player, const std::vector<Enemy*>& enemiesList);
    void triggerAttack(AttackInput input, Player& player, const sf::Vector2f& mouseWorld);
};
