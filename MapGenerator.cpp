#include "MapGenerator.h"
#include <stdexcept>
#include <iostream>
#include <string>
#include <algorithm>
#include "headers.h"

MapGenerator::MapGenerator(int width, int height, float frequency, const std::string& filename)
    : _width(width), _height(height), _frequency(frequency), _spawnPosition(640.0f, 930.0f)
{
    std::string resolved = resolveAssetPath(filename);
    if (!_tileTexture.loadFromFile(resolved)) {
        std::cerr << "Предупреждение: Не удалось загрузить тайлсет '" << filename 
                  << "'. Создание процедурного тайлсета." << std::endl;
        sf::Image img({96, 32}, sf::Color::Transparent);
        for (unsigned int x = 32; x < 64; ++x) {
            for (unsigned int y = 0; y < 32; ++y) {
                if (x == 32 || x == 63 || y == 0 || y == 31) {
                    img.setPixel({x, y}, sf::Color(45, 55, 65));
                } else if ((x + y) % 7 == 0) {
                    img.setPixel({x, y}, sf::Color(75, 85, 95));
                } else {
                    img.setPixel({x, y}, sf::Color(60, 70, 80));
                }
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

    int imageCount = static_cast<int>(_tileTexture.getSize().x / 32);
    if (imageCount < 3) imageCount = 3;
    _tileMap.reserve(imageCount);

    for (int i = 0; i < imageCount; ++i) {
        sf::Sprite sprite(_tileTexture);
        sprite.setTextureRect(sf::IntRect({i * 32, 0}, {32, 32}));
        sprite.scale({2.0f, 2.0f});
        _tileMap.push_back(sprite);
    }
}

std::vector<std::vector<int>> MapGenerator::generate() {
    // 0 = воздух, 1 = пол/платформа, 2 = стена/фундамент
    std::vector<std::vector<int>> map(_height, std::vector<int>(_width, 0));

    // Уровень основного прямого пола арены (на 5 блоков выше низа карты)
    int floorY = std::max(2, _height - 5);

    // 1. Сплошной прямой пол (floorY) и монолитный фундамент под ним
    for (int x = 0; x < _width; ++x) {
        map[floorY][x] = 1; // Поверхность пола
        for (int y = floorY + 1; y < _height; ++y) {
            map[y][x] = 2; // Фундамент под ногами
        }
    }

    // 2. Потолок (y = 0) и боковые монолитные стены-ограничители (x = 0, x = _width - 1)
    for (int x = 0; x < _width; ++x) {
        map[0][x] = 2;
    }
    for (int y = 0; y < _height; ++y) {
        map[y][0] = 2;
        map[y][_width - 1] = 2;
    }

    // 3. Точка спавна: слева на прямом полу
    float spawnX = 6.0f * 64.0f;
    float spawnY = static_cast<float>(floorY) * 64.0f - 30.0f;
    _spawnPosition = {spawnX, spawnY};

    std::cout << "Создана фиксированная прямая арена (без висящих блоков): " << _width << "x" << _height << std::endl;
    return map;
}

const sf::Sprite& MapGenerator::getTileSprite(int id) const {
    if (id < 0 || static_cast<size_t>(id) >= _tileMap.size()) {
        throw std::out_of_range("Неверный ID тайла: " + std::to_string(id));
    }
    return _tileMap[id];
}
