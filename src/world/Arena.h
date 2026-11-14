#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "core/Common.h"

class Arena {
public:
    Arena(int width = ARENA_WIDTH, int height = ARENA_HEIGHT);

    bool isSolid(int tx, int ty) const;
    bool overlapsSolid(float px, float py, float halfW, float topOffset, float bottomOffset) const;

    sf::Vector2f getSpawnPosition() const { return _spawnPosition; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    int getFloorY() const { return _floorY; }

    void draw(sf::RenderWindow& window);

private:
    int _width;
    int _height;
    int _floorY;
    std::vector<std::vector<int>> _map;
    sf::Texture _tileTexture;
    std::vector<sf::Sprite> _tileSprites;
    sf::Vector2f _spawnPosition;

    void loadTileset();
    void buildMap();
};

