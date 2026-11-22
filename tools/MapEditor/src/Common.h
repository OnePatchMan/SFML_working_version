#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <algorithm>

constexpr float EDITOR_TILE_SIZE = 64.0f;

#ifndef TO_SF_STRING_DEFINED
#define TO_SF_STRING_DEFINED
inline sf::String toSfString(std::string_view str) {
    return sf::String::fromUtf8(str.begin(), str.end());
}
#endif

inline std::string getGameMapsDir() {
    std::vector<std::string> candidates = {
        "assets/maps",
        "../assets/maps",
        "../../assets/maps",
        "/home/dagai/Projects/SFML_working_version/assets/maps",
        "../SFML_working_version/assets/maps",
        "../../SFML_working_version/assets/maps"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/maps";
}

inline std::string getGameConfigDir() {
    std::vector<std::string> candidates = {
        "assets/config",
        "../assets/config",
        "../../assets/config",
        "/home/dagai/Projects/SFML_working_version/assets/config",
        "../SFML_working_version/assets/config",
        "../../SFML_working_version/assets/config"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/config";
}

inline std::string getGameTilesDir() {
    std::vector<std::string> candidates = {
        "assets/tiles",
        "../assets/tiles",
        "../../assets/tiles",
        "/home/dagai/Projects/SFML_working_version/assets/tiles",
        "../SFML_working_version/assets/tiles",
        "../../SFML_working_version/assets/tiles"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles";
}

inline std::string getGameBlocksDir() {
    std::vector<std::string> candidates = {
        "assets/tiles/blocks",
        "../assets/tiles/blocks",
        "../../assets/tiles/blocks",
        "/home/dagai/Projects/SFML_working_version/assets/tiles/blocks",
        "../SFML_working_version/assets/tiles/blocks",
        "../../SFML_working_version/assets/tiles/blocks"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles/blocks";
}

inline std::string getGameStairsDir() {
    std::vector<std::string> candidates = {
        "assets/tiles/stairs",
        "../assets/tiles/stairs",
        "../../assets/tiles/stairs",
        "/home/dagai/Projects/SFML_working_version/assets/tiles/stairs",
        "../SFML_working_version/assets/tiles/stairs",
        "../../SFML_working_version/assets/tiles/stairs"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles/stairs";
}

inline std::string getGameDecorDir() {
    std::vector<std::string> candidates = {
        "assets/tiles/decor",
        "../assets/tiles/decor",
        "../../assets/tiles/decor",
        "/home/dagai/Projects/SFML_working_version/assets/tiles/decor",
        "../SFML_working_version/assets/tiles/decor",
        "../../SFML_working_version/assets/tiles/decor"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles/decor";
}

inline std::string getGameBackgroundDir() {
    std::vector<std::string> candidates = {
        "assets/tiles/background",
        "../assets/tiles/background",
        "../../assets/tiles/background",
        "/home/dagai/Projects/SFML_working_version/assets/tiles/background",
        "../SFML_working_version/assets/tiles/background",
        "../../SFML_working_version/assets/tiles/background"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles/background";
}

inline std::string getGameToolsDir() {
    std::vector<std::string> candidates = {
        "assets/tiles/tools",
        "../assets/tiles/tools",
        "../../assets/tiles/tools",
        "/home/dagai/Projects/SFML_working_version/assets/tiles/tools",
        "../SFML_working_version/assets/tiles/tools",
        "../../SFML_working_version/assets/tiles/tools"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "assets/tiles/tools";
}

inline std::string resolveEditorFont() {
    std::vector<std::string> candidates = {
        "/home/dagai/Projects/SFML_working_version/assets/fonts/font.ttf",
        "assets/fonts/font.ttf",
        "../SFML_working_version/assets/fonts/font.ttf",
        "../../SFML_working_version/assets/fonts/font.ttf",
        "/usr/share/fonts/TTF/TerminessNerdFont-Bold.ttf",
        "/usr/share/fonts/TTF/TerminessNerdFont-Regular.ttf"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }
    return "/home/dagai/Projects/SFML_working_version/assets/fonts/font.ttf";
}
