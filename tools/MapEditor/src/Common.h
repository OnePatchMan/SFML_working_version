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

#include <string_view>

#ifndef TO_SF_STRING_DEFINED
#define TO_SF_STRING_DEFINED
inline sf::String toSfString(std::string_view str) {
    return sf::String::fromUtf8(str.begin(), str.end());
}
#endif

/** Resolves a repository asset subdirectory across common editor launch working directories. */
inline std::string resolveGameAssetDir(std::string_view relativeDir) {
    std::string rel(relativeDir);
    const std::string candidates[] = {
        rel,
        "../" + rel,
        "../../" + rel,
        "/home/dagai/Projects/SFML_working_version/" + rel,
        "../SFML_working_version/" + rel,
        "../../SFML_working_version/" + rel
    };
    for (const auto& path : candidates) {
        if (std::filesystem::exists(path)) {
            return std::filesystem::canonical(path).string();
        }
    }
    return rel;
}

inline std::string getGameMapsDir()       { return resolveGameAssetDir("assets/maps"); }
inline std::string getGameConfigDir()     { return resolveGameAssetDir("assets/config"); }
inline std::string getGameTilesDir()      { return resolveGameAssetDir("assets/tiles"); }
inline std::string getGameBlocksDir()     { return resolveGameAssetDir("assets/tiles/blocks"); }
inline std::string getGameStairsDir()     { return resolveGameAssetDir("assets/tiles/stairs"); }
inline std::string getGameDecorDir()      { return resolveGameAssetDir("assets/tiles/decor"); }
inline std::string getGameBackgroundDir() { return resolveGameAssetDir("assets/tiles/background"); }
inline std::string getGameToolsDir()      { return resolveGameAssetDir("assets/tiles/tools"); }

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
