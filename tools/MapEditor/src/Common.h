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
        "/home/dagai/Projects/SFML_working_version/assets/maps",
        "../SFML_working_version/assets/maps",
        "../../SFML_working_version/assets/maps"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "/home/dagai/Projects/SFML_working_version/assets/maps";
}

inline std::string getGameConfigDir() {
    std::vector<std::string> candidates = {
        "/home/dagai/Projects/SFML_working_version/assets/config",
        "../SFML_working_version/assets/config",
        "../../SFML_working_version/assets/config"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "/home/dagai/Projects/SFML_working_version/assets/config";
}

inline std::string getGameTilesDir() {
    std::vector<std::string> candidates = {
        "/home/dagai/Projects/SFML_working_version/assets/tiles",
        "../SFML_working_version/assets/tiles",
        "../../SFML_working_version/assets/tiles",
        "assets/tiles"
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return "/home/dagai/Projects/SFML_working_version/assets/tiles";
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
