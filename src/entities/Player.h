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
    float maxAlpha = 180.0f;
    float lifetime = 0.15f;
    float maxLifetime = 0.15f;
    DashGhost(const sf::Sprite& spr, float a = 180.0f, float life = 0.15f)
        : sprite(spr), alpha(a), maxAlpha(a), lifetime(life), maxLifetime(life > 0.0f ? life : 0.15f) {}
};

enum class AttackAnimType {
    NONE,
    LIGHT,          // 1, f1, 111 (Light Jab)
    HEAVY,          // 2, f2, 222 (Heavy Strike / Power Slash)
    UPPERCUT,       // u1, u2, 112 (Rising Uppercut Launcher)
    SWEEP,          // dw1, dw2, 21db1 (Low Sweep / Reverse Sweep)
    DASH_ATTACK,    // d1, d2, 1f2d1 (Dash Thrust / Diagonal Launcher)
    THROW,          // 11b2 (Grab & Back Throw)
    AIR_ATTACK,     // a1, a2, a1a2 (Air Juggle / Air Push)
    DIVE_SLAM,      // adw2, a1adw2, a1a1adw2 (Air Down Slam / Crater Impact)
    TURN_ATTACK     // b1, b2 (Spinning Backfist)
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

    // Хилки на кнопку [R] (восстанавливают до макс ХП, по умолчанию 4 штуки)
    int healFlasks = 4;
    int maxHealFlasks = 4;

    int getHealFlasks() const { return healFlasks; }
    int getMaxHealFlasks() const { return maxHealFlasks; }
    void setHealFlasks(int count) { healFlasks = count; }
    void addHealFlasks(int count) { healFlasks = std::max(0, healFlasks + count); }
    bool useHealFlask();

    static constexpr float HALF_W = 20.0f;
    static constexpr float HALF_H = 27.5f;
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
    void update(float dt, const Arena& arena, const sf::Vector2f& mouseWorld = {0.0f, 0.0f});
    void draw(sf::RenderWindow& window);
    void reset(sf::Vector2f spawnPos, bool withWeapon = true);
    bool takeDamage(float amount, float knockX, float knockY, float stunTime, 
                    HitLevel hitLevel = HitLevel::MID, bool isBackHit = false, 
                    const sf::Vector2f& attackerPos = {0.0f, 0.0f});

    sf::FloatRect getBounds() const {
        return sf::FloatRect({position.x - HALF_W, position.y - TOP_OFFSET}, 
                             {HALF_W * 2.0f, TOP_OFFSET + BOTTOM_OFFSET});
    }

    bool isOnGround() const { return _isOnGround; }
    void setAirborneForTest(bool airborne) { _isOnGround = !airborne; if (airborne) _blockType = BlockType::NONE; }
    bool isFacingRight() const { return _animation.isFacingRight(); }
    bool isDashing() const { return _dashTimer > 0.0f; }
    void cancelDash() { _dashTimer = 0.0f; }
    bool isSprinting() const { return _isSprinting; }
    void cancelSprint() { _isSprinting = false; _sprintDirX = 0.0f; }
    bool isSpotDodging() const { return _isSpotDodging; }
    void triggerSpotDodge(bool spawnGhost = false) {
        const auto& pcfg = PlayerConfig::get();
        _isSpotDodging = true;
        _spotDodgeTimer = pcfg.spotDodgeDuration;
        _spotDodgeCooldown = pcfg.spotDodgeCooldown;
        _dashCooldown = pcfg.spotDodgeCooldown;
        _iframeTimer = pcfg.spotDodgeDuration;
        _perfectDodgeWindowTimer = pcfg.perfectDodgeWindow;
        _dashWindowTimer = 0.20f;
        velocity.x = 0.0f;
        if (spawnGhost) {
            _dashGhosts.emplace_back(_animation.getCurrentSprite(), 220.0f, pcfg.spotDodgeDuration);
            _dashGhosts.back().sprite.setColor(sf::Color(140, 220, 255, 210));
        }
    }
    bool isInvulnerable() const { return _iframeTimer > 0.0f; }
    bool isPerfectDodgeActive() const { return _perfectDodgeWindowTimer > 0.0f; }
    bool isPerfectDodgeEmpowered() const { return _perfectDodgeEmpoweredTimer > 0.0f; }
    void triggerPerfectDodge() { 
        _perfectDodgeEmpoweredTimer = PlayerConfig::get().perfectDodgeEmpower; 
        _dashCooldown = 0.0f;
        _spotDodgeCooldown = 0.0f;
        _dashTimer = 0.0f;
        _isSpotDodging = false;
        _perfectDodgeWindowTimer = 0.0f;
    }
    void consumePerfectDodgeEmpower() { _perfectDodgeEmpoweredTimer = 0.0f; }
    bool isDashWindow() const { return _dashWindowTimer > 0.0f; }
    void clearDashWindow() { _dashWindowTimer = 0.0f; }

