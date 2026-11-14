#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

class MapGenerator {
public:
    MapGenerator(int width, int height, float frequency, const std::string& filename);

    // 0 = воздух, 1 = пол/платформа, 2 = стена/фундамент
    std::vector<std::vector<int>> generate();

    const sf::Sprite& getTileSprite(int id) const;

    sf::Vector2f getSpawnPosition() const { return _spawnPosition; }

private:
    int _width;
    int _height;
    float _frequency;
    sf::Texture _tileTexture;
    std::vector<sf::Sprite> _tileMap;
    sf::Vector2f _spawnPosition;
};
