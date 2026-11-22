#include <SFML/Graphics.hpp>
#include "world/Arena.h"
#include <iostream>

int main() {
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Stair Lighting Test", sf::Style::None);
    Arena arena;
    arena.loadRoom(1);

    // Lamp from room_1.kv: "lamp_1" "12 4 240 250 255 2 1000 130 1"
    arena.addLamp(12, 4);
    for (auto& l : arena.getLamps()) {
        if (l.tileX == 12 && l.tileY == 4) {
            l.color = sf::Color(240, 250, 255);
            l.intensity = 2.0f;
            l.radius = 1000.0f;
            l.coneAngle = 130.0f;
            l.dirty = true;
        }
    }

    // Place stair tiles
    arena.setTile(8, 5, 40);
    arena.setTile(9, 6, 40);
    arena.setTile(8, 6, 42);

    sf::View view;
    view.setSize({1280.0f, 720.0f});
    view.setCenter({640.0f, 384.0f});

    window.clear(sf::Color::Black);
    window.setView(view);

    arena.draw(window);
    arena.renderLighting(window, view, {640.0f, 384.0f}, true);
    arena.drawBlackoutMask(window);

    window.display();

    sf::Texture tex({1280, 720});
    tex.update(window);
    sf::Image img = tex.copyToImage();
    std::string outPath = "/home/dagai/.gemini/antigravity/brain/1810658f-f67f-4d8d-a67d-8143b1ecf63e/stair_shadow_fixed.png";
    (void)img.saveToFile(outPath);
    std::cout << "Saved screenshot to " << outPath << std::endl;
    return 0;
}

