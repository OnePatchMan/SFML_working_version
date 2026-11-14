#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <iostream>
#include <variant>
#include <filesystem>

// Вспомогательная функция для поиска ассетов по относительным путям из любого рабочего каталога
inline std::string resolveAssetPath(const std::string& path) {
    namespace fs = std::filesystem;
    if (path.empty()) return path;

    // Проверяем путь напрямую
    if (fs::exists(path)) return path;

    // Проверяем на уровень выше (если бинарник запущен из build/)
    if (fs::exists("../" + path)) return "../" + path;

    // Проверяем на 2 уровня выше
    if (fs::exists("../../" + path)) return "../../" + path;

    fs::path p(path);
    std::string filename = p.filename().string();

    const std::string candidates[] = {
        "assets/sprites/" + filename,
        "../assets/sprites/" + filename,
        "../../assets/sprites/" + filename,
        "src/sprites/" + filename,
        "../src/sprites/" + filename,
        "../../src/sprites/" + filename,
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
