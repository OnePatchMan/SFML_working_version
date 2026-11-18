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
    float damageMult = 1.0f;
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
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::vector<std::string> blockStack;
        std::string pendingBlock = "";

        std::string line;
        while (std::getline(file, line)) {
            auto tokens = tokenizeLine(line);
            if (tokens.empty()) continue;

            for (size_t t = 0; t < tokens.size(); ++t) {
                const std::string& tok = tokens[t];
                if (tok == "{") {
                    blockStack.push_back(pendingBlock);
                    pendingBlock.clear();
                } else if (tok == "}") {
                    if (!blockStack.empty()) blockStack.pop_back();
                } else {
                    if (t + 1 < tokens.size() && (tokens[t+1] != "{" && tokens[t+1] != "}")) {
                        std::string key = tok;
                        std::string val = tokens[t+1];
                        t++;
                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                        applyKV(blockStack, key, val);
                    } else {
                        pendingBlock = tok;
                    }
                }
            }
        }
        return true;
    }

    static bool loadArchetypeKVFile(const std::string& filepath, EnemyArchetypeData& arch) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::vector<std::string> blockStack;
        std::string pendingBlock = "";

        std::string line;
        while (std::getline(file, line)) {
            auto tokens = tokenizeLine(line);
            if (tokens.empty()) continue;

            for (size_t t = 0; t < tokens.size(); ++t) {
                const std::string& tok = tokens[t];
                if (tok == "{") {
                    blockStack.push_back(pendingBlock);
                    pendingBlock.clear();
                } else if (tok == "}") {
                    if (!blockStack.empty()) blockStack.pop_back();
                } else {
                    if (t + 1 < tokens.size() && (tokens[t+1] != "{" && tokens[t+1] != "}")) {
                        std::string key = tok;
                        std::string val = tokens[t+1];
                        t++;
                        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

                        applyArchetypeKV(arch, blockStack, key, val);
                    } else {
                        pendingBlock = tok;
                    }
                }
            }
        }
        return true;
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
        if (key == "active_time" || key == "activetime") st.activeTime = std::stof(val);
        else if (key == "active_frames" || key == "activeframes") st.activeTime = std::stof(val) / 60.0f;
        else if (key == "duration_frames" || key == "durationframes" || key == "total_frames") {
            float totalTime = std::stof(val) / 60.0f;
            st.recoveryTime = std::max(0.01f, totalTime - st.windup - st.activeTime);
        }
        else if (key == "recovery_time" || key == "recoverytime" || key == "recovery") st.recoveryTime = std::stof(val);
        else if (key == "damage") st.damage = std::stof(val);
        else if (key == "knock_x" || key == "knockx") st.knockX = std::stof(val);
        else if (key == "knock_y" || key == "knocky") st.knockY = std::stof(val);
        else if (key == "stun") st.stun = std::stof(val);
        else if (key == "lunge") st.lunge = std::stof(val);
        else if (key == "hitbox_w" || key == "hitboxw") st.hbw = std::stof(val);
        else if (key == "hitbox_h" || key == "hitboxh") st.hbh = std::stof(val);
        else if (key == "hitbox_offset_y" || key == "hitboxoffsety") st.hbyOffset = std::stof(val);
    }

    static void applyArchetypeKV(EnemyArchetypeData& arch, const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
        if (stack.size() < 2) return;
        const std::string& root = stack[1];

        if (root == "Stats") {
            if (key == "max_hp" || key == "hp" || key == "maxhp") arch.hp = std::stof(val);
            else if (key == "speed") arch.speed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") arch.approachSpeed = std::stof(val);
            else if (key == "damage_multiplier" || key == "damagemultiplier" || key == "damagemult") arch.damageMult = std::stof(val);
            else if (key == "scale") arch.scale = std::stof(val);
            else if (key == "hitbox_scale" || key == "hitboxscale") arch.hitboxScale = std::stof(val);
            else if (key == "stun_multiplier" || key == "stunmult") arch.stunMult = std::stof(val);
            else if (key == "knock_multiplier" || key == "knockmult") arch.knockMult = std::stof(val);
            else if (key == "windup_multiplier" || key == "windupmult") arch.windupMult = std::stof(val);
            else if (key == "can_counter" || key == "cancounter") arch.canCounter = (std::stoi(val) != 0);
            else if (key == "can_block" || key == "canblock") arch.canBlock = (std::stoi(val) != 0);
            else if (key == "can_dodge" || key == "candodge") arch.canDodge = (std::stoi(val) != 0);
            else if (key == "aggressiveness") arch.aggressiveness = std::stof(val);
            else if (key == "intelligence") arch.intelligence = std::stof(val);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") arch.dodgeCooldown = std::stof(val);
            else if (key == "spacing_distance" || key == "spacingdistance") arch.spacingDistance = std::stof(val);
            else if (key == "idle_attack_delay" || key == "idleattackdelay") arch.idleAttackDelay = std::stof(val);
            else if (key == "invis_duration" || key == "invisduration") arch.invisDuration = std::stof(val);
            else if (key == "invis_cooldown" || key == "inviscooldown") arch.invisCooldown = std::stof(val);
            else if (key == "whiff_recovery" || key == "whiffrecovery") arch.stats.whiffRecovery = std::stof(val);
            else if (key == "hit_recovery" || key == "hitrecovery") arch.stats.hitRecovery = std::stof(val);
            else if (key == "attack_cooldown" || key == "attackcooldown") arch.stats.attackCooldown = std::stof(val);
            else if (key == "gravity") arch.stats.gravity = std::stof(val);
        } else if (root == "Perception") {
            if (key == "view_distance" || key == "viewdistance") arch.perception.viewDistance = std::stof(val);
            else if (key == "rear_view_distance" || key == "rearviewdistance") arch.perception.rearViewDistance = std::stof(val);
            else if (key == "vertical_view_span" || key == "verticalviewspan") arch.perception.verticalViewSpan = std::stof(val);
            else if (key == "patrol_speed" || key == "patrolspeed") arch.perception.patrolSpeed = std::stof(val);
            else if (key == "patrol_radius" || key == "patrolradius") arch.perception.patrolRadius = std::stof(val);
            else if (key == "patrol_wait_time" || key == "patrolwaittime") arch.perception.patrolWaitTime = std::stof(val);
            else if (key == "alert_duration" || key == "alertduration") arch.perception.alertDuration = std::stof(val);
            else if (key == "lost_sight_timeout" || key == "lostsighttimeout") arch.perception.lostSightTimeout = std::stof(val);
            else if (key == "jump_speed" || key == "jumpspeed") arch.perception.jumpSpeed = std::stof(val);
            else if (key == "jump_cooldown" || key == "jumpcooldown") arch.perception.jumpCooldown = std::stof(val);
        } else if (root == "Counter") {
            if (key == "window") arch.counter.window = std::stof(val);
            else if (key == "active_frames" || key == "activeframes") arch.counter.window = std::stof(val) / 60.0f;
            else if (key == "cooldown") arch.counter.cooldown = std::stof(val);
            else if (key == "whiff_stun" || key == "whiffstun") arch.counter.whiffStun = std::stof(val);
            else if (key == "player_stun" || key == "playerstun") arch.counter.playerStun = std::stof(val);
            else if (key == "damage") arch.counter.damage = std::stof(val);
            else if (key == "knock_x" || key == "knockx") arch.counter.knockX = std::stof(val);
            else if (key == "knock_y" || key == "knocky") arch.counter.knockY = std::stof(val);
            else if (key == "trigger_distance" || key == "triggerdistance") arch.counter.triggerDistance = std::stof(val);
        } else if (root == "DummyAttack") {
            if (key == "windup") arch.stats.dummyWindup = std::stof(val);
            else if (key == "windup_frames" || key == "windupframes") arch.stats.dummyWindup = std::stof(val) / 60.0f;
            else if (key == "active_time" || key == "activetime") arch.stats.dummyActiveTime = std::stof(val);
            else if (key == "active_frames" || key == "activeframes") arch.stats.dummyActiveTime = std::stof(val) / 60.0f;
            else if (key == "recovery") arch.stats.dummyRecovery = std::stof(val);
            else if (key == "damage") arch.stats.dummyDamage = std::stof(val);
            else if (key == "knock_x" || key == "knockx") arch.stats.dummyKnockX = std::stof(val);
            else if (key == "knock_y" || key == "knocky") arch.stats.dummyKnockY = std::stof(val);
            else if (key == "stun") arch.stats.dummyStun = std::stof(val);
            else if (key == "cooldown") arch.stats.dummyCooldown = std::stof(val);
            else if (key == "hitbox_w" || key == "hitboxw") arch.stats.dummyHbw = std::stof(val);
            else if (key == "hitbox_h" || key == "hitboxh") arch.stats.dummyHbh = std::stof(val);
            else if (key == "hitbox_offset_y" || key == "hitboxoffsety") arch.stats.dummyHbyOffset = std::stof(val);
        } else if (root == "Combos" && stack.size() >= 3) {
            const std::string& comboId = stack[2];

            if (comboId == "1f2d1") {
                if (stack.size() == 3) {
                    if (key == "windup") arch.starter1.windup = std::stof(val);
                    else if (key == "windup_frames" || key == "windupframes") arch.starter1.windup = std::stof(val) / 60.0f;
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(arch.starter1.hit1, key, val);
                    else if (sub == "hit_f2") parseAttackField(arch.starter1.hitF2, key, val);
                    else if (sub == "hit_d1") parseAttackField(arch.starter1.hitD1, key, val);
                }
            } else if (comboId == "112") {
                if (stack.size() == 3) {
                    if (key == "windup") arch.starter2.windup = std::stof(val);
                    else if (key == "windup_frames" || key == "windupframes") arch.starter2.windup = std::stof(val) / 60.0f;
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(arch.starter2.hit1, key, val);
                    else if (sub == "hit2nd1") parseAttackField(arch.starter2.hit2nd1, key, val);
                    else if (sub == "hit2") parseAttackField(arch.starter2.hit2, key, val);
                }
            } else if (comboId == "21db1") {
                if (stack.size() == 3) {
                    if (key == "windup") arch.starter3.windup = std::stof(val);
                    else if (key == "windup_frames" || key == "windupframes") arch.starter3.windup = std::stof(val) / 60.0f;
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit2") parseAttackField(arch.starter3.hit2, key, val);
                    else if (sub == "hit1") parseAttackField(arch.starter3.hit1, key, val);
                    else if (sub == "dash_behind") {
                        if (key == "duration") arch.starter3.dashBehindDuration = std::stof(val);
                        else if (key == "duration_frames" || key == "durationframes") arch.starter3.dashBehindDuration = std::stof(val) / 60.0f;
                        else if (key == "offset") arch.starter3.dashBehindOffset = std::stof(val);
                    } else if (sub == "sweep_db1") {
                        parseAttackField(arch.starter3.hitSweep, key, val);
                    }
                }
            } else if (comboId == "GrapplePull") {
                if (key == "windup") arch.grapplePull.windup = std::stof(val);
                else if (key == "windup_frames" || key == "windupframes") arch.grapplePull.windup = std::stof(val) / 60.0f;
                else if (key == "pull_speed" || key == "pullspeed") arch.grapplePull.pullSpeed = std::stof(val);
                else parseAttackField(arch.grapplePull.hit, key, val);
            } else if (comboId == "AirFollowup1") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") arch.air1.jumpVelY = std::stof(val);
                        else if (key == "duration") arch.air1.jumpDuration = std::stof(val);
                        else if (key == "duration_frames" || key == "durationframes") arch.air1.jumpDuration = std::stof(val) / 60.0f;
                        else if (key == "horiz_factor" || key == "horizfactor") arch.air1.jumpHorizFactor = std::stof(val);
                        else if (key == "max_speed_x" || key == "maxspeedx") arch.air1.jumpMaxSpeedX = std::stof(val);
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
                        if (key == "duration") arch.air2.groundAdvanceDuration = std::stof(val);
                        else if (key == "duration_frames" || key == "durationframes") arch.air2.groundAdvanceDuration = std::stof(val) / 60.0f;
                        else if (key == "advance_dist" || key == "advancedist") arch.air2.groundAdvanceDist = std::stof(val);
                    } else if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") arch.air2.jumpVelY = std::stof(val);
                        else if (key == "duration") arch.air2.jumpDuration = std::stof(val);
                        else if (key == "duration_frames" || key == "durationframes") arch.air2.jumpDuration = std::stof(val) / 60.0f;
                    } else if (sub == "a1_first") {
                        parseAttackField(arch.air2.hitA1_1, key, val);
                    } else if (sub == "a1_second") {
                        parseAttackField(arch.air2.hitA1_2, key, val);
                    } else if (sub == "adw2_final") {
                        parseAttackField(arch.air2.hitAdw2, key, val);
                        if (key == "dive_vel_y" || key == "divevely") arch.air2.diveSpeedY = std::stof(val);
                        else if (key == "knock_x_vertical" || key == "knockxvertical") arch.air2.finalKnockXVertical = std::stof(val);
                    }
                }
            }
        }
    }

    static void applyKV(const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
        if (stack.size() < 2) return;
        const std::string& root = stack[1];

        if (root == "Stats") {
            if (key == "max_hp" || key == "maxhp") _config.stats.maxHp = std::stof(val);
            else if (key == "gravity") _config.stats.gravity = std::stof(val);
            else if (key == "whiff_recovery" || key == "whiffrecovery") _config.stats.whiffRecovery = std::stof(val);
            else if (key == "hit_recovery" || key == "hitrecovery") _config.stats.hitRecovery = std::stof(val);
            else if (key == "neutral_pursuit_speed" || key == "neutralpursuitspeed") _config.stats.neutralPursuitSpeed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") _config.stats.approachSpeed = std::stof(val);
            else if (key == "attack_cooldown" || key == "attackcooldown") _config.stats.attackCooldown = std::stof(val);
        } else if (root == "Normal") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") _config.normal.hp = std::stof(val);
            else if (key == "speed") _config.normal.speed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") _config.normal.approachSpeed = std::stof(val);
            else if (key == "damage_multiplier" || key == "damagemult") _config.normal.damageMult = std::stof(val);
            else if (key == "scale") _config.normal.scale = std::stof(val);
            else if (key == "can_counter" || key == "cancounter") _config.normal.canCounter = (std::stoi(val) != 0);
            else if (key == "can_block" || key == "canblock") _config.normal.canBlock = (std::stoi(val) != 0);
            else if (key == "can_dodge" || key == "candodge") _config.normal.canDodge = (std::stoi(val) != 0);
        } else if (root == "Tank") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") _config.tank.hp = std::stof(val);
            else if (key == "speed") _config.tank.speed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") _config.tank.approachSpeed = std::stof(val);
            else if (key == "damage_multiplier" || key == "damagemult") _config.tank.damageMult = std::stof(val);
            else if (key == "hitbox_scale" || key == "hitboxscale") _config.tank.hitboxScale = std::stof(val);
            else if (key == "scale") _config.tank.scale = std::stof(val);
            else if (key == "stun_multiplier" || key == "stunmult") _config.tank.stunMult = std::stof(val);
            else if (key == "knock_multiplier" || key == "knockmult") _config.tank.knockMult = std::stof(val);
            else if (key == "windup_multiplier" || key == "windupmult") _config.tank.windupMult = std::stof(val);
            else if (key == "can_counter" || key == "cancounter") _config.tank.canCounter = (std::stoi(val) != 0);
            else if (key == "can_block" || key == "canblock") _config.tank.canBlock = (std::stoi(val) != 0);
        } else if (root == "Duelist") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") _config.duelist.hp = std::stof(val);
            else if (key == "speed") _config.duelist.speed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") _config.duelist.approachSpeed = std::stof(val);
            else if (key == "damage_multiplier" || key == "damagemult") _config.duelist.damageMult = std::stof(val);
            else if (key == "scale") _config.duelist.scale = std::stof(val);
            else if (key == "can_counter" || key == "cancounter") _config.duelist.canCounter = (std::stoi(val) != 0);
            else if (key == "can_block" || key == "canblock") _config.duelist.canBlock = (std::stoi(val) != 0);
            else if (key == "can_dodge" || key == "candodge") _config.duelist.canDodge = (std::stoi(val) != 0);
            else if (key == "aggressiveness") _config.duelist.aggressiveness = std::stof(val);
            else if (key == "intelligence") _config.duelist.intelligence = std::stof(val);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") _config.duelist.dodgeCooldown = std::stof(val);
            else if (key == "spacing_distance" || key == "spacingdistance") _config.duelist.spacingDistance = std::stof(val);
            else if (key == "idle_attack_delay" || key == "idleattackdelay") _config.duelist.idleAttackDelay = std::stof(val);
        } else if (root == "DuelistPassive") {
            if (key == "hp" || key == "max_hp" || key == "maxhp") _config.duelistPassive.hp = std::stof(val);
            else if (key == "speed") _config.duelistPassive.speed = std::stof(val);
            else if (key == "approach_speed" || key == "approachspeed") _config.duelistPassive.approachSpeed = std::stof(val);
            else if (key == "damage_multiplier" || key == "damagemult") _config.duelistPassive.damageMult = std::stof(val);
            else if (key == "scale") _config.duelistPassive.scale = std::stof(val);
            else if (key == "can_counter" || key == "cancounter") _config.duelistPassive.canCounter = (std::stoi(val) != 0);
            else if (key == "can_block" || key == "canblock") _config.duelistPassive.canBlock = (std::stoi(val) != 0);
            else if (key == "can_dodge" || key == "candodge") _config.duelistPassive.canDodge = (std::stoi(val) != 0);
            else if (key == "aggressiveness") _config.duelistPassive.aggressiveness = std::stof(val);
            else if (key == "intelligence") _config.duelistPassive.intelligence = std::stof(val);
            else if (key == "dodge_cooldown" || key == "dodgecooldown") _config.duelistPassive.dodgeCooldown = std::stof(val);
            else if (key == "spacing_distance" || key == "spacingdistance") _config.duelistPassive.spacingDistance = std::stof(val);
            else if (key == "invis_duration" || key == "invisduration") _config.duelistPassive.invisDuration = std::stof(val);
            else if (key == "invis_cooldown" || key == "inviscooldown") _config.duelistPassive.invisCooldown = std::stof(val);
        } else if (root == "DummyAttack") {
            if (key == "windup") _config.stats.dummyWindup = std::stof(val);
            else if (key == "windup_frames" || key == "windupframes") _config.stats.dummyWindup = std::stof(val) / 60.0f;
            else if (key == "active_time" || key == "activetime") _config.stats.dummyActiveTime = std::stof(val);
            else if (key == "active_frames" || key == "activeframes") _config.stats.dummyActiveTime = std::stof(val) / 60.0f;
            else if (key == "recovery") _config.stats.dummyRecovery = std::stof(val);
            else if (key == "damage") _config.stats.dummyDamage = std::stof(val);
            else if (key == "knock_x" || key == "knockx") _config.stats.dummyKnockX = std::stof(val);
            else if (key == "knock_y" || key == "knocky") _config.stats.dummyKnockY = std::stof(val);
            else if (key == "stun") _config.stats.dummyStun = std::stof(val);
            else if (key == "cooldown") _config.stats.dummyCooldown = std::stof(val);
            else if (key == "hitbox_w" || key == "hitboxw") _config.stats.dummyHbw = std::stof(val);
            else if (key == "hitbox_h" || key == "hitboxh") _config.stats.dummyHbh = std::stof(val);
            else if (key == "hitbox_offset_y" || key == "hitboxoffsety") _config.stats.dummyHbyOffset = std::stof(val);
        } else if (root == "Counter") {
            if (key == "window") _config.counter.window = std::stof(val);
            else if (key == "cooldown") _config.counter.cooldown = std::stof(val);
            else if (key == "whiff_stun" || key == "whiffstun") _config.counter.whiffStun = std::stof(val);
            else if (key == "player_stun" || key == "playerstun") _config.counter.playerStun = std::stof(val);
            else if (key == "damage") _config.counter.damage = std::stof(val);
            else if (key == "knock_x" || key == "knockx") _config.counter.knockX = std::stof(val);
            else if (key == "knock_y" || key == "knocky") _config.counter.knockY = std::stof(val);
            else if (key == "trigger_distance" || key == "triggerdistance") _config.counter.triggerDistance = std::stof(val);
        } else if (root == "Perception") {
            if (key == "view_distance" || key == "viewdistance") _config.perception.viewDistance = std::stof(val);
            else if (key == "rear_view_distance" || key == "rearviewdistance") _config.perception.rearViewDistance = std::stof(val);
            else if (key == "vertical_view_span" || key == "verticalviewspan") _config.perception.verticalViewSpan = std::stof(val);
            else if (key == "patrol_speed" || key == "patrolspeed") _config.perception.patrolSpeed = std::stof(val);
            else if (key == "patrol_radius" || key == "patrolradius") _config.perception.patrolRadius = std::stof(val);
            else if (key == "patrol_wait_time" || key == "patrolwaittime") _config.perception.patrolWaitTime = std::stof(val);
            else if (key == "alert_duration" || key == "alertduration") _config.perception.alertDuration = std::stof(val);
            else if (key == "lost_sight_timeout" || key == "lostsighttimeout") _config.perception.lostSightTimeout = std::stof(val);
            else if (key == "jump_speed" || key == "jumpspeed") _config.perception.jumpSpeed = std::stof(val);
            else if (key == "jump_cooldown" || key == "jumpcooldown") _config.perception.jumpCooldown = std::stof(val);
        } else if (root == "Combos" && stack.size() >= 3) {
            const std::string& comboId = stack[2];

            if (comboId == "1f2d1") {
                if (stack.size() == 3 && key == "windup") {
                    _config.starter1.windup = std::stof(val);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(_config.starter1.hit1, key, val);
                    else if (sub == "hit_f2") parseAttackField(_config.starter1.hitF2, key, val);
                    else if (sub == "hit_d1") parseAttackField(_config.starter1.hitD1, key, val);
                }
            } else if (comboId == "112") {
                if (stack.size() == 3 && key == "windup") {
                    _config.starter2.windup = std::stof(val);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit1") parseAttackField(_config.starter2.hit1, key, val);
                    else if (sub == "hit2nd1") parseAttackField(_config.starter2.hit2nd1, key, val);
                    else if (sub == "hit2") parseAttackField(_config.starter2.hit2, key, val);
                }
            } else if (comboId == "21db1") {
                if (stack.size() == 3 && key == "windup") {
                    _config.starter3.windup = std::stof(val);
                } else if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "hit2") parseAttackField(_config.starter3.hit2, key, val);
                    else if (sub == "hit1") parseAttackField(_config.starter3.hit1, key, val);
                    else if (sub == "dash_behind") {
                        if (key == "duration") _config.starter3.dashBehindDuration = std::stof(val);
                        else if (key == "offset") _config.starter3.dashBehindOffset = std::stof(val);
                    } else if (sub == "sweep_db1") {
                        parseAttackField(_config.starter3.hitSweep, key, val);
                    }
                }
            } else if (comboId == "AirFollowup1") {
                if (stack.size() >= 4) {
                    const std::string& sub = stack[3];
                    if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") _config.air1.jumpVelY = std::stof(val);
                        else if (key == "duration") _config.air1.jumpDuration = std::stof(val);
                        else if (key == "horiz_factor" || key == "horizfactor") _config.air1.jumpHorizFactor = std::stof(val);
                        else if (key == "max_speed_x" || key == "maxspeedx") _config.air1.jumpMaxSpeedX = std::stof(val);
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
                        if (key == "duration") _config.air2.groundAdvanceDuration = std::stof(val);
                        else if (key == "advance_dist" || key == "advancedist") _config.air2.groundAdvanceDist = std::stof(val);
                    } else if (sub == "chase_jump") {
                        if (key == "jump_vel_y" || key == "jumpvely") _config.air2.jumpVelY = std::stof(val);
                        else if (key == "duration") _config.air2.jumpDuration = std::stof(val);
                    } else if (sub == "a1_first") {
                        parseAttackField(_config.air2.hitA1_1, key, val);
                    } else if (sub == "a1_second") {
                        parseAttackField(_config.air2.hitA1_2, key, val);
                    } else if (sub == "adw2_final") {
                        parseAttackField(_config.air2.hitAdw2, key, val);
                        if (key == "dive_vel_y" || key == "divevely") _config.air2.diveSpeedY = std::stof(val);
                        else if (key == "knock_x_vertical" || key == "knockxvertical") _config.air2.finalKnockXVertical = std::stof(val);
                    }
                }
            }
        }
    }

    static std::vector<std::string> tokenizeLine(const std::string& line) {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' || line[i] == '\n' || line[i] == '=' || line[i] == ':')) {
                i++;
            }
            if (i >= line.size()) break;

            if (line[i] == '/' && i + 1 < line.size() && line[i+1] == '/') break;
            if (line[i] == '#' || line[i] == ';') break;

            if (line[i] == '"') {
                i++;
                size_t start = i;
                while (i < line.size() && line[i] != '"') {
                    i++;
                }
                tokens.push_back(line.substr(start, i - start));
                if (i < line.size() && line[i] == '"') i++;
            } else if (line[i] == '{' || line[i] == '}') {
                tokens.push_back(std::string(1, line[i]));
                i++;
            } else {
                size_t start = i;
                while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r' && line[i] != '\n' && line[i] != '=' && line[i] != ':' && line[i] != '{' && line[i] != '}') {
                    i++;
                }
                tokens.push_back(line.substr(start, i - start));
            }
        }
        return tokens;
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
