#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    app.loadRoomByName("room_1");
    app.autoTileCurrentRoom();
    app.saveToGame();
    std::cout << "room_1 autotiled and saved to game successfully!" << std::endl;
    return 0;
}
