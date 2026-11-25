#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "core/Common.h"

struct EnemyAttackStats {
    float windup = 0.0f;
    float activeTime = 0.14f;
    float recoveryTime = 0.06f;
    float damage = 2.0f;
    float knockX = 160.0f;
    float knockY = -25.0f;
    float stun = 0.24f;
    float lunge = 0.0f;
    float hbw = 55.0f;
    float hbh = 42.0f;
    float hbyOffset = -18.0f;
    bool isDiveSlam = false;
};

struct Starter1F2D1Config {
    float windup = 0.07f;
    EnemyAttackStats hit1{0.0f, 0.12f, 0.06f, 1.0f, 160.0f, -25.0f, 0.24f, 0.0f, 55.0f, 42.0f, -18.0f, false};
    EnemyAttackStats hitF2{0.0f, 0.16f, 0.06f, 1.8f, 460.0f, -45.0f, 0.35f, 18.0f, 70.0f, 46.0f, -20.0f, false};
    EnemyAttackStats hitD1{0.0f, 0.18f, 0.06f, 2.5f, 220.0f, -435.0f, 0.48f, 30.0f, 75.0f, 70.0f, -42.0f, false};
};

struct Starter112Config {
    float windup = 0.06f;
    EnemyAttackStats hit1{0.0f, 0.12f, 0.05f, 1.0f, 150.0f, -25.0f, 0.24f, 0.0f, 55.0f, 42.0f, -18.0f, false};
    EnemyAttackStats hit2nd1{0.0f, 0.12f, 0.05f, 1.1f, 160.0f, -25.0f, 0.24f, 14.0f, 58.0f, 44.0f, -18.0f, false};
    EnemyAttackStats hit2{0.0f, 0.18f, 0.06f, 2.2f, 180.0f, -435.0f, 0.45f, 18.0f, 70.0f, 68.0f, -40.0f, false};
};

struct Starter21DB1Config {
    float windup = 0.09f;
    EnemyAttackStats hit2{0.0f, 0.16f, 0.06f, 1.8f, 350.0f, -35.0f, 0.30f, 0.0f, 65.0f, 46.0f, -18.0f, false};
    EnemyAttackStats hit1{0.0f, 0.14f, 0.06f, 1.5f, 850.0f, -35.0f, 0.35f, 16.0f, 60.0f, 44.0f, -18.0f, false};
    float dashBehindDuration = 0.08f;
    float dashBehindOffset = 38.0f;
    EnemyAttackStats hitSweep{0.0f, 0.15f, 0.06f, 2.4f, 0.0f, -435.0f, 0.48f, 0.0f, 70.0f, 50.0f, -10.0f, false};
};

struct GrapplePullConfig {
    float windup = 0.18f;
    EnemyAttackStats hit{0.0f, 0.20f, 0.10f, 1.5f, 0.0f, 0.0f, 0.45f, 0.0f, 160.0f, 40.0f, -18.0f, false};
    float pullSpeed = 580.0f;
};

struct AirFollowup1Config {
    float jumpVelY = -880.0f;
    float jumpDuration = 0.32f;
    float jumpHorizFactor = 6.5f;
    float jumpMaxSpeedX = 500.0f;
    EnemyAttackStats hitA1{0.0f, 0.14f, 0.06f, 1.0f, 60.0f, -40.0f, 0.35f, 0.0f, 70.0f, 55.0f, -25.0f, false};
    EnemyAttackStats hitAdw2{0.0f, 0.18f, 0.06f, 2.2f, 440.0f, 550.0f, 0.42f, 0.0f, 75.0f, 60.0f, -15.0f, true};
};

struct AirFollowup2Config {
    float groundAdvanceDuration = 0.18f;
    float groundAdvanceDist = 72.0f;
    float jumpVelY = -860.0f;
    float jumpDuration = 0.32f;
    EnemyAttackStats hitA1_1{0.0f, 0.12f, 0.05f, 1.0f, 60.0f, -40.0f, 0.35f, 0.0f, 70.0f, 55.0f, -25.0f, false};
    EnemyAttackStats hitA1_2{0.0f, 0.12f, 0.05f, 1.0f, 60.0f, -40.0f, 0.35f, 0.0f, 65.0f, 50.0f, -25.0f, false};
    EnemyAttackStats hitAdw2{0.0f, 0.30f, 0.50f, 2.5f, 1150.0f, -280.0f, 0.52f, 0.0f, 80.0f, 65.0f, -15.0f, false};
    float diveSpeedY = 1200.0f;
    float finalKnockXVertical = 600.0f;
};

struct EnemyCounterConfig {
    float window = 0.35f;
    float cooldown = 3.50f;
    float whiffStun = 1.20f;
    float playerStun = 1.40f;
    float damage = 3.5f;
    float knockX = 320.0f;
    float knockY = -40.0f;
    float triggerDistance = 65.0f;
};

struct EnemyPerceptionConfig {
    float viewDistance = 450.0f;     // Дальность обзора вперед (px)
    float rearViewDistance = 110.0f; // Радиус обнаружения сзади / слух (px)
    float verticalViewSpan = 180.0f; // Вертикальный сектор видимости (px)
    float patrolSpeed = 120.0f;      // Скорость патрулирования (px/s)
    float patrolRadius = 220.0f;     // Радиус патрулирования от точки спавна (px)
    float patrolWaitTime = 1.0f;     // Ожидание на краю патруля перед разворотом (сек)
    float alertDuration = 0.45f;     // Время фазы тревоги (!) перед погоней (сек)
    float lostSightTimeout = 2.5f;   // Время поиска игрока после потери из виду (сек)
    float jumpSpeed = 850.0f;        // Сила прыжка врага (px/s)
    float jumpCooldown = 0.65f;      // Перезарядка прыжков врага (сек)
};

