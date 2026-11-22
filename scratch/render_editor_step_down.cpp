#include "world/Arena.h"
#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    sf::RenderWindow win(sf::VideoMode({1600, 900}), "Editor Snapshot");
    Arena arena;
    arena.loadMapConfig("assets/maps/room_1.kv");
    win.clear(sf::Color(10, 11, 15));
    arena.draw(win);
    win.display();

    sf::Texture tex({1600, 900});
    tex.update(win);
    tex.copyToImage().saveToFile("scratch/editor_fixed_step_down.png");
    std::cout << "Saved snapshot to scratch/editor_fixed_step_down.png" << std::endl;
    return 0;
}
