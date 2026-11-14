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
    EnemyAttackStats hit1{0.0f, 0.12f, 0.06f, 2.0f, 160.0f, -25.0f, 0.24f, 0.0f, 55.0f, 42.0f, -18.0f, false};
    EnemyAttackStats hitF2{0.0f, 0.16f, 0.06f, 3.5f, 460.0f, -45.0f, 0.35f, 18.0f, 70.0f, 46.0f, -20.0f, false};
    EnemyAttackStats hitD1{0.0f, 0.18f, 0.06f, 5.0f, 220.0f, -435.0f, 0.48f, 30.0f, 75.0f, 70.0f, -42.0f, false};
};

struct Starter112Config {
    float windup = 0.06f;
    EnemyAttackStats hit1{0.0f, 0.12f, 0.05f, 2.0f, 150.0f, -25.0f, 0.24f, 0.0f, 55.0f, 42.0f, -18.0f, false};
    EnemyAttackStats hit2nd1{0.0f, 0.12f, 0.05f, 2.2f, 160.0f, -25.0f, 0.24f, 14.0f, 58.0f, 44.0f, -18.0f, false};
    EnemyAttackStats hit2{0.0f, 0.18f, 0.06f, 4.5f, 180.0f, -435.0f, 0.45f, 18.0f, 70.0f, 68.0f, -40.0f, false};
};

struct Starter21DB1Config {
    float windup = 0.09f;
    EnemyAttackStats hit2{0.0f, 0.16f, 0.06f, 3.5f, 350.0f, -35.0f, 0.30f, 0.0f, 65.0f, 46.0f, -18.0f, false};
    EnemyAttackStats hit1{0.0f, 0.14f, 0.06f, 3.0f, 850.0f, -35.0f, 0.35f, 16.0f, 60.0f, 44.0f, -18.0f, false};
    float dashBehindDuration = 0.08f;
    float dashBehindOffset = 38.0f;
    EnemyAttackStats hitSweep{0.0f, 0.15f, 0.06f, 4.8f, 0.0f, -435.0f, 0.48f, 0.0f, 70.0f, 50.0f, -10.0f, false};
};

struct AirFollowup1Config {
    float jumpVelY = -880.0f;
    float jumpDuration = 0.32f;
    float jumpHorizFactor = 6.5f;
    float jumpMaxSpeedX = 500.0f;
    EnemyAttackStats hitA1{0.0f, 0.14f, 0.06f, 2.0f, 60.0f, -40.0f, 0.35f, 0.0f, 70.0f, 55.0f, -25.0f, false};
    EnemyAttackStats hitAdw2{0.0f, 0.18f, 0.06f, 4.5f, 440.0f, 550.0f, 0.42f, 0.0f, 75.0f, 60.0f, -15.0f, true};
};

struct AirFollowup2Config {
    float groundAdvanceDuration = 0.18f;
    float groundAdvanceDist = 72.0f;
    float jumpVelY = -860.0f;
    float jumpDuration = 0.32f;
    EnemyAttackStats hitA1_1{0.0f, 0.12f, 0.05f, 2.0f, 60.0f, -40.0f, 0.35f, 0.0f, 70.0f, 55.0f, -25.0f, false};
    EnemyAttackStats hitA1_2{0.0f, 0.12f, 0.05f, 2.0f, 60.0f, -40.0f, 0.35f, 0.0f, 65.0f, 50.0f, -25.0f, false};
    EnemyAttackStats hitAdw2{0.0f, 0.30f, 0.50f, 5.5f, 1150.0f, -280.0f, 0.52f, 0.0f, 80.0f, 65.0f, -15.0f, true};
    float diveSpeedY = 1200.0f;
    float finalKnockXVertical = 600.0f;
};

struct EnemyCounterConfig {
    float window = 0.35f;
    float cooldown = 3.50f;
    float whiffStun = 1.20f;
    float playerStun = 1.40f;
    float damage = 7.0f;
    float knockX = 320.0f;
    float knockY = -40.0f;
    float triggerDistance = 65.0f;
};

struct EnemyGeneralStats {
    float maxHp = 100.0f;
    float gravity = 2600.0f;
    float whiffRecovery = 0.50f;
    float hitRecovery = 0.20f;
    float neutralPursuitSpeed = 250.0f;
    float approachSpeed = 320.0f;
    float attackCooldown = 0.75f;
    float dummyWindup = 0.38f;
    float dummyActiveTime = 0.16f;
    float dummyRecovery = 0.20f;
    float dummyDamage = 3.0f;
    float dummyKnockX = 250.0f;
    float dummyKnockY = -30.0f;
    float dummyStun = 0.25f;
    float dummyCooldown = 2.4f;
    float dummyHbw = 55.0f;
    float dummyHbh = 42.0f;
    float dummyHbyOffset = -18.0f;
};

struct EnemyAllConfig {
    EnemyGeneralStats stats;
    Starter1F2D1Config starter1;
    Starter112Config starter2;
    Starter21DB1Config starter3;
    AirFollowup1Config air1;
    AirFollowup2Config air2;
    EnemyCounterConfig counter;
};

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

    static void set(const EnemyAllConfig& cfg) {
        ensureInitialized();
        _config = cfg;
    }

    static bool reload() {
        _initialized = false;
        ensureInitialized();
        std::cout << "[KV CONFIG] Конфигурация врагов и комбо (enemy.kv) перезагружена!" << std::endl;
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

private:
    inline static EnemyAllConfig _config;
    inline static bool _initialized = false;

    static void ensureInitialized() {
        if (_initialized) return;
        _initialized = true;
        _config = EnemyAllConfig{}; // Значения по умолчанию

        std::string kvPath = resolveAssetPath("assets/config/enemy.kv");
        if (std::filesystem::exists(kvPath)) {
            loadKVFile(kvPath);
        }
    }

    static void parseAttackField(EnemyAttackStats& st, const std::string& key, const std::string& val) {
        if (key == "active_time" || key == "activetime") st.activeTime = std::stof(val);
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
        } else if (root == "DummyAttack") {
            if (key == "windup") _config.stats.dummyWindup = std::stof(val);
            else if (key == "active_time" || key == "activetime") _config.stats.dummyActiveTime = std::stof(val);
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