struct EnemyGeneralStats {
    float maxHp = 45.0f;
    float gravity = 2600.0f;
    float whiffRecovery = 0.50f;
    float hitRecovery = 0.20f;
    float neutralPursuitSpeed = 220.0f;
    float approachSpeed = 280.0f;
    float attackCooldown = 0.75f;
    float dummyWindup = 0.38f;
    float dummyActiveTime = 0.16f;
    float dummyRecovery = 0.20f;
    float dummyDamage = 1.5f;
    float dummyKnockX = 250.0f;
    float dummyKnockY = -30.0f;
    float dummyStun = 0.25f;
    float dummyCooldown = 2.4f;
    float dummyHbw = 55.0f;
    float dummyHbh = 42.0f;
    float dummyHbyOffset = -18.0f;
};

// -------------------------------------------------------------
// АРХЕТИПЫ ВРАГОВ (ПОЛНЫЙ НАБОР СВОЙСТВ И КОМБО ДЛЯ КАЖДОГО)
// -------------------------------------------------------------
struct EnemyArchetypeData {
    float hp = 45.0f;
    float speed = 220.0f;
    float approachSpeed = 280.0f;
    float damageMult = 1.0f;
    float hitboxScale = 1.0f;
    float scale = 1.0f;
    float stunMult = 1.0f;
    float knockMult = 1.0f;
    float windupMult = 1.0f;
    bool canCounter = false;
    bool canBlock = false;
    bool canDodge = false;
    float aggressiveness = 0.5f;
    float intelligence = 0.5f;
    float dodgeCooldown = 1.5f;
    float spacingDistance = 108.0f;
    float idleAttackDelay = 0.5f;
    float invisDuration = 1.0f;
    float invisCooldown = 7.0f;

    EnemyGeneralStats stats;
    Starter1F2D1Config starter1;
    Starter112Config starter2;
    Starter21DB1Config starter3;
    GrapplePullConfig grapplePull;
    AirFollowup1Config air1;
    AirFollowup2Config air2;
    EnemyCounterConfig counter;
    EnemyPerceptionConfig perception;
};

struct NormalEnemyConfig {
    float hp = 45.0f;
    float speed = 220.0f;
    float approachSpeed = 280.0f;
    float damageMult = 5.5f; // Урон обычной атаки 5-6 (5.5)
    float scale = 1.0f;
    bool canCounter = false;
    bool canBlock = false;
    bool canDodge = false;
};

struct TankEnemyConfig {
    float hp = 110.0f;
    float speed = 135.0f;
    float approachSpeed = 180.0f;
    float damageMult = 1.65f;
    float hitboxScale = 1.50f;
    float scale = 1.50f;
    float stunMult = 0.55f;
    float knockMult = 0.50f;
    float windupMult = 1.30f;
    bool canCounter = false;
    bool canBlock = true;
};

struct DuelistEnemyConfig {
    float hp = 65.0f;
    float speed = 340.0f;
    float approachSpeed = 420.0f;
    float damageMult = 1.15f;
    float scale = 1.0f;
    bool canCounter = true;
    bool canBlock = true;
    bool canDodge = true;
    float aggressiveness = 0.95f;
    float intelligence = 0.85f;
    float dodgeCooldown = 1.50f;
    float spacingDistance = 108.0f;
    float idleAttackDelay = 0.50f;
};

struct DuelistPassiveConfig {
    float hp = 65.0f;
    float speed = 320.0f;
    float approachSpeed = 380.0f;
    float damageMult = 1.15f;
    float scale = 1.0f;
    bool canCounter = true;
    bool canBlock = true;
    bool canDodge = true;
    float aggressiveness = 0.25f;
    float intelligence = 0.95f;
    float dodgeCooldown = 2.00f;
    float spacingDistance = 115.0f;
    float invisDuration = 1.00f;
    float invisCooldown = 7.00f;
};

struct EnemyAllConfig {
    EnemyGeneralStats stats;
    Starter1F2D1Config starter1;
    Starter112Config starter2;
    Starter21DB1Config starter3;
    AirFollowup1Config air1;
    AirFollowup2Config air2;
    EnemyCounterConfig counter;
    EnemyPerceptionConfig perception;
    NormalEnemyConfig normal;
    TankEnemyConfig tank;
    DuelistEnemyConfig duelist;
    DuelistPassiveConfig duelistPassive;

    // Персональные конфиги врагов со всеми их комбо и таймингами
    EnemyArchetypeData normalArchetype;
    EnemyArchetypeData tankArchetype;
    EnemyArchetypeData duelistArchetype;
    EnemyArchetypeData duelistPassiveArchetype;
    EnemyArchetypeData dummyArchetype;
};

enum class EnemyType;

class EnemyConfig {
public:
    static const EnemyAllConfig& get() {
        ensureInitialized();
        return _config;
    }

    static EnemyAllConfig& getMutable() {
        ensureInitialized();
        return _config;
    }

