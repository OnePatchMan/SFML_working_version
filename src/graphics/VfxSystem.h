#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include "core/Common.h"

// Описание одного типа спец-эффекта (метаданные спрайт-шита)
struct VfxDefinition {
    std::string id;
    std::string texturePath;
    int frameW = 0;
    int frameH = 0;
    int totalFrames = 0;
    int columns = 1;
    sf::Vector2f pivot = {0.0f, 0.0f};
    float fps = 30.0f;
    std::shared_ptr<sf::Texture> texture;
};

// Экземпляр проигрываемого эффекта в игровом мире
struct VfxInstance {
    std::string name;
    sf::Vector2f position = {0.0f, 0.0f};
    sf::Vector2f scale = {1.0f, 1.0f};
    float rotation = 0.0f;
    sf::Color color = sf::Color::White;
    float timer = 0.0f;
    int currentFrame = 0;
    bool active = true;
    const VfxDefinition* def = nullptr;

    void update(float dt) {
        if (!active || !def || def->totalFrames <= 0) return;
        timer += dt;
        float frameDuration = (def->fps > 0.0f) ? (1.0f / def->fps) : 0.0333f;
        while (timer >= frameDuration) {
            timer -= frameDuration;
            currentFrame++;
            if (currentFrame >= def->totalFrames) {
                active = false;
                break;
            }
        }
    }

    void draw(sf::RenderTarget& target) const {
        if (!active || !def || !def->texture || def->columns <= 0 || def->frameW <= 0 || def->frameH <= 0) return;
        int col = currentFrame % def->columns;
        int row = currentFrame / def->columns;

        sf::IntRect rect({col * def->frameW, row * def->frameH}, {def->frameW, def->frameH});
        sf::Sprite sprite(*def->texture, rect);
        sprite.setOrigin(def->pivot);
        sprite.setPosition(position);
        sprite.setScale(scale);
        sprite.setRotation(sf::degrees(rotation));
        sprite.setColor(color);
        target.draw(sprite);
    }
};

class VfxSystem {
private:
    std::unordered_map<std::string, VfxDefinition> m_defs;
    std::vector<VfxInstance> m_instances;
    bool m_initialized = false;

    VfxSystem() = default;

public:
    static VfxSystem& get() {
        static VfxSystem instance;
        return instance;
    }

    void init();
    void registerDef(const VfxDefinition& def);

    void spawn(const std::string& name, const sf::Vector2f& pos,
               const sf::Vector2f& scale = {1.0f, 1.0f},
               float rotation = 0.0f,
               const sf::Color& color = sf::Color::White);

    void spawnGroundSlam(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnEarthSpike(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnQuake(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnHeavyCleave(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnHammerSmash(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnSpinSlash(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnSwordSlash(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnDirtExplosion(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnHeavyLanding(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);
    void spawnHitSpark(const sf::Vector2f& pos, float scaleX = 1.0f, float scaleY = 1.0f);

    void update(float dt);
    void draw(sf::RenderTarget& target);
    void clear();

    size_t getActiveCount() const;
    const std::vector<VfxInstance>& getActiveInstances() const { return m_instances; }
    bool hasDefinition(const std::string& name) const { return m_defs.find(name) != m_defs.end(); }
};

