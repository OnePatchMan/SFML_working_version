
#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    app.loadRoomByName("room_1");
    int autoIdx = app.findAutoTileSetForCode(17);
    for (int y = 8; y <= 18; ++y) {
        for (int x = 16; x <= 26; ++x) {
            int t = app.getMap()[y][x];
            int r = app.resolveAutoTile(x, y, autoIdx);
            if (t != r && t > 0) {
                std::cout << "MISMATCH at (" << x << ", " << y << "): in_map=" << t << ", resolved=" << r << std::endl;
            }
        }
    }
    std::cout << "Scan completed!" << std::endl;
    return 0;
}
