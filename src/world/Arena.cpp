#include "world/Arena.h"
#include <iostream>
#include <algorithm>

Arena::Arena(int width, int height)
    : _width(width), _height(height), _floorY(std::max(2, height - 5))
{
    loadTileset();
    buildMap();

    float spawnX = 6.0f * TILE_SIZE;
    float spawnY = static_cast<float>(_floorY) * TILE_SIZE - 30.0f;
    _spawnPosition = {spawnX, spawnY};
}

void Arena::loadTileset() {
    std::string path = resolveAssetPath("assets/sprites/BlocksTest.png");
    if (!_tileTexture.loadFromFile(path)) {
        std::cerr << "Предупреждение: Не удалось загрузить тайлсет. Создание процедурного тайлсета." << std::endl;
        sf::Image img({96, 32}, sf::Color::Transparent);
        for (unsigned int x = 32; x < 64; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if (x == 32 || x == 63 || y == 0 || y == 31) img.setPixel({x, y}, sf::Color(45, 55, 65));
                else if ((x + y) % 7 == 0) img.setPixel({x, y}, sf::Color(75, 85, 95));
                else img.setPixel({x, y}, sf::Color(60, 70, 80));
            }
        }
        for (unsigned int x = 64; x < 96; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if (y == 0 || y == 16 || y == 31 || 
                    ((y < 16) && (x == 64 || x == 80 || x == 95)) ||
                    ((y >= 16) && (x == 72 || x == 88))) {
                    img.setPixel({x, y}, sf::Color(80, 50, 40));
                } else {
                    img.setPixel({x, y}, sf::Color(130, 90, 70));
                }
            }
        }
        (void)_tileTexture.loadFromImage(img);
    }

    int count = static_cast<int>(_tileTexture.getSize().x / 32);
    if (count < 3) count = 3;
    _tileSprites.reserve(count);

    for (int i = 0; i < count; ++i) {
        sf::Sprite sp(_tileTexture);
        sp.setTextureRect(sf::IntRect({i * 32, 0}, {32, 32}));
        sp.scale({2.0f, 2.0f});
        _tileSprites.push_back(sp);
    }
}

void Arena::buildMap() {
    // 0 = воздух, 1 = пол, 2 = стена/фундамент
    _map.assign(_height, std::vector<int>(_width, 0));

    // Сплошной пол и монолитный фундамент под ним
    for (int x = 0; x < _width; ++x) {
        _map[_floorY][x] = 1;
        for (int y = _floorY + 1; y < _height; ++y) {
            _map[y][x] = 2;
        }
    }

    // Потолок (y = 0) и боковые стены-ограничители
    for (int x = 0; x < _width; ++x) {
        _map[0][x] = 2;
    }
    for (int y = 0; y < _height; ++y) {
        _map[y][0] = 2;
        _map[y][_width - 1] = 2;
    }
}

bool Arena::isSolid(int tx, int ty) const {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return true;
    return _map[ty][tx] == 1 || _map[ty][tx] == 2;
}

bool Arena::overlapsSolid(float px, float py, float halfW, float topOffset, float bottomOffset) const {
    float left = px - halfW;
    float right = px + halfW - 0.001f;
    float top = py - topOffset;
    float bottom = py + bottomOffset - 0.001f;

    int minTx = static_cast<int>(std::floor(left / TILE_SIZE));
    int maxTx = static_cast<int>(std::floor(right / TILE_SIZE));
    int minTy = static_cast<int>(std::floor(top / TILE_SIZE));
    int maxTy = static_cast<int>(std::floor(bottom / TILE_SIZE));

    for (int ty = minTy; ty <= maxTy; ++ty) {
        for (int tx = minTx; tx <= maxTx; ++tx) {
            if (isSolid(tx, ty)) return true;
        }
    }
    return false;
}

void Arena::draw(sf::RenderWindow& window) {
    sf::View view = window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / TILE_SIZE) - 1);
    int endX = std::min(_width, static_cast<int>((center.x + size.x / 2.0f) / TILE_SIZE) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / TILE_SIZE) - 1);
    int endY = std::min(_height, static_cast<int>((center.y + size.y / 2.0f) / TILE_SIZE) + 2);

    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            int tile = _map[y][x];
            if (tile == 0) continue;
            if (static_cast<size_t>(tile) < _tileSprites.size()) {
                sf::Sprite sp = _tileSprites[tile];
                sp.setPosition({static_cast<float>(x * TILE_SIZE), static_cast<float>(y * TILE_SIZE)});
                window.draw(sp);
            }
        }
    }
}

