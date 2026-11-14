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

    if (fs::exists(path)) return path;
    if (fs::exists("../" + path)) return "../" + path;
    if (fs::exists("../../" + path)) return "../../" + path;

    fs::path p(path);
    std::string filename = p.filename().string();

    const std::string candidates[] = {
        "assets/sprites/" + filename,
        "../assets/sprites/" + filename,
        "assets/fonts/" + filename,
        "../assets/fonts/" + filename,
        "assets/config/" + filename,
        "../assets/config/" + filename,
        "assets/" + filename,
        "../assets/" + filename
    };

    for (const auto& candidate : candidates) {
        if (fs::exists(candidate)) {
            return candidate;
        }
    }
    return path;
}

inline sf::String toSfString(std::string_view str) {
    return sf::String::fromUtf8(str.begin(), str.end());
}

