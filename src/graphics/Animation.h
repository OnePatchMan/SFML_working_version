#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "core/Common.h"

constexpr int FRAME_SIZE = 32;
constexpr float SPRITE_SCALE = 2.0f;

class Animation {
private:
    int _imageCount = 1;
    int _currentFrame = 0;
    bool _isLooping = true;
    bool _facingRight = true;
    sf::Vector2u _imageSize{0, 0};
    sf::Vector2f _spritePosition{0.0f, 0.0f};
    sf::Texture _animationTexture;
    std::vector<sf::Sprite> _animationSprites;

public:
    Animation(const std::string& filename, bool isLooping, 
              sf::Vector2f position = {0.0f, 0.0f}) 
        : _currentFrame(0), _isLooping(isLooping), _facingRight(true), _spritePosition(position) 
    {
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
            animationSprite.setOrigin({ FRAME_SIZE / 2.0f, FRAME_SIZE / 2.0f });
            animationSprite.setPosition(_spritePosition);
            // Исходный спрайт смотрит влево. Для взгляда вправо зеркалим по оси X (-SPRITE_SCALE)
            float initialScaleX = _facingRight ? -SPRITE_SCALE : SPRITE_SCALE;
            animationSprite.setScale({ initialScaleX, SPRITE_SCALE });

            _animationSprites.push_back(animationSprite);
        }
    }

    int getCurrentFrame() const { return _currentFrame; }
    int getImageCount() const { return _imageCount; }
    bool isLooping() const { return _isLooping; }
    bool isFacingRight() const { return _facingRight; }
    const sf::Sprite& getSprite(int frameIndex) const { return _animationSprites[frameIndex]; }
    const sf::Sprite& getCurrentSprite() const { return _animationSprites[_currentFrame]; }
    const sf::Vector2f& getPosition() const { return _spritePosition; }
    
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
        float scaleX = faceRight ? -SPRITE_SCALE : SPRITE_SCALE;
        for (auto& sprite : _animationSprites) {
            sprite.setScale({ scaleX, SPRITE_SCALE });
        }
    }

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

