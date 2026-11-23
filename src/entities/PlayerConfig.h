#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include "core/Common.h"

struct PlayerStatsConfig {
    float maxHp = 100.0f;
    float moveSpeed = 400.0f;
    float sprintSpeed = 600.0f; // Ускорение (бег) после рывка на земле
    float gravity = 3000.0f;
    float jumpSpeed = 900.0f;
    float maxFallSpeed = 1800.0f;
    float turnDelay = 0.020f; // Окно буфера задней атаки перед разворотом (~1 кадр при 60 FPS, 0.020s)

    float dashSpeed = 1250.0f;
    float dashDuration = 0.09f;
    float dashCooldown = 2.00f;
    float iframeDuration = 0.13f;

    float spotDodgeDuration = 0.22f;
    float spotDodgeCooldown = 2.00f;
    float perfectDodgeWindow = 0.11f;
    float perfectDodgeEmpower = 1.40f;

    float halfW = 20.0f;
    float topOffset = 25.0f;
    float bottomOffset = 30.0f;

    // Стойка контр-атаки / парирования на [Q]
    float counterWindow = 0.30f;
    float counterCooldown = 1.20f;
    float counterWhiffStun = 1.00f;
    float counterEnemyStun = 1.50f;
    float counterDamage = 6.0f;
    float counterKnockX = 350.0f;
    float counterKnockY = -50.0f;
};

class PlayerConfig {
public:
    static const PlayerStatsConfig& get() {
        ensureInitialized();
        return _config;
    }

    static PlayerStatsConfig& getMutable() {
        ensureInitialized();
        return _config;
    }

    static void set(const PlayerStatsConfig& cfg) {
        ensureInitialized();
        _config = cfg;
    }

    static bool reload() {
        _initialized = false;
        ensureInitialized();
        std::cout << "[KV CONFIG] Конфигурация игрока (player.kv) перезагружена!" << std::endl;
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

                        if (blockStack.size() >= 2) {
                            const std::string& section = blockStack[1];
                            if (section == "Stats") {
                                if (key == "max_hp" || key == "maxhp") _config.maxHp = std::stof(val);
                                else if (key == "move_speed" || key == "movespeed") _config.moveSpeed = std::stof(val);
                                else if (key == "sprint_speed" || key == "sprintspeed") _config.sprintSpeed = std::stof(val);
                                else if (key == "gravity") _config.gravity = std::stof(val);
                                else if (key == "jump_speed" || key == "jumpspeed") _config.jumpSpeed = std::stof(val);
                                else if (key == "max_fall_speed" || key == "maxfallspeed") _config.maxFallSpeed = std::stof(val);
                                else if (key == "turn_delay" || key == "turndelay") _config.turnDelay = std::stof(val);
                            } else if (section == "Dash") {
                                if (key == "dash_speed" || key == "dashspeed") _config.dashSpeed = std::stof(val);
                                else if (key == "dash_duration" || key == "dashduration") _config.dashDuration = std::stof(val);
                                else if (key == "dash_cooldown" || key == "dashcooldown") _config.dashCooldown = std::stof(val);
                                else if (key == "iframe_duration" || key == "iframeduration") _config.iframeDuration = std::stof(val);
                            } else if (section == "SpotDodge") {
                                if (key == "spot_dodge_duration" || key == "spotdodgeduration") _config.spotDodgeDuration = std::stof(val);
                                else if (key == "spot_dodge_cooldown" || key == "spotdodgecooldown") _config.spotDodgeCooldown = std::stof(val);
                                else if (key == "perfect_dodge_window" || key == "perfectdodgewindow") _config.perfectDodgeWindow = std::stof(val);
                                else if (key == "perfect_dodge_empower" || key == "perfectdodgeempower") _config.perfectDodgeEmpower = std::stof(val);
                            } else if (section == "Collision") {
                                if (key == "half_w" || key == "halfw") _config.halfW = std::stof(val);
                                else if (key == "top_offset" || key == "topoffset") _config.topOffset = std::stof(val);
                                else if (key == "bottom_offset" || key == "bottomoffset") _config.bottomOffset = std::stof(val);
                            } else if (section == "Counter") {
                                if (key == "window") _config.counterWindow = std::stof(val);
                                else if (key == "cooldown") _config.counterCooldown = std::stof(val);
                                else if (key == "whiff_stun" || key == "whiffstun") _config.counterWhiffStun = std::stof(val);
                                else if (key == "enemy_stun" || key == "enemystun") _config.counterEnemyStun = std::stof(val);
                                else if (key == "damage") _config.counterDamage = std::stof(val);
                                else if (key == "knock_x" || key == "knockx") _config.counterKnockX = std::stof(val);
                                else if (key == "knock_y" || key == "knocky") _config.counterKnockY = std::stof(val);
                            }
                        }
                    } else {
                        pendingBlock = tok;
                    }
                }
            }
        }
        return true;
    }

private:
    inline static PlayerStatsConfig _config;
    inline static bool _initialized = false;

    static void ensureInitialized() {
        if (_initialized) return;
        _initialized = true;
        _config = PlayerStatsConfig{}; // Дефолтные значения

        std::string kvPath = resolveAssetPath("assets/config/player.kv");
        if (std::filesystem::exists(kvPath)) {
            loadKVFile(kvPath);
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
