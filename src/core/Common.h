#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <optional>
#include <functional>
#include <random>

constexpr float TILE_SIZE = 64.0f;
constexpr int ARENA_WIDTH = 80;
constexpr int ARENA_HEIGHT = 20;

inline std::string resolveAssetPath(const std::string& path) {
    namespace fs = std::filesystem;
    if (path.empty()) return path;

    // Сравниваем кандидаты путей: если файл существует и в build/, и в корне исходников (../path),
    // выбираем тот, у которого новее дата изменения (last_write_time), чтобы ручные правки
    // в исходниках ВСЕГДА немедленно подхватывались игрой!
    std::string best = "";
    auto checkCandidate = [&](const std::string& cand) {
        if (!fs::exists(cand)) return;
        if (best.empty()) {
            best = cand;
        } else {
            try {
                if (fs::last_write_time(cand) > fs::last_write_time(best)) {
                    best = cand;
                }
            } catch (...) {}
        }
    };

    checkCandidate(path);
    checkCandidate("../" + path);
    checkCandidate("../../" + path);

    if (!best.empty()) return best;

    fs::path p(path);
    std::string filename = p.filename().string();

    const std::string candidates[] = {
        "assets/sprites/" + filename,
        "../assets/sprites/" + filename,
        "assets/sprites/player/" + filename,
        "../assets/sprites/player/" + filename,
        "assets/tiles/blocks/" + filename,
        "../assets/tiles/blocks/" + filename,
        "assets/tiles/decor/" + filename,
        "../assets/tiles/decor/" + filename,
        "assets/sprites/enemies/normal/" + filename,
        "../assets/sprites/enemies/normal/" + filename,
        "assets/sprites/enemies/tank/" + filename,
        "../assets/sprites/enemies/tank/" + filename,
        "assets/sprites/enemies/duelist_aggressive/" + filename,
        "../assets/sprites/enemies/duelist_aggressive/" + filename,
        "assets/sprites/enemies/duelist_passive/" + filename,
        "../assets/sprites/enemies/duelist_passive/" + filename,
        "assets/sprites/enemies/dummy/" + filename,
        "../assets/sprites/enemies/dummy/" + filename,
        "assets/fonts/" + filename,
        "../assets/fonts/" + filename,
        "assets/config/" + filename,
        "../assets/config/" + filename,
        "assets/maps/" + filename,
        "../assets/maps/" + filename,
        "assets/soundeff/" + filename,
        "../assets/soundeff/" + filename,
        "../../assets/soundeff/" + filename,
        "assets/" + filename,
        "../assets/" + filename
    };

    for (const auto& candidate : candidates) {
        checkCandidate(candidate);
    }
    return !best.empty() ? best : path;
}

// =========================================================================
// УРОВНИ АТАКИ И ТИПЫ БЛОКА (FIGHTING COMBAT MATRIX)
// =========================================================================
enum class HitLevel {
    MID,       // Нейтральная атака: блокируется и верхним, и нижним блоком
    LOW,       // Нижняя атака: пробивает верхний блок! Блокируется ТОЛЬКО нижним блоком
    OVERHEAD   // Оверхэд (атака сверху/в прыжке): пробивает нижний блок! Блокируется ТОЛЬКО верхним блоком
};

enum class BlockType {
    NONE,      // Без блока
    HIGH,      // Верхний блок (стоя)
    LOW        // Нижний блок (сидя / присед)
};

#ifndef TO_SF_STRING_DEFINED
#define TO_SF_STRING_DEFINED
inline sf::String toSfString(std::string_view str) {
    return sf::String::fromUtf8(str.begin(), str.end());
}
#endif

