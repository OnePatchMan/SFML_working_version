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
        return kv::forEachPair(filepath, [](const std::vector<std::string>& stack, const std::string& key, const std::string& val) {
            if (stack.size() < 2) return;
            const std::string& section = stack[1];
            if (section == "Stats") {
                if (key == "max_hp" || key == "maxhp") kv::tryParseFloat(val, _config.maxHp);
                else if (key == "move_speed" || key == "movespeed") kv::tryParseFloat(val, _config.moveSpeed);
                else if (key == "sprint_speed" || key == "sprintspeed") kv::tryParseFloat(val, _config.sprintSpeed);
                else if (key == "gravity") kv::tryParseFloat(val, _config.gravity);
                else if (key == "jump_speed" || key == "jumpspeed") kv::tryParseFloat(val, _config.jumpSpeed);
                else if (key == "max_fall_speed" || key == "maxfallspeed") kv::tryParseFloat(val, _config.maxFallSpeed);
                else if (key == "turn_delay" || key == "turndelay") kv::tryParseFloat(val, _config.turnDelay);
            } else if (section == "Dash") {
                if (key == "dash_speed" || key == "dashspeed") kv::tryParseFloat(val, _config.dashSpeed);
                else if (key == "dash_duration" || key == "dashduration") kv::tryParseFloat(val, _config.dashDuration);
                else if (key == "dash_cooldown" || key == "dashcooldown") kv::tryParseFloat(val, _config.dashCooldown);
                else if (key == "iframe_duration" || key == "iframeduration") kv::tryParseFloat(val, _config.iframeDuration);
            } else if (section == "SpotDodge") {
                if (key == "spot_dodge_duration" || key == "spotdodgeduration") kv::tryParseFloat(val, _config.spotDodgeDuration);
                else if (key == "spot_dodge_cooldown" || key == "spotdodgecooldown") kv::tryParseFloat(val, _config.spotDodgeCooldown);
                else if (key == "perfect_dodge_window" || key == "perfectdodgewindow") kv::tryParseFloat(val, _config.perfectDodgeWindow);
                else if (key == "perfect_dodge_empower" || key == "perfectdodgeempower") kv::tryParseFloat(val, _config.perfectDodgeEmpower);
            } else if (section == "Collision") {
                if (key == "half_w" || key == "halfw") kv::tryParseFloat(val, _config.halfW);
                else if (key == "top_offset" || key == "topoffset") kv::tryParseFloat(val, _config.topOffset);
                else if (key == "bottom_offset" || key == "bottomoffset") kv::tryParseFloat(val, _config.bottomOffset);
            } else if (section == "Counter") {
                if (key == "window") kv::tryParseFloat(val, _config.counterWindow);
                else if (key == "cooldown") kv::tryParseFloat(val, _config.counterCooldown);
                else if (key == "whiff_stun" || key == "whiffstun") kv::tryParseFloat(val, _config.counterWhiffStun);
                else if (key == "enemy_stun" || key == "enemystun") kv::tryParseFloat(val, _config.counterEnemyStun);
                else if (key == "damage") kv::tryParseFloat(val, _config.counterDamage);
                else if (key == "knock_x" || key == "knockx") kv::tryParseFloat(val, _config.counterKnockX);
                else if (key == "knock_y" || key == "knocky") kv::tryParseFloat(val, _config.counterKnockY);
            }
        });
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
};