    // Шкала позы / выносливости блока (Poise & Guard Meter / ХП блока)
    float guardHp = 100.0f;
    float maxGuardHp = 100.0f;
    float guardRegenDelay = 0.0f;
    float guardBrokenTimer = 0.0f;
    bool isGuardBroken() const { return guardBrokenTimer > 0.0f; }
    void breakGuard() {
        guardHp = 0.0f;
        guardBrokenTimer = 1.5f;
        _blockType = BlockType::NONE;
        triggerStun(1.5f);
    }
    void damageGuard(float amount) {
        guardHp -= amount;
        guardRegenDelay = 1.2f;
        if (guardHp <= 0.0f) {
            breakGuard();
        }
    }

    sf::Vector2f getEffectiveVelocity() const;
    bool isAirSuspended() const { return _isAirSuspended; }
    void setAirSuspended(bool s) { _isAirSuspended = s; }
    void lunge(float distance);
    void setSimulatedMoveDir(float dir) { _simulatedMoveDir = dir; }
    void setSimulatedVerticalDir(float dir) { _simulatedVerticalDir = dir; }
    float getSimulatedMoveDir() const { return _simulatedMoveDir; }
    float getSimulatedVerticalDir() const { return _simulatedVerticalDir; }
    void turnAround();

    void triggerStun(float dur) { _stunTimer = dur; _isJuggledByEnemy = false; cancelAttackAnimation(); }
    bool isStunned() const { return _stunTimer > 0.0f; }
    float getStunTimer() const { return _stunTimer; }

    bool hasWeapon() const { return _hasWeapon; }
    void setHasWeapon(bool val) { _hasWeapon = val; }

    // Система защиты / блока (верхний и нижний блок, снижение урона на 80%, удержание F/E, строго на земле)
    BlockType getBlockType() const { return _isOnGround ? _blockType : BlockType::NONE; }
    void setBlockType(BlockType bt) {
        if ((!_isOnGround || isGuardBroken()) && bt != BlockType::NONE) {
            _blockType = BlockType::NONE;
            return;
        }
        _blockType = bt;
    }
    bool isBlocking() const { return _isOnGround && !isGuardBroken() && (_blockType != BlockType::NONE); }
    void setBlocking(bool b) {
        if ((!_isOnGround || isGuardBroken()) && b) {
            _blockType = BlockType::NONE;
            return;
        }
        _blockType = b ? BlockType::HIGH : BlockType::NONE;
    }
    bool isBlockStunned() const { return _blockStunTimer > 0.0f; }
    float getBlockStunTimer() const { return _blockStunTimer; }
    void triggerBlockStun(float dur = 0.14f) { _blockStunTimer = dur; }

    // Стойка контр-атаки / парирования на кнопку [Q]
    bool isCounterStanceActive() const { return _isCounterStance && _counterWindowTimer > 0.0f; }
    bool isCounterWhiffStun() const { return _counterWhiffStunActive && isStunned(); }
    float getCounterWindowTimer() const { return _counterWindowTimer; }
    float getCounterCooldown() const { return _counterCooldown; }
    float getDashCooldown() const { return _dashCooldown; }
    float getSpotDodgeCooldown() const { return _spotDodgeCooldown; }
    void triggerCounterStance();
    void endCounterSuccess();
    void cancelCounterStance() { _isCounterStance = false; _counterWindowTimer = 0.0f; }

    bool wasWallBounced() const { return _hasWallBounced; }

