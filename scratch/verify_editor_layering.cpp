#include <SFML/Graphics.hpp>
#include <iostream>
#include <cassert>
#include "../tools/MapEditor/src/EditorApp.h"
#include "../src/world/Arena.h"

int main() {
    EditorApp editor;
    // scanRooms loads room_1
    editor.scanRooms();
    editor.syncCanvasToCurrentRoom("room_1");

    std::cout << "Room 1 size: " << editor.getWidth() << "x" << editor.getHeight() << std::endl;
    std::cout << "Lamps count: " << editor.getLamps().size() << std::endl;
    std::cout << "Background map size: " << editor.getBackgroundMap().size() << std::endl;

    // Check if background is preserved under lamps in room_1
    for (const auto& l : editor.getLamps()) {
        int lx = l.x;
        int ly = l.y;
        if (ly < static_cast<int>(editor.getBackgroundMap().size()) && lx < static_cast<int>(editor.getBackgroundMap()[ly].size())) {
            int bg = editor.getBackgroundMap()[ly][lx];
            std::cout << "Lamp at (" << lx << ", " << ly << ") has bgTile: " << bg << std::endl;
        }
    }

    // Verify placing background over a lamp or enemy doesn't delete them
    int testLx = 15, testLy = 5;
    editor.setCurrentTool(ToolType::LAMP, 4);
    editor.applyToolAt(testLx, testLy);
    assert(editor.getMap()[testLy][testLx] == 4);

    // Place background under the lamp
    editor.setCurrentTool(ToolType::BG_DECOR, 8);
    editor.applyToolAt(testLx, testLy);
    // Lamp should still be at (testLx, testLy) and background should also be at (testLx, testLy)
    assert(editor.getMap()[testLy][testLx] == 4);
    assert(editor.getBackgroundMap()[testLy][testLx] == 8);
    std::cout << "Verified: placing BG_DECOR under lamp does not delete lamp!" << std::endl;

    // Place an enemy
    int testEx = 20, testEy = 10;
    editor.setCurrentTool(ToolType::ENEMY_SPAWN, 0);
    editor.applyToolAt(testEx, testEy);
    bool hasEnemy = false;
    for (const auto& es : editor.getEnemySpawns()) {
        if (es.x == testEx && es.y == testEy) hasEnemy = true;
    }
    assert(hasEnemy);

    // Place background under enemy
    editor.setCurrentTool(ToolType::BG_DECOR, 8);
    editor.applyToolAt(testEx, testEy);
    hasEnemy = false;
    for (const auto& es : editor.getEnemySpawns()) {
        if (es.x == testEx && es.y == testEy) hasEnemy = true;
    }
    assert(hasEnemy);
    assert(editor.getBackgroundMap()[testEy][testEx] == 8);
    std::cout << "Verified: placing BG_DECOR under enemy does not delete enemy!" << std::endl;

    std::cout << "ALL VERIFICATION CHECKS PASSED!" << std::endl;
    return 0;
}

