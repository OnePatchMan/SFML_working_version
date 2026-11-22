
#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    app.loadRoomByName("room_1");
    // Render room_1 to image
    sf::RenderTexture rt({1920, 1080});
    rt.clear(sf::Color(20, 24, 30));
    // Set camera to focus on the stairs (tx around 15..35, ty around 5..18)
    // tx=25, ty=12 -> x=25*32=800, y=12*32=384
    app._camera.setCenter({800.0f, 384.0f});
    app._camera.setSize({1920.0f, 1080.0f});
    app.renderDetailGrid(rt);
    rt.display();
    rt.getTexture().copyToImage().saveToFile("scratch/current_editor_room_1.png");
    std::cout << "Saved scratch/current_editor_room_1.png" << std::endl;
    return 0;
}
