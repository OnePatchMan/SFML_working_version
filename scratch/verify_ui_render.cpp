#include "core/Common.h"
#include "combat/CombatSystem.h"
#include <iostream>

int main() {
    CombatSystem combat;
    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Test");
    window.clear(sf::Color(15, 15, 20));
    combat.drawUI(window);
    window.display();

    sf::Texture tex({1280, 720});
    tex.update(window);
    (void)tex.copyToImage().saveToFile("scratch/ui_rendered.png");
    std::cout << "Rendered UI to scratch/ui_rendered.png" << std::endl;
    return 0;
}
