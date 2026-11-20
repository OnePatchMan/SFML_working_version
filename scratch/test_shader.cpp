#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    bool avail = sf::Shader::isAvailable();
    std::cout << "Shader available: " << (avail ? "YES" : "NO") << std::endl;
    return 0;
}
