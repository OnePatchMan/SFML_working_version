
#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    app.loadRoomByName("room_1");
    auto beforeMap = app.getMap();
    app.autoTileCurrentRoom();
    auto afterMap = app.getMap();
    int diffCount = 0;
    for (int y = 0; y < 20; ++y) {
        for (int x = 0; x < 80; ++x) {
            if (beforeMap[y][x] != afterMap[y][x]) {
                diffCount++;
                std::cout << "Diff at (" << x << ", " << y << "): "
                          << beforeMap[y][x] << " -> " << afterMap[y][x] << std::endl;
            }
        }
    }
    std::cout << "Total diffs: " << diffCount << std::endl;
    return 0;
}