    void setFacingLocked(bool locked) { _facingLocked = locked; }
    bool isFacingLocked() const { return _facingLocked; }

    void setAirGrabLocked(bool locked) { _isAirGrabLocked = locked; }
    bool isAirGrabLocked() const { return _isAirGrabLocked; }

    const Animation& getAnimation() const { return _animation; }
    Animation& getAnimation() { return _animation; }
    const Animation& getRollAnimation() const { return _rollAnimation; }
    Animation& getRollAnimation() { return _rollAnimation; }

    void triggerAttackAnimation(AttackAnimType animType, float duration);
    void cancelAttackAnimation();
    bool isPlayingAttackAnimation() const { return _currentAttackAnim != AttackAnimType::NONE && _attackAnimTimer > 0.0f; }
    AttackAnimType getCurrentAttackAnim() const { return _currentAttackAnim; }
    float getAttackAnimTimer() const { return _attackAnimTimer; }
    float getAttackAnimDuration() const { return _attackAnimDuration; }
    Animation* getAttackAnimation(AttackAnimType animType);
    const Animation* getAttackAnimation(AttackAnimType animType) const;

    bool isHoldingUp() const;
    bool isHoldingDown() const;
    bool isHoldingForward() const;
    bool isHoldingBackward() const;

private:
    Animation _animation;
    Animation _rollAnimation;
    Animation _lightAttackAnim;
    Animation _heavyAttackAnim;
    Animation _uppercutAnim;
    Animation _sweepAnim;
    Animation _dashAttackAnim;
    Animation _throwAnim;
    Animation _airAttackAnim;
    Animation _diveSlamAnim;
    Animation _turnAttackAnim;
    Animator _animator;

    AttackAnimType _currentAttackAnim = AttackAnimType::NONE;
    float _attackAnimTimer = 0.0f;
    float _attackAnimDuration = 0.0f;

    void syncAllAnimations(const sf::Color& color);

    bool _isOnGround = true; // С момента спавна персонаж стоит на земле, обновляется Arena коллизией
    bool _isAirSuspended = false;
    bool _facingLocked = false;
    float _turnDelayTimer = 0.0f;
    float _coyoteTime = 0.0f;
    float _jumpBuffer = 0.0f;
    bool _collisionEnabled = true;
    float _pendingLunge = 0.0f;
    float _simulatedMoveDir = 0.0f;
    float _simulatedVerticalDir = 0.0f;
    float _stunTimer = 0.0f;
    bool _isJuggledByEnemy = false;
    bool _forceAirborneForTest = false;

    float _dashTimer = 0.0f;
    float _dashWindowTimer = 0.0f;
    float _dashCooldown = 0.0f;
    float _iframeTimer = 0.0f;
    float _dashDirX = 1.0f;
    float _ghostSpawnTimer = 0.0f;

    bool _isSprinting = false;       // Бег после рывка на земле
    float _sprintDirX = 0.0f;        // Направление ускоренного бега
    float _sprintGhostTimer = 0.0f;  // Таймер шлейфа скорости бега

    bool _isSpotDodging = false;
    float _spotDodgeTimer = 0.0f;
    float _spotDodgeCooldown = 0.0f;
    float _perfectDodgeWindowTimer = 0.0f;
    float _perfectDodgeEmpoweredTimer = 0.0f;

    bool _isCounterStance = false;
    float _counterWindowTimer = 0.0f;
    float _counterCooldown = 0.0f;
    bool _counterWhiffStunActive = false;

    BlockType _blockType = BlockType::NONE;
    float _blockStunTimer = 0.0f;
    bool _isAirGrabLocked = false;

    bool _hasWeapon = true; // С самого начала персонаж появляется с булавой
    bool _hasWallBounced = false; // Отскок от стены (Wall Splat как в Mortal Kombat X)
    mutable sf::Texture _heldMaceTexture;
    mutable std::optional<sf::Sprite> _heldMaceSprite;
    mutable bool _heldMaceLoaded = false;

    std::vector<DashGhost> _dashGhosts;

    bool moveAxis(float& currentCoord, float delta, bool isX, float fixedCoord, const Arena& arena);
};