    static const EnemyArchetypeData& getArchetype(int type);

    static bool reload() {
        _initialized = false;
        ensureInitialized();
        std::cout << "[KV CONFIG] Индивидуальные конфигурации врагов и комбо перезагружены!" << std::endl;
        return true;
    }

    static bool loadKVFile(const std::string& filepath) {
        return kv::forEachPair(filepath, [](const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
            applyKV(stack, key, val);
        });
    }

    static bool loadArchetypeKVFile(const std::string& filepath, EnemyArchetypeData& arch) {
        return kv::forEachPair(filepath, [&arch](const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
            applyArchetypeKV(arch, stack, key, val);
        });
    }

private:
    inline static EnemyAllConfig _config;
    inline static bool _initialized = false;

    static void ensureInitialized() {
        if (_initialized) return;
        _initialized = true;
        _config = EnemyAllConfig{};

        // 1. Инициализация базовых значений
        std::string kvMain = resolveAssetPath("assets/config/enemy.kv");
        if (std::filesystem::exists(kvMain)) {
            loadKVFile(kvMain);
        }

        // Копируем общие комбо в архетипы по умолчанию
        auto copyBaseToArch = [](EnemyArchetypeData& a, const EnemyAllConfig& cfg) {
            a.stats = cfg.stats;
            a.starter1 = cfg.starter1;
            a.starter2 = cfg.starter2;
            a.starter3 = cfg.starter3;
            a.air1 = cfg.air1;
            a.air2 = cfg.air2;
            a.counter = cfg.counter;
            a.perception = cfg.perception;
        };

        copyBaseToArch(_config.normalArchetype, _config);
        _config.normalArchetype.hp = _config.normal.hp;
        _config.normalArchetype.speed = _config.normal.speed;
        _config.normalArchetype.approachSpeed = _config.normal.approachSpeed;
        _config.normalArchetype.damageMult = _config.normal.damageMult;
        _config.normalArchetype.scale = _config.normal.scale;
        _config.normalArchetype.canCounter = _config.normal.canCounter;
        _config.normalArchetype.canBlock = _config.normal.canBlock;
        _config.normalArchetype.canDodge = _config.normal.canDodge;

        copyBaseToArch(_config.tankArchetype, _config);
        _config.tankArchetype.hp = _config.tank.hp;
        _config.tankArchetype.speed = _config.tank.speed;
        _config.tankArchetype.approachSpeed = _config.tank.approachSpeed;
        _config.tankArchetype.damageMult = _config.tank.damageMult;
        _config.tankArchetype.hitboxScale = _config.tank.hitboxScale;
        _config.tankArchetype.scale = _config.tank.scale;
        _config.tankArchetype.stunMult = _config.tank.stunMult;
        _config.tankArchetype.knockMult = _config.tank.knockMult;
        _config.tankArchetype.windupMult = _config.tank.windupMult;
        _config.tankArchetype.canCounter = _config.tank.canCounter;
        _config.tankArchetype.canBlock = _config.tank.canBlock;

        copyBaseToArch(_config.duelistArchetype, _config);
        _config.duelistArchetype.hp = _config.duelist.hp;
        _config.duelistArchetype.speed = _config.duelist.speed;
        _config.duelistArchetype.approachSpeed = _config.duelist.approachSpeed;
        _config.duelistArchetype.damageMult = _config.duelist.damageMult;
        _config.duelistArchetype.scale = _config.duelist.scale;
        _config.duelistArchetype.canCounter = _config.duelist.canCounter;
        _config.duelistArchetype.canBlock = _config.duelist.canBlock;
        _config.duelistArchetype.canDodge = _config.duelist.canDodge;
        _config.duelistArchetype.aggressiveness = _config.duelist.aggressiveness;
        _config.duelistArchetype.intelligence = _config.duelist.intelligence;
        _config.duelistArchetype.dodgeCooldown = _config.duelist.dodgeCooldown;
        _config.duelistArchetype.spacingDistance = _config.duelist.spacingDistance;
        _config.duelistArchetype.idleAttackDelay = _config.duelist.idleAttackDelay;

        copyBaseToArch(_config.duelistPassiveArchetype, _config);
        _config.duelistPassiveArchetype.hp = _config.duelistPassive.hp;
        _config.duelistPassiveArchetype.speed = _config.duelistPassive.speed;
        _config.duelistPassiveArchetype.approachSpeed = _config.duelistPassive.approachSpeed;
        _config.duelistPassiveArchetype.damageMult = _config.duelistPassive.damageMult;
        _config.duelistPassiveArchetype.scale = _config.duelistPassive.scale;
        _config.duelistPassiveArchetype.canCounter = _config.duelistPassive.canCounter;
        _config.duelistPassiveArchetype.canBlock = _config.duelistPassive.canBlock;
        _config.duelistPassiveArchetype.canDodge = _config.duelistPassive.canDodge;
        _config.duelistPassiveArchetype.aggressiveness = _config.duelistPassive.aggressiveness;
        _config.duelistPassiveArchetype.intelligence = _config.duelistPassive.intelligence;
        _config.duelistPassiveArchetype.dodgeCooldown = _config.duelistPassive.dodgeCooldown;
        _config.duelistPassiveArchetype.spacingDistance = _config.duelistPassive.spacingDistance;
        _config.duelistPassiveArchetype.invisDuration = _config.duelistPassive.invisDuration;
        _config.duelistPassiveArchetype.invisCooldown = _config.duelistPassive.invisCooldown;

        copyBaseToArch(_config.dummyArchetype, _config);
        _config.dummyArchetype.hp = 100.0f;
        _config.dummyArchetype.scale = 1.0f;

        // 2. Загружаем индивидуальные KV файлы каждого врага
        std::string normalKV = resolveAssetPath("assets/config/enemy_normal.kv");
        if (std::filesystem::exists(normalKV)) loadArchetypeKVFile(normalKV, _config.normalArchetype);

        std::string tankKV = resolveAssetPath("assets/config/enemy_tank.kv");
        if (std::filesystem::exists(tankKV)) loadArchetypeKVFile(tankKV, _config.tankArchetype);

        std::string duelistKV = resolveAssetPath("assets/config/enemy_duelist.kv");
        if (std::filesystem::exists(duelistKV)) loadArchetypeKVFile(duelistKV, _config.duelistArchetype);

        std::string duelistPassKV = resolveAssetPath("assets/config/enemy_duelist_passive.kv");
        if (std::filesystem::exists(duelistPassKV)) loadArchetypeKVFile(duelistPassKV, _config.duelistPassiveArchetype);

        std::string dummyKV = resolveAssetPath("assets/config/enemy_dummy.kv");
        if (std::filesystem::exists(dummyKV)) loadArchetypeKVFile(dummyKV, _config.dummyArchetype);
    }

