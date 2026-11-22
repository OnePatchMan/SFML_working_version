#include "EditorApp.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "Starting Editor Palette & Autotile Verification..." << std::endl;
    EditorApp app;

    // 1. Check palette items for BLOCKS tab
    const auto& palette = app.getPaletteSprites();
    int blocks1Count = 0;
    int stairs1Count = 0;
    sf::IntRect blocks1Rect;
    std::string blocks1Name;

    for (const auto& item : palette) {
        if (item.filename.find("blocks1.png") != std::string::npos) {
            blocks1Count++;
            blocks1Name = item.name;
            if (item.sprite) blocks1Rect = item.sprite->getTextureRect();
        }
        if (item.filename.find("stairs1.png") != std::string::npos) {
            stairs1Count++;
        }
    }

    std::cout << "blocks1 palette cards: " << blocks1Count << " (Name: " << blocks1Name << ")" << std::endl;
    std::cout << "blocks1 preview rect: (" << blocks1Rect.position.x << ", " << blocks1Rect.position.y 
              << ", " << blocks1Rect.size.x << ", " << blocks1Rect.size.y << ")" << std::endl;
    std::cout << "stairs1 palette cards: " << stairs1Count << std::endl;

    // Verify blocks1 has ONLY 1 card in palette!
    assert(blocks1Count == 1);
    assert(blocks1Name == "blocks1 [Авто-выбор]");
    // Verify preview is single block at (128, 32)
    assert(blocks1Rect.position.x == 128);
    assert(blocks1Rect.position.y == 32);

    // Verify stairs1 has ONLY 2 cards (pair for left/right)
    assert(stairs1Count == 2);

    // 2. Check resolveAutoTile for inner corners
    int autoIdx = app.findAutoTileSetForCode(11);
    assert(autoIdx >= 0);

    // In room_1: tx = 7, ty = 7
    std::cout << "Neighbors at (7, 7):" << std::endl;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            std::cout << app.getMap()[7 + dy][7 + dx] << " ";
        }
        std::cout << std::endl;
    }
    const auto& set = app.getAutoTileSets()[autoIdx];
    auto isSolid = [&](int x, int y) {
        if (x < 0 || x >= 80 || y < 0 || y >= 20) return true;
        int t = app.getMap()[y][x];
        if (t == 0) return false;
        if (set.masterCode == t || set.memberCodes.count(t) > 0) return true;
        if (t == 1 || t == 2 || t == 6 || t == 7) return true;
        return false;
    };
    std::cout << "isSolid around (7, 7):" << std::endl;
    std::cout << "N=" << isSolid(7, 6) << " S=" << isSolid(7, 8) 
              << " W=" << isSolid(6, 7) << " E=" << isSolid(8, 7) << std::endl;
    std::cout << "NW=" << isSolid(6, 6) << " NE=" << isSolid(8, 6) 
              << " SW=" << isSolid(6, 8) << " SE=" << isSolid(8, 8) << std::endl;

    std::cout << "gridCodes in set:" << std::endl;
    for (int r = 0; r < set.rows; ++r) {
        std::cout << "Row " << (r+1) << ": ";
        for (int c = 0; c < set.cols; ++c) {
            std::cout << set.gridCodes[r][c] << " ";
        }
        std::cout << std::endl;
    }
    int codeAtCorner = app.resolveAutoTile(7, 7, autoIdx);
    std::cout << "Resolved tileCode at inner corner (7, 7): " << codeAtCorner << std::endl;

    // 3. Check resolveAutoStair
    int stairIdx = app.findAutoStairSetForCode(app.getAutoStairSets()[0].upRightSlopeCode);
    assert(stairIdx >= 0);
    const auto& ass = app.getAutoStairSets()[stairIdx];

    
    std::cout << "ass.upRightSlopeCode = " << ass.upRightSlopeCode << std::endl;
    std::cout << "ass.downRightSlopeCode = " << ass.downRightSlopeCode << std::endl;
    std::cout << "ass.upRightBaseCode = " << ass.upRightBaseCode << std::endl;
    std::cout << "ass.downRightBaseCode = " << ass.downRightBaseCode << std::endl;
    // Air above:

    int stairSlope = app.resolveAutoStair(8, 7, stairIdx, 0);
    std::cout << "Stair with air above resolved to: " << stairSlope << " (expected slope " << ass.upRightSlopeCode << ")" << std::endl;
    assert(stairSlope == ass.upRightSlopeCode);

    // Stair with solid above:
    int stairBase = app.resolveAutoStair(7, 8, stairIdx, 0);
    std::cout << "Stair with solid above resolved to: " << stairBase << " (expected base " << ass.upRightBaseCode << ")" << std::endl;
    assert(stairBase == ass.upRightBaseCode);

    std::cout << "ALL PALETTE & AUTOTILE CHECKS PASSED 100%!" << std::endl;
    return 0;
}
