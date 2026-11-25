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
#include <fstream>
#include <cctype>
#include <string_view>

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
        "assets/sprites/vfx/" + filename,
        "../assets/sprites/vfx/" + filename,
        "assets/sprites/vfx/impacts/" + filename,
        "../assets/sprites/vfx/impacts/" + filename,
        "assets/sprites/vfx/nature/" + filename,
        "../assets/sprites/vfx/nature/" + filename,
        "assets/sprites/vfx/melee/" + filename,
        "../assets/sprites/vfx/melee/" + filename,
        "assets/sprites/vfx/explosions/" + filename,
        "../assets/sprites/vfx/explosions/" + filename,
        "assets/sprites/player/" + filename,
        "../assets/sprites/player/" + filename,
        "assets/tiles/blocks/" + filename,
        "../assets/tiles/blocks/" + filename,
        "assets/tiles/stairs/" + filename,
        "../assets/tiles/stairs/" + filename,
        "assets/tiles/decor/" + filename,
        "../assets/tiles/decor/" + filename,
        "assets/tiles/background/" + filename,
        "../assets/tiles/background/" + filename,
        "assets/tiles/tools/" + filename,
        "../assets/tiles/tools/" + filename,
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

namespace kv {
    /** Tokenizes a Key-Values line, respecting double-quoted strings, braces, and // or #/; comments. */
    inline std::vector<std::string> tokenizeLine(const std::string& line) {
        std::vector<std::string> tokens;
        size_t i = 0;
        while (i < line.size()) {
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r' ||
                                       line[i] == '\n' || line[i] == '=' || line[i] == ':')) {
                ++i;
            }
            if (i >= line.size()) break;
            if ((line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') ||
                line[i] == '#' || line[i] == ';') {
                break;
            }
            if (line[i] == '"') {
                ++i;
                const size_t start = i;
                while (i < line.size() && line[i] != '"') ++i;
                tokens.push_back(line.substr(start, i - start));
                if (i < line.size() && line[i] == '"') ++i;
            } else if (line[i] == '{' || line[i] == '}') {
                tokens.push_back(std::string(1, line[i]));
                ++i;
            } else {
                const size_t start = i;
                while (i < line.size() && line[i] != ' ' && line[i] != '\t' && line[i] != '\r' &&
                       line[i] != '\n' && line[i] != '=' && line[i] != ':' &&
                       line[i] != '{' && line[i] != '}' && line[i] != '"') {
                    ++i;
                }
                tokens.push_back(line.substr(start, i - start));
            }
        }
        return tokens;
    }

    inline bool tryParseFloat(const std::string& str, float& outVal) {
        try {
            outVal = std::stof(str);
            return true;
        } catch (...) {
            return false;
        }
    }

    inline bool tryParseInt(const std::string& str, int& outVal) {
        try {
            outVal = std::stoi(str);
            return true;
        } catch (...) {
            return false;
        }
    }

    inline float parseFloatSafe(const std::string& str, float fallback = 0.0f) {
        float v = fallback;
        tryParseFloat(str, v);
        return v;
    }

    inline int parseIntSafe(const std::string& str, int fallback = 0) {
        int v = fallback;
        tryParseInt(str, v);
        return v;
    }

    inline bool assignFramesSeconds(const std::string& str, float& outSeconds) {
        float frames = 0.0f;
        if (!tryParseFloat(str, frames)) return false;
        outSeconds = frames / 60.0f;
        return true;
    }

    inline bool parseBool(const std::string& str, bool fallback = false) {
        if (str == "1" || str == "true" || str == "TRUE") return true;
        if (str == "0" || str == "false" || str == "FALSE") return false;
        int n = fallback ? 1 : 0;
        if (tryParseInt(str, n)) return n != 0;
        return fallback;
    }

    /** Streams key-value pairs with active block stack through a callback(stack, lowerKey, val). */
    template <typename Callback>
    inline bool forEachPair(const std::string& filepath, Callback&& onPair) {
        std::ifstream file(filepath);
        if (!file.is_open()) return false;

        std::vector<std::string> blockStack;
        std::string pendingBlock;
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
                } else if (t + 1 < tokens.size() && tokens[t + 1] != "{" && tokens[t + 1] != "}") {
                    std::string key = tok;
                    const std::string& val = tokens[++t];
                    std::transform(key.begin(), key.end(), key.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    onPair(blockStack, key, val);
                } else {
                    pendingBlock = tok;
                }
            }
        }
        return true;
    }
}

