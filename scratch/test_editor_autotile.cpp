#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    app.loadRoomByName("room_1");
    const auto& m1 = app.getMap();
    std::cout << "--- BEFORE AUTOTILE --- " << std::endl;
    for (int y = 6; y <= 18; ++y) {
        std::cout << "y=" << y << ": ";
        for (int x = 14; x <= 28; ++x) {
            std::cout << m1[y][x] << "	";
        }
        std::cout << std::endl;
    }
    std::cout << "--- RUNNING AUTOTILE --- " << std::endl;
    app.autoTileCurrentRoom();
    const auto& m2 = app.getMap();
    for (int y = 6; y <= 18; ++y) {
        std::cout << "y=" << y << ": ";
        for (int x = 14; x <= 28; ++x) {
            std::cout << m2[y][x] << "	";
        }
        std::cout << std::endl;
    }
    return 0;
}