    static void parseAttackField(EnemyAttackStats& st, const std::string& key, const std::string& val) {
        if (key == "active_time" || key == "activetime") kv::tryParseFloat(val, st.activeTime);
        else if (key == "active_frames" || key == "activeframes") kv::assignFramesSeconds(val, st.activeTime);
        else if (key == "duration_frames" || key == "durationframes" || key == "total_frames") {
            float totalTime = 0.0f;
            if (kv::assignFramesSeconds(val, totalTime)) {
                st.recoveryTime = std::max(0.01f, totalTime - st.windup - st.activeTime);
            }
        }
        else if (key == "recovery_time" || key == "recoverytime" || key == "recovery") kv::tryParseFloat(val, st.recoveryTime);
        else if (key == "damage") kv::tryParseFloat(val, st.damage);
        else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, st.knockX);
        else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, st.knockY);
        else if (key == "stun") kv::tryParseFloat(val, st.stun);
        else if (key == "lunge") kv::tryParseFloat(val, st.lunge);
        else if (key == "hitbox_w" || key == "hitboxw") kv::tryParseFloat(val, st.hbw);
        else if (key == "hitbox_h" || key == "hitboxh") kv::tryParseFloat(val, st.hbh);
        else if (key == "hitbox_offset_y" || key == "hitboxoffsety") kv::tryParseFloat(val, st.hbyOffset);
    }

    static void applyArchetypeKV(EnemyArchetypeData& arch, const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
        if (stack.size() < 2) return;
        const std::string& root = stack[1];

        if (root == "Stats") {
            if (key == "max_hp" || key == "hp" || key == "maxhp") kv::tryParseFloat(val, arch.hp);
            else if (key == "speed") kv::tryParseFloat(val, arch.speed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, arch.approachSpeed);
            else if (key == "damage_multiplier" || key == "damagemultiplier" || key == "damagemult") kv::tryParseFloat(val, arch.damageMult);
            else if (key == "scale") kv::tryParseFloat(val, arch.scale);
            else if (key == "hitbox_scale" || key == "hitboxscale") kv::tryParseFloat(val, arch.hitboxScale);
            else if (key == "stun_multiplier" || key == "stunmult") kv::tryParseFloat(val, arch.stunMult);
            else if (key == "knock_multiplier" || key == "knockmult") kv::tryParseFloat(val, arch.knockMult);
            else if (key == "windup_multiplier" || key == "windupmult") kv::tryParseFloat(val, arch.windupMult);
            else if (key == "can_counter" || key == "cancounter") arch.canCounter = kv::parseBool(val, arch.canCounter);
            else if (key == "can_block" || key == "canblock") arch.canBlock = kv::parseBool(val, arch.canBlock);
            else if (key == "can_dodge" || key == "candodge") arch.canDodge = kv::parseBool(val, arch.canDodge);
            else if (key == "aggressiveness") kv::tryParseFloat(val, arch.aggressiveness);
            else if (key == "intelligence") kv::tryParseFloat(val, arch.intelligence);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") kv::tryParseFloat(val, arch.dodgeCooldown);
            else if (key == "spacing_distance" || key == "spacingdistance") kv::tryParseFloat(val, arch.spacingDistance);
            else if (key == "idle_attack_delay" || key == "idleattackdelay") kv::tryParseFloat(val, arch.idleAttackDelay);
            else if (key == "invis_duration" || key == "invisduration") kv::tryParseFloat(val, arch.invisDuration);
            else if (key == "invis_cooldown" || key == "inviscooldown") kv::tryParseFloat(val, arch.invisCooldown);
            else if (key == "whiff_recovery" || key == "whiffrecovery") kv::tryParseFloat(val, arch.stats.whiffRecovery);
            else if (key == "hit_recovery" || key == "hitrecovery") kv::tryParseFloat(val, arch.stats.hitRecovery);
            else if (key == "attack_cooldown" || key == "attackcooldown") kv::tryParseFloat(val, arch.stats.attackCooldown);
            else if (key == "gravity") kv::tryParseFloat(val, arch.stats.gravity);
        } else if (root == "Perception") {
            if (key == "view_distance" || key == "viewdistance") kv::tryParseFloat(val, arch.perception.viewDistance);
            else if (key == "rear_view_distance" || key == "rearviewdistance") kv::tryParseFloat(val, arch.perception.rearViewDistance);
            else if (key == "vertical_view_span" || key == "verticalviewspan") kv::tryParseFloat(val, arch.perception.verticalViewSpan);
            else if (key == "patrol_speed" || key == "patrolspeed") kv::tryParseFloat(val, arch.perception.patrolSpeed);
            else if (key == "patrol_radius" || key == "patrolradius") kv::tryParseFloat(val, arch.perception.patrolRadius);
            else if (key == "patrol_wait_time" || key == "patrolwaittime") kv::tryParseFloat(val, arch.perception.patrolWaitTime);
            else if (key == "alert_duration" || key == "alertduration") kv::tryParseFloat(val, arch.perception.alertDuration);
            else if (key == "lost_sight_timeout" || key == "lostsighttimeout") kv::tryParseFloat(val, arch.perception.lostSightTimeout);
            else if (key == "jump_speed" || key == "jumpspeed") kv::tryParseFloat(val, arch.perception.jumpSpeed);
            else if (key == "jump_cooldown" || key == "jumpcooldown") kv::tryParseFloat(val, arch.perception.jumpCooldown);
        } else if (root == "Counter") {
            if (key == "window") kv::tryParseFloat(val, arch.counter.window);
            else if (key == "active_frames" || key == "activeframes") kv::assignFramesSeconds(val, arch.counter.window);
            else if (key == "cooldown") kv::tryParseFloat(val, arch.counter.cooldown);
            else if (key == "whiff_stun" || key == "whiffstun") kv::tryParseFloat(val, arch.counter.whiffStun);
            else if (key == "player_stun" || key == "playerstun") kv::tryParseFloat(val, arch.counter.playerStun);
            else if (key == "damage") kv::tryParseFloat(val, arch.counter.damage);
            else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, arch.counter.knockX);
            else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, arch.counter.knockY);
            else if (key == "trigger_distance" || key == "triggerdistance") kv::tryParseFloat(val, arch.counter.triggerDistance);
        } else if (root == "DummyAttack") {
            if (key == "windup") kv::tryParseFloat(val, arch.stats.dummyWindup);
            else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, arch.stats.dummyWindup);
            else if (key == "active_time" || key == "activetime") kv::tryParseFloat(val, arch.stats.dummyActiveTime);
            else if (key == "active_frames" || key == "activeframes") kv::assignFramesSeconds(val, arch.stats.dummyActiveTime);
            else if (key == "recovery") kv::tryParseFloat(val, arch.stats.dummyRecovery);
            else if (key == "damage") kv::tryParseFloat(val, arch.stats.dummyDamage);
            else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, arch.stats.dummyKnockX);
            else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, arch.stats.dummyKnockY);
            else if (key == "stun") kv::tryParseFloat(val, arch.stats.dummyStun);
            else if (key == "cooldown") kv::tryParseFloat(val, arch.stats.dummyCooldown);
            else if (key == "hitbox_w" || key == "hitboxw") kv::tryParseFloat(val, arch.stats.dummyHbw);
            else if (key == "hitbox_h" || key == "hitboxh") kv::tryParseFloat(val, arch.stats.dummyHbh);
            else if (key == "hitbox_offset_y" || key == "hitboxoffsety") kv::tryParseFloat(val, arch.stats.dummyHbyOffset);
        } else if (root == "Combos" && stack.size() >= 3) {
            const std::string& comboId = stack[2];

            if (comboId == "1f2d1") {
                if (stack.size() == 3) {
                    if (key == "windup") kv::tryParseFloat(val, arch.starter1.windup);
                    else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, arch.starter1.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(arch.starter1.hit1, key, val);
                    else if (sub == "hit_f2") parseAttackField(arch.starter1.hitF2, key, val);
                    else if (sub == "hit_d1") parseAttackField(arch.starter1.hitD1, key, val);
                }
            } else if (comboId == "112") {
                if (stack.size() == 3) {
                    if (key == "windup") kv::tryParseFloat(val, arch.starter2.windup);
                    else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, arch.starter2.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(arch.starter2.hit1, key, val);
                    else if (sub == "hit2nd1") parseAttackField(arch.starter2.hit2nd1, key, val);
                    else if (sub == "hit2") parseAttackField(arch.starter2.hit2, key, val);
                }
            } else if (comboId == "21db1") {
                if (stack.size() == 3) {
                    if (key == "windup") kv::tryParseFloat(val, arch.starter3.windup);
                    else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, arch.starter3.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit2") parseAttackField(arch.starter3.hit2, key, val);
                    else if (sub == "hit1") parseAttackField(arch.starter3.hit1, key, val);
                    else if (sub == "dash_behind") {
                        if (key == "duration") kv::tryParseFloat(val, arch.starter3.dashBehindDuration);
                        else if (key == "duration_frames" || key == "durationframes") kv::assignFramesSeconds(val, arch.starter3.dashBehindDuration);
                        else if (key == "offset") kv::tryParseFloat(val, arch.starter3.dashBehindOffset);
                    } else if (sub == "sweep_db1") {
                        parseAttackField(arch.starter3.hitSweep, key, val);
                    }
                }
            } else if (comboId == "GrapplePull") {
                if (key == "windup") kv::tryParseFloat(val, arch.grapplePull.windup);
                else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, arch.grapplePull.windup);
                else if (key == "pull_speed" || key == "pullspeed") kv::tryParseFloat(val, arch.grapplePull.pullSpeed);
                else parseAttackField(arch.grapplePull.hit, key, val);
            } else if (comboId == "AirFollowup1") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") kv::tryParseFloat(val, arch.air1.jumpVelY);
                        else if (key == "duration") kv::tryParseFloat(val, arch.air1.jumpDuration);
                        else if (key == "duration_frames" || key == "durationframes") kv::assignFramesSeconds(val, arch.air1.jumpDuration);
                        else if (key == "horiz_factor" || key == "horizfactor") kv::tryParseFloat(val, arch.air1.jumpHorizFactor);
                        else if (key == "max_speed_x" || key == "maxspeedx") kv::tryParseFloat(val, arch.air1.jumpMaxSpeedX);
                    } else if (sub == "a1") {
                        parseAttackField(arch.air1.hitA1, key, val);
                    } else if (sub == "adw2") {
                        parseAttackField(arch.air1.hitAdw2, key, val);
                    }
                }
            } else if (comboId == "AirFollowup2") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "ground_advance") {
                        if (key == "duration") kv::tryParseFloat(val, arch.air2.groundAdvanceDuration);
                        else if (key == "duration_frames" || key == "durationframes") kv::assignFramesSeconds(val, arch.air2.groundAdvanceDuration);
                        else if (key == "advance_dist" || key == "advancedist") kv::tryParseFloat(val, arch.air2.groundAdvanceDist);
                    } else if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") kv::tryParseFloat(val, arch.air2.jumpVelY);
                        else if (key == "duration") kv::tryParseFloat(val, arch.air2.jumpDuration);
                        else if (key == "duration_frames" || key == "durationframes") kv::assignFramesSeconds(val, arch.air2.jumpDuration);
                    } else if (sub == "a1_first") {
                        parseAttackField(arch.air2.hitA1_1, key, val);
                    } else if (sub == "a1_second") {
                        parseAttackField(arch.air2.hitA1_2, key, val);
                    } else if (sub == "adw2_final") {
                        parseAttackField(arch.air2.hitAdw2, key, val);
                        if (key == "dive_vel_y" || key == "divevely") kv::tryParseFloat(val, arch.air2.diveSpeedY);
                        else if (key == "knock_x_vertical" || key == "knockxvertical") kv::tryParseFloat(val, arch.air2.finalKnockXVertical);
                    }
                }
            }
        }
    }

    static void applyKV(const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
        if (stack.size() < 2) return;
        const std::string& root = stack[1];

        if (root == "Stats") {
            if (key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.stats.maxHp);
            else if (key == "gravity") kv::tryParseFloat(val, _config.stats.gravity);
            else if (key == "whiff_recovery" || key == "whiffrecovery") kv::tryParseFloat(val, _config.stats.whiffRecovery);
            else if (key == "hit_recovery" || key == "hitrecovery") kv::tryParseFloat(val, _config.stats.hitRecovery);
            else if (key == "neutral_pursuit_speed" || key == "neutralpursuitspeed") kv::tryParseFloat(val, _config.stats.neutralPursuitSpeed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, _config.stats.approachSpeed);
            else if (key == "attack_cooldown" || key == "attackcooldown") kv::tryParseFloat(val, _config.stats.attackCooldown);
        } else if (root == "Normal") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.normal.hp);
            else if (key == "speed") kv::tryParseFloat(val, _config.normal.speed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, _config.normal.approachSpeed);
            else if (key == "damage_multiplier" || key == "damagemult") kv::tryParseFloat(val, _config.normal.damageMult);
            else if (key == "scale") kv::tryParseFloat(val, _config.normal.scale);
            else if (key == "can_counter" || key == "cancounter") _config.normal.canCounter = kv::parseBool(val, _config.normal.canCounter);
            else if (key == "can_block" || key == "canblock") _config.normal.canBlock = kv::parseBool(val, _config.normal.canBlock);
            else if (key == "can_dodge" || key == "candodge") _config.normal.canDodge = kv::parseBool(val, _config.normal.canDodge);
        } else if (root == "Tank") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.tank.hp);
            else if (key == "speed") kv::tryParseFloat(val, _config.tank.speed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, _config.tank.approachSpeed);
            else if (key == "damage_multiplier" || key == "damagemult") kv::tryParseFloat(val, _config.tank.damageMult);
            else if (key == "hitbox_scale" || key == "hitboxscale") kv::tryParseFloat(val, _config.tank.hitboxScale);
            else if (key == "scale") kv::tryParseFloat(val, _config.tank.scale);
            else if (key == "stun_multiplier" || key == "stunmult") kv::tryParseFloat(val, _config.tank.stunMult);
            else if (key == "knock_multiplier" || key == "knockmult") kv::tryParseFloat(val, _config.tank.knockMult);
            else if (key == "windup_multiplier" || key == "windupmult") kv::tryParseFloat(val, _config.tank.windupMult);
            else if (key == "can_counter" || key == "cancounter") _config.tank.canCounter = kv::parseBool(val, _config.tank.canCounter);
            else if (key == "can_block" || key == "canblock") _config.tank.canBlock = kv::parseBool(val, _config.tank.canBlock);
        } else if (root == "Duelist") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.duelist.hp);
            else if (key == "speed") kv::tryParseFloat(val, _config.duelist.speed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, _config.duelist.approachSpeed);
            else if (key == "damage_multiplier" || key == "damagemult") kv::tryParseFloat(val, _config.duelist.damageMult);
            else if (key == "scale") kv::tryParseFloat(val, _config.duelist.scale);
            else if (key == "can_counter" || key == "cancounter") _config.duelist.canCounter = kv::parseBool(val, _config.duelist.canCounter);
            else if (key == "can_block" || key == "canblock") _config.duelist.canBlock = kv::parseBool(val, _config.duelist.canBlock);
            else if (key == "can_dodge" || key == "candodge") _config.duelist.canDodge = kv::parseBool(val, _config.duelist.canDodge);
            else if (key == "aggressiveness") kv::tryParseFloat(val, _config.duelist.aggressiveness);
            else if (key == "intelligence") kv::tryParseFloat(val, _config.duelist.intelligence);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") kv::tryParseFloat(val, _config.duelist.dodgeCooldown);
            else if (key == "spacing_distance" || key == "spacingdistance") kv::tryParseFloat(val, _config.duelist.spacingDistance);
            else if (key == "idle_attack_delay" || key == "idleattackdelay") kv::tryParseFloat(val, _config.duelist.idleAttackDelay);
        } else if (root == "DuelistPassive") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.duelistPassive.hp);
            else if (key == "speed") kv::tryParseFloat(val, _config.duelistPassive.speed);
            else if (key == "approach_speed" || key == "approachspeed") kv::tryParseFloat(val, _config.duelistPassive.approachSpeed);
            else if (key == "damage_multiplier" || key == "damagemult") kv::tryParseFloat(val, _config.duelistPassive.damageMult);
            else if (key == "scale") kv::tryParseFloat(val, _config.duelistPassive.scale);
            else if (key == "can_counter" || key == "cancounter") _config.duelistPassive.canCounter = kv::parseBool(val, _config.duelistPassive.canCounter);
            else if (key == "can_block" || key == "canblock") _config.duelistPassive.canBlock = kv::parseBool(val, _config.duelistPassive.canBlock);
            else if (key == "can_dodge" || key == "candodge") _config.duelistPassive.canDodge = kv::parseBool(val, _config.duelistPassive.canDodge);
            else if (key == "aggressiveness") kv::tryParseFloat(val, _config.duelistPassive.aggressiveness);
            else if (key == "intelligence") kv::tryParseFloat(val, _config.duelistPassive.intelligence);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") kv::tryParseFloat(val, _config.duelistPassive.dodgeCooldown);
            else if (key == "spacing_distance" || key == "spacingdistance") kv::tryParseFloat(val, _config.duelistPassive.spacingDistance);
            else if (key == "invis_duration" || key == "invisduration") kv::tryParseFloat(val, _config.duelistPassive.invisDuration);
            else if (key == "invis_cooldown" || key == "inviscooldown") kv::tryParseFloat(val, _config.duelistPassive.invisCooldown);
        } else if (root == "DummyAttack") {
            if (key == "windup") kv::tryParseFloat(val, _config.stats.dummyWindup);
            else if (key == "windup_frames" || key == "windupframes") kv::assignFramesSeconds(val, _config.stats.dummyWindup);
            else if (key == "active_time" || key == "activetime") kv::tryParseFloat(val, _config.stats.dummyActiveTime);
            else if (key == "active_frames" || key == "activeframes") kv::assignFramesSeconds(val, _config.stats.dummyActiveTime);
            else if (key == "recovery") kv::tryParseFloat(val, _config.stats.dummyRecovery);
            else if (key == "damage") kv::tryParseFloat(val, _config.stats.dummyDamage);
            else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, _config.stats.dummyKnockX);
            else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, _config.stats.dummyKnockY);
            else if (key == "stun") kv::tryParseFloat(val, _config.stats.dummyStun);
            else if (key == "cooldown") kv::tryParseFloat(val, _config.stats.dummyCooldown);
            else if (key == "hitbox_w" || key == "hitboxw") kv::tryParseFloat(val, _config.stats.dummyHbw);
            else if (key == "hitbox_h" || key == "hitboxh") kv::tryParseFloat(val, _config.stats.dummyHbh);
            else if (key == "hitbox_offset_y" || key == "hitboxoffsety") kv::tryParseFloat(val, _config.stats.dummyHbyOffset);
        } else if (root == "Counter") {
            if (key == "window") kv::tryParseFloat(val, _config.counter.window);
            else if (key == "cooldown") kv::tryParseFloat(val, _config.counter.cooldown);
            else if (key == "whiff_stun" || key == "whiffstun") kv::tryParseFloat(val, _config.counter.whiffStun);
            else if (key == "player_stun" || key == "playerstun") kv::tryParseFloat(val, _config.counter.playerStun);
            else if (key == "damage") kv::tryParseFloat(val, _config.counter.damage);
            else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, _config.counter.knockX);
            else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, _config.counter.knockY);
            else if (key == "trigger_distance" || key == "triggerdistance") kv::tryParseFloat(val, _config.counter.triggerDistance);
        } else if (root == "Perception") {
            if (key == "view_distance" || key == "viewdistance") kv::tryParseFloat(val, _config.perception.viewDistance);
            else if (key == "rear_view_distance" || key == "rearviewdistance") kv::tryParseFloat(val, _config.perception.rearViewDistance);
            else if (key == "vertical_view_span" || key == "verticalviewspan") kv::tryParseFloat(val, _config.perception.verticalViewSpan);
            else if (key == "patrol_speed" || key == "patrolspeed") kv::tryParseFloat(val, _config.perception.patrolSpeed);
            else if (key == "patrol_radius" || key == "patrolradius") kv::tryParseFloat(val, _config.perception.patrolRadius);
            else if (key == "patrol_wait_time" || key == "patrolwaittime") kv::tryParseFloat(val, _config.perception.patrolWaitTime);
            else if (key == "alert_duration" || key == "alertduration") kv::tryParseFloat(val, _config.perception.alertDuration);
            else if (key == "lost_sight_timeout" || key == "lostsighttimeout") kv::tryParseFloat(val, _config.perception.lostSightTimeout);
            else if (key == "jump_speed" || key == "jumpspeed") kv::tryParseFloat(val, _config.perception.jumpSpeed);
            else if (key == "jump_cooldown" || key == "jumpcooldown") kv::tryParseFloat(val, _config.perception.jumpCooldown);
        } else if (root == "Combos" && stack.size() >= 3) {
            const std::string& comboId = stack[2];

            if (comboId == "1f2d1") {
                if (stack.size() == 3 && key == "windup") {
                    kv::tryParseFloat(val, _config.starter1.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(_config.starter1.hit1, key, val);
                    else if (sub == "hit_f2") parseAttackField(_config.starter1.hitF2, key, val);
                    else if (sub == "hit_d1") parseAttackField(_config.starter1.hitD1, key, val);
                }
            } else if (comboId == "112") {
                if (stack.size() == 3 && key == "windup") {
                    kv::tryParseFloat(val, _config.starter2.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(_config.starter2.hit1, key, val);
                    else if (sub == "hit2nd1") parseAttackField(_config.starter2.hit2nd1, key, val);
                    else if (sub == "hit2") parseAttackField(_config.starter2.hit2, key, val);
                }
            } else if (comboId == "21db1") {
                if (stack.size() == 3 && key == "windup") {
                    kv::tryParseFloat(val, _config.starter3.windup);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit2") parseAttackField(_config.starter3.hit2, key, val);
                    else if (sub == "hit1") parseAttackField(_config.starter3.hit1, key, val);
                    else if (sub == "dash_behind") {
                        if (key == "duration") kv::tryParseFloat(val, _config.starter3.dashBehindDuration);
                        else if (key == "offset") kv::tryParseFloat(val, _config.starter3.dashBehindOffset);
                    } else if (sub == "sweep_db1") {
                        parseAttackField(_config.starter3.hitSweep, key, val);
                    }
                }
            } else if (comboId == "AirFollowup1") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") kv::tryParseFloat(val, _config.air1.jumpVelY);
                        else if (key == "duration") kv::tryParseFloat(val, _config.air1.jumpDuration);
                        else if (key == "horiz_factor" || key == "horizfactor") kv::tryParseFloat(val, _config.air1.jumpHorizFactor);
                        else if (key == "max_speed_x" || key == "maxspeedx") kv::tryParseFloat(val, _config.air1.jumpMaxSpeedX);
                    } else if (sub == "a1") {
                        parseAttackField(_config.air1.hitA1, key, val);
                    } else if (sub == "adw2") {
                        parseAttackField(_config.air1.hitAdw2, key, val);
                    }
                }
            } else if (comboId == "AirFollowup2") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "ground_advance") {
                        if (key == "duration") kv::tryParseFloat(val, _config.air2.groundAdvanceDuration);
                        else if (key == "advance_dist" || key == "advancedist") kv::tryParseFloat(val, _config.air2.groundAdvanceDist);
                    } else if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") kv::tryParseFloat(val, _config.air2.jumpVelY);
                        else if (key == "duration") kv::tryParseFloat(val, _config.air2.jumpDuration);
                    } else if (sub == "a1_first") {
                        parseAttackField(_config.air2.hitA1_1, key, val);
                    } else if (sub == "a1_second") {
                        parseAttackField(_config.air2.hitA1_2, key, val);
                    } else if (sub == "adw2_final") {
                        parseAttackField(_config.air2.hitAdw2, key, val);
                        if (key == "dive_vel_y" || key == "divevely") kv::tryParseFloat(val, _config.air2.diveSpeedY);
                        else if (key == "knock_x_vertical" || key == "knockxvertical") kv::tryParseFloat(val, _config.air2.finalKnockXVertical);
                    }
                }
            }
        }
    }
};

inline const EnemyArchetypeData& EnemyConfig::getArchetype(int type) {
    ensureInitialized();
    switch (type) {
    case 0: return _config.normalArchetype;
    case 1: return _config.tankArchetype;
    case 2: return _config.duelistArchetype;
    case 3: return _config.duelistPassiveArchetype;
    case 4: return _config.dummyArchetype;
    default: return _config.normalArchetype;
    }
}
