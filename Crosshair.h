#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <iostream>
#include "headers.h"

class Crosshair {
private:
    sf::Texture _animationTexture;
    sf::Sprite _animationSprite;

public:
    Crosshair(const std::string& filename, sf::RenderWindow& window)
        : _animationSprite(_animationTexture)
    {
        std::string resolved = resolveAssetPath(filename);
        if (!_animationTexture.loadFromFile(resolved)) {
            std::cerr << "Предупреждение: Не удалось загрузить прицел '" << filename 
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
            (void)_animationTexture.loadFromImage(img);
        }

        _animationSprite.setTexture(_animationTexture, true);
        sf::Vector2u size = _animationTexture.getSize();
        _animationSprite.setOrigin({static_cast<float>(size.x) / 2.0f, static_cast<float>(size.y) / 2.0f});
        _animationSprite.setScale({2.0f, 2.0f});
    }

    void Draw(sf::RenderWindow& window) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        _animationSprite.setPosition({static_cast<float>(mousePos.x), static_cast<float>(mousePos.y)});
        window.draw(_animationSprite);
    }
};
