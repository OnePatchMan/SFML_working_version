#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include "core/Common.h"

class Crosshair {
private:
    sf::Texture _texture;
    sf::Sprite _sprite;

public:
    Crosshair(const std::string& texturePath, sf::RenderWindow& window) 
        : _sprite(_texture)
    {
        std::string resolved = resolveAssetPath(texturePath);
        if (!_texture.loadFromFile(resolved)) {
            std::cerr << "Предупреждение: Не удалось загрузить прицел '" << texturePath 
                      << "'. Создание процедурного прицела." << std::endl;
            sf::Image img({16, 16}, sf::Color::Transparent);
            for (unsigned int i = 2; i <= 13; ++i) {
                if (i < 6 || i > 9) {
                    img.setPixel({i, 7}, sf::Color::Red);
                    img.setPixel({i, 8}, sf::Color::Red);
                    img.setPixel({7, i}, sf::Color::Red);
                    img.setPixel({8, i}, sf::Color::Red);
                }
            }
            (void)_texture.loadFromImage(img);
        }

        _sprite.setTexture(_texture, true); // true сбрасывает Rect на полный размер текстуры!
        sf::Vector2u size = _texture.getSize();
        _sprite.setTextureRect(sf::IntRect({0, 0}, {static_cast<int>(size.x), static_cast<int>(size.y)}));
        _sprite.setOrigin({static_cast<float>(size.x) / 2.0f, static_cast<float>(size.y) / 2.0f});
        _sprite.setScale({3.0f, 3.0f}); // Четкий, отлично различимый размер
    }

    void Draw(sf::RenderWindow& window) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        _sprite.setPosition({static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)});
        window.draw(_sprite);
    }
};
