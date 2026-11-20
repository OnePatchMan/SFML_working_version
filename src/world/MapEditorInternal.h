#pragma once
#include "world/MapEditor.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>

namespace map_editor_internal {

struct IngameLeftButton {
    std::string icon;
    std::string label;
    EditorTool tool;
    sf::Color accent;
};

inline const std::vector<IngameLeftButton> s_ingameLeftButtons = {
    {"Q",  "Пипетка [Q]",         EditorTool::EYEDROPPER,           sf::Color(200, 100, 255)},
    {"=",  "Платформа [1]",       EditorTool::PLATFORM,             sf::Color(76, 175, 80)},
    {"#",  "Стена [2]",           EditorTool::WALL,                 sf::Color(90, 110, 140)},
    {"L",  "Лампа [0]",           EditorTool::LAMP,                 sf::Color(255, 255, 160)},
    {"D",  "Дверь [3]",           EditorTool::DOOR,                 sf::Color(0, 229, 255)},
    {"P",  "Игрок [4]",           EditorTool::PLAYER_SPAWN,         sf::Color(0, 230, 118)},
    {"N",  "Обычный [5]",         EditorTool::ENEMY_SPAWN,          sf::Color(180, 180, 180)},
    {"T",  "Танк [6]",            EditorTool::ENEMY_TANK,           sf::Color(240, 50, 50)},
    {"U",  "Дуэлянт Агр [7]",     EditorTool::ENEMY_DUELIST,        sf::Color(170, 70, 230)},
    {"V",  "Дуэлянт Пасс [8]",    EditorTool::ENEMY_DUELIST_PASSIVE,sf::Color(45, 185, 245)},
    {"[]", "Заливка Box [X]",     EditorTool::BOX_FILL,             sf::Color(100, 180, 255)},
    {"/",  "Линия [L]",           EditorTool::LINE,                 sf::Color(80, 220, 160)},
    {"F",  "Ведро [G]",           EditorTool::FLOOD_FILL,           sf::Color(255, 180, 0)},
    {"X",  "Ластик [9]",          EditorTool::ERASER,               sf::Color(80, 80, 90)}
};

// Вспомогательный алгоритм Брезенхема для рисования прямых линий тайлов
inline std::vector<sf::Vector2i> getLineTiles(int x0, int y0, int x1, int y1) {
    std::vector<sf::Vector2i> tiles;
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int curX = x0;
    int curY = y0;
    while (true) {
        tiles.push_back({curX, curY});
        if (curX == x1 && curY == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            curX += sx;
        }
        if (e2 < dx) {
            err += dx;
            curY += sy;
        }
    }
    return tiles;
}

} // namespace map_editor_internal
