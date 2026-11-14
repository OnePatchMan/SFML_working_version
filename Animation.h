#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "headers.h"
#include "MapGenerator.h"

// Константы для анимации
constexpr int FRAME_SIZE = 32;
constexpr float SPRITE_SCALE = 2.0f;
constexpr float DEFAULT_POSITION_X = 960.0f;
constexpr float DEFAULT_POSITION_Y = 540.0f;

class Animation;

// Проигрывает анимации из класса Animation
class Animation {
private:
    int _imageCount;
    int _currentFrame;
    bool _isLooping;
    bool _facingRight = true;
    sf::Vector2u _imageSize;
    sf::Vector2f _spritePosition;
    sf::Texture _animationTexture;
    std::vector<sf::Sprite> _animationSprites;

public:
    Animation(const std::string& filename, bool isLooping, 
            sf::Vector2f position = {DEFAULT_POSITION_X, DEFAULT_POSITION_Y}) 
        : _isLooping(isLooping), _currentFrame(0), _facingRight(true), _spritePosition(position) {
        
        std::string resolved = resolveAssetPath(filename);
        if (!_animationTexture.loadFromFile(resolved)) {
            std::cerr << "Предупреждение: Не удалось загрузить анимацию '" << filename 
                      << "'. Создание процедурных кадров." << std::endl;
            sf::Image img({128, 32}, sf::Color::Transparent);
            for (int f = 0; f < 4; ++f) {
                unsigned int ox = static_cast<unsigned int>(f * 32);
                unsigned int oy = static_cast<unsigned int>((f == 1 || f == 3) ? 1 : 0);
                for (unsigned int x = 10; x <= 21; ++x) {
                    for (unsigned int y = 14; y <= 27; ++y) {
                        img.setPixel({ox + x, oy + y}, sf::Color(50, 100, 200));
                    }
                }
                for (unsigned int x = 11; x <= 20; ++x) {
                    for (unsigned int y = 4; y <= 13; ++y) {
                        img.setPixel({ox + x, oy + y}, sf::Color(240, 190, 150));
                    }
                }
                img.setPixel({ox + 13, oy + 7}, sf::Color::Black);
                img.setPixel({ox + 17, oy + 7}, sf::Color::Black);
            }
            (void)_animationTexture.loadFromImage(img);
        }

        _imageSize = _animationTexture.getSize();
        if (_imageSize.x == 0 || _imageSize.y == 0) {
            throw std::runtime_error("Размер текстуры равен 0!");
        }

        _imageCount = static_cast<int>(_imageSize.x / FRAME_SIZE);
        if (_imageCount < 1) _imageCount = 1;

        for (int i = 0; i < _imageCount; ++i) {
            sf::Sprite animationSprite(
                _animationTexture,
                sf::IntRect({ i * FRAME_SIZE, 0 }, { FRAME_SIZE, FRAME_SIZE })
            );
            // Устанавливаем точку привязки в центр спрайта для корректного разворота
            animationSprite.setOrigin({ FRAME_SIZE / 2.0f, FRAME_SIZE / 2.0f });
            animationSprite.setPosition(_spritePosition);
            // Исходный спрайт в текстуре смотрит влево.
            // Для взгляда вправо зеркалим по оси X (-SPRITE_SCALE)
            float initialScaleX = _facingRight ? -SPRITE_SCALE : SPRITE_SCALE;
            animationSprite.setScale({ initialScaleX, SPRITE_SCALE });

            _animationSprites.push_back(animationSprite);
        }
    }

    // Геттеры
    int getCurrentFrame() const { return _currentFrame; }
    int getImageCount() const { return _imageCount; }
    bool isLooping() const { return _isLooping; }
    bool isFacingRight() const { return _facingRight; }
    const sf::Sprite& getSprite(int frameIndex) const { return _animationSprites[frameIndex]; }
    const sf::Sprite& getCurrentSprite() const { return _animationSprites[_currentFrame]; }
    const sf::Vector2f& getPosition() const { return _spritePosition; }
    
    // Сеттеры
    void setCurrentFrame(int frame) { 
        if (frame >= 0 && frame < _imageCount) {
            _currentFrame = frame;
        }
    }
    void setPosition(const sf::Vector2f& position) {
        _spritePosition = position;
        for (auto& sprite : _animationSprites) {
            sprite.setPosition(_spritePosition);
        }
    }
    void move(const sf::Vector2f& offset) {
        setPosition(_spritePosition + offset);
    }
    void setColor(const sf::Color& color) {
        for (auto& sprite : _animationSprites) {
            sprite.setColor(color);
        }
    }
    void setFacing(bool faceRight) {
        if (_facingRight == faceRight) return;
        _facingRight = faceRight;
        // Исходный спрайт нарисован лицом влево.
        // Для взгляда вправо (-SPRITE_SCALE), для взгляда влево (+SPRITE_SCALE)
        float scaleX = faceRight ? -SPRITE_SCALE : SPRITE_SCALE;
        for (auto& sprite : _animationSprites) {
            sprite.setScale({ scaleX, SPRITE_SCALE });
        }
    }

    // Дружественный класс для доступа к приватным полям
    friend class Animator;
};

class Animator {
private:
    sf::Clock _clock;
    float _frameTime = 0.125f;

public:
    Animator(float frameTime = 0.125f) : _frameTime(frameTime) {}

    void setFrameTime(float frameTime) { _frameTime = frameTime; }

    void RunAnimation(Animation& animation, sf::RenderWindow& window) {
        if (_clock.getElapsedTime().asSeconds() >= _frameTime) {
            if (animation._isLooping) {
                animation._currentFrame = (animation._currentFrame + 1) % animation._imageCount;
            } else {
                if (animation._currentFrame < animation._imageCount - 1) {
                    animation._currentFrame++;
                }
            }
            _clock.restart();
        }

        window.draw(animation._animationSprites[animation._currentFrame]);
    }
};

// Отдельный класс для отрисовки карты
class MapRenderer {
private:
    static constexpr int TILE_SIZE = 64;

public:
    static void DrawMap(sf::RenderWindow& window, MapGenerator& generator, 
                       const std::vector<std::vector<int>>& map) {
        sf::View currentView = window.getView();
        sf::Vector2f center = currentView.getCenter();
        sf::Vector2f size = currentView.getSize();

        // Отрисовываем только видимые на экране тайлы с небольшим запасом
        int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / TILE_SIZE) - 1);
        int endX = std::min(static_cast<int>(map.empty() ? 0 : map[0].size()), 
                            static_cast<int>((center.x + size.x / 2.0f) / TILE_SIZE) + 2);
        int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / TILE_SIZE) - 1);
        int endY = std::min(static_cast<int>(map.size()), 
                            static_cast<int>((center.y + size.y / 2.0f) / TILE_SIZE) + 2);

        for (int y = startY; y < endY; y++) {
            for (int x = startX; x < endX; x++) {
                int tile = map[y][x];

                if (tile == 0) continue; // пусто, не рисуем

                sf::Sprite sprite = generator.getTileSprite(tile);
                sprite.setPosition({static_cast<float>(x * TILE_SIZE), 
                                   static_cast<float>(y * TILE_SIZE)});
                window.draw(sprite);
            }
        }
    }
};
