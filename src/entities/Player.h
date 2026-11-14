#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "core/Common.h"
#include "graphics/Animation.h"
#include "entities/PlayerConfig.h"

class Arena;

struct DashGhost {
    sf::Sprite sprite;
    float alpha = 180.0f;
    float lifetime = 0.15f;
    DashGhost(const sf::Sprite& spr, float a = 180.0f, float life = 0.15f)
        : sprite(spr), alpha(a), lifetime(life) {}
};

class Player {
public:
    sf::Vector2f position;
    sf::Vector2f velocity{0.0f, 0.0f};
    float hp = 100.0f;
    float maxHp = 100.0f;
    float displayedHp = 100.0f;
    bool isDead = false;
    bool diveSlamBounce = false;
    float diveSlamDir = 1.0f;

    static constexpr float HALF_W = 20.0f;
    static constexpr float TOP_OFFSET = 25.0f;
    static constexpr float BOTTOM_OFFSET = 30.0f;

    static constexpr float MOVE_SPEED = 400.0f;
    static constexpr float GRAVITY = 3000.0f;
    static constexpr float JUMP_SPEED = 900.0f;
    static constexpr float MAX_FALL_SPEED = 1800.0f;

    static constexpr float DASH_SPEED = 1250.0f;
    static constexpr float DASH_DURATION = 0.09f;
    static constexpr float DASH_COOLDOWN = 0.30f;
    static constexpr float IFRAME_DURATION = 0.13f;

    static constexpr float SPOT_DODGE_DURATION = 0.22f;
    static constexpr float SPOT_DODGE_COOLDOWN = 0.32f;
    static constexpr float PERFECT_DODGE_WINDOW = 0.11f;

    Player(sf::Vector2f spawnPos);

    void handleInput(const std::optional<sf::Event>& event);
    void update(float dt, const Arena& arena, const sf::Vector2f& mouseWorld);
    void draw(sf::RenderWindow& window);
    void reset(sf::Vector2f spawnPos);
    bool takeDamage(float amount, float knockX, float knockY, float stunTime);

    sf::FloatRect getBounds() const {
        return sf::FloatRect({position.x - HALF_W, position.y - TOP_OFFSET}, 
                             {HALF_W * 2.0f, TOP_OFFSET + BOTTOM_OFFSET});
    }

    bool isOnGround() const { return _isOnGround; }
    bool isFacingRight() const { return _animation.isFacingRight(); }
    bool isDashing() const { return _dashTimer > 0.0f; }
    void cancelDash() { _dashTimer = 0.0f; }
    bool isSpotDodging() const { return _isSpotDodging; }
    bool isInvulnerable() const { return _iframeTimer > 0.0f; }
    bool isPerfectDodgeActive() const { return _perfectDodgeWindowTimer > 0.0f; }
    bool isPerfectDodgeEmpowered() const { return _perfectDodgeEmpoweredTimer > 0.0f; }
    void triggerPerfectDodge() { _perfectDodgeEmpoweredTimer = PlayerConfig::get().perfectDodgeEmpower; }
    void consumePerfectDodgeEmpower() { _perfectDodgeEmpoweredTimer = 0.0f; }
    bool isDashWindow() const { return _dashWindowTimer > 0.0f; }
    void clearDashWindow() { _dashWindowTimer = 0.0f; }
    bool isAirSuspended() const { return _isAirSuspended; }
    void setAirSuspended(bool s) { _isAirSuspended = s; }
    void lunge(float distance);
    void setSimulatedMoveDir(float dir) { _simulatedMoveDir = dir; }
    void setSimulatedVerticalDir(float dir) { _simulatedVerticalDir = dir; }
    void turnAround() { _animation.setFacing(!_animation.isFacingRight()); }

    void triggerStun(float dur) { _stunTimer = dur; }
    bool isStunned() const { return _stunTimer > 0.0f; }
    float getStunTimer() const { return _stunTimer; }

    // Стойка контр-атаки / парирования на кнопку [Q]
    bool isCounterStanceActive() const { return _isCounterStance && _counterWindowTimer > 0.0f; }
    bool isCounterWhiffStun() const { return _counterWhiffStunActive && isStunned(); }
    float getCounterWindowTimer() const { return _counterWindowTimer; }
    float getCounterCooldown() const { return _counterCooldown; }
    void triggerCounterStance();
    void endCounterSuccess();
    void cancelCounterStance() { _isCounterStance = false; _counterWindowTimer = 0.0f; }

    void setFacingLocked(bool locked) { _facingLocked = locked; }
    bool isFacingLocked() const { return _facingLocked; }

    bool isHoldingUp() const;
    bool isHoldingDown() const;
    bool isHoldingForward() const;
    bool isHoldingBackward() const;

private:
    Animation _animation;
    Animator _animator;

    bool _isOnGround = false;
    bool _isAirSuspended = false;
    bool _facingLocked = false;
    float _turnDelayTimer = 0.0f;
    float _coyoteTime = 0.0f;
    float _jumpBuffer = 0.0f;
    bool _collisionEnabled = true;
    float _simulatedMoveDir = 0.0f;
    float _simulatedVerticalDir = 0.0f;
    float _stunTimer = 0.0f;

    float _dashTimer = 0.0f;
    float _dashWindowTimer = 0.0f;
    float _dashCooldown = 0.0f;
    float _iframeTimer = 0.0f;
    float _dashDirX = 1.0f;
    float _ghostSpawnTimer = 0.0f;

    bool _isSpotDodging = false;
    float _spotDodgeTimer = 0.0f;
    float _spotDodgeCooldown = 0.0f;
    float _perfectDodgeWindowTimer = 0.0f;
    float _perfectDodgeEmpoweredTimer = 0.0f;

    bool _isCounterStance = false;
    float _counterWindowTimer = 0.0f;
    float _counterCooldown = 0.0f;
    bool _counterWhiffStunActive = false;

    std::vector<DashGhost> _dashGhosts;

    bool moveAxis(float& currentCoord, float delta, bool isX, float fixedCoord, const Arena& arena);
};

