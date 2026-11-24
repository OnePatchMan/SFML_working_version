#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include "core/Common.h"

enum class BossSlashType {
    HEAVY_OVERHEAD,    // Вертикальный/диагональный сокрушительный удар сверху вниз
    HORIZONTAL_CLEAVE, // Горизонтальный размашистый рубящий взмах
    CROSS_X,           // Крестообразный двойной рассекатель
    WHIRLWIND_360      // Круговой вихревой взмах (Смерч ударов)
};

// Экземпляр процедурного рубящего взмаха босса (точно по размеру хитбокса)
struct BossSlashInstance {
    sf::FloatRect hitbox;
    bool facingLeft = false;
    BossSlashType type = BossSlashType::HORIZONTAL_CLEAVE;
    float timer = 0.0f;
    float maxDuration = 0.22f;
    bool active = true;

    // Внутренние процедурные частицы/искры взмаха
    struct Spark {
        sf::Vector2f pos;
        sf::Vector2f vel;
        float timer = 0.0f;
        float maxLife = 0.25f;
    };
    std::vector<Spark> sparks;
};

// Вырывающиеся из земли каменные шипы при Землетрясении и Слэме
struct BossGroundSpike {
    sf::Vector2f basePos;
    float width = 24.0f;
    float targetHeight = 55.0f;
    float currentHeight = 0.0f;
    float tilt = 0.0f;
    float timer = 0.0f;
    float maxDuration = 0.85f;
    bool active = true;
};

// Разлетающиеся каменные осколки и комья земли
struct BossDebrisChunk {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float rot = 0.0f;
    float rotSpeed = 0.0f;
    float size = 8.0f;
    float timer = 0.0f;
    float maxDuration = 0.70f;
    sf::Color color = sf::Color(110, 85, 75);
    bool active = true;
};

// Расширяющиеся кольца ударной волны по полу
struct BossShockwaveRing {
    sf::Vector2f center;
    float radiusX = 10.0f;
    float radiusY = 5.0f;
    float maxRadiusX = 180.0f;
    float speed = 450.0f;
    float timer = 0.0f;
    float maxDuration = 0.45f;
    sf::Color color = sf::Color(255, 60, 30);
    bool active = true;
};

// Сокрушительный энергетический столб удара из воздуха до пола (Jump Slam)
struct BossJumpSlamPillar {
    sf::FloatRect hitbox;
    float timer = 0.0f;
    float maxDuration = 0.32f;
    bool active = true;
};

// Взрыв породы и пыли при врезании тарана в стену
struct BossWallCrashEffect {
    sf::Vector2f pos;
    float dirX = -1.0f;
    float timer = 0.0f;
    float maxDuration = 0.65f;
    bool active = true;
    std::vector<BossDebrisChunk> chunks;
};

// Красный шлейф-фантом при стремительном беге босса (Таран)
struct BossAfterimageGhost {
    sf::Vector2f pos;
    sf::Vector2f size;
    sf::Vector2f origin;
    bool facingLeft = false;
    float timer = 0.0f;
    float maxDuration = 0.18f;
    bool active = true;
};

class BossVfxManager {
private:
    std::vector<BossSlashInstance> m_slashes;
    std::vector<BossGroundSpike> m_spikes;
    std::vector<BossDebrisChunk> m_debris;
    std::vector<BossShockwaveRing> m_rings;
    std::vector<BossJumpSlamPillar> m_pillars;
    std::vector<BossWallCrashEffect> m_wallCrashes;
    std::vector<BossAfterimageGhost> m_afterimages;

    BossVfxManager() = default;

public:
    static BossVfxManager& get() {
        static BossVfxManager instance;
        return instance;
    }

    // Спавн рубящего удара, идеально подогнанного под размеры хитбокса
    void spawnSlash(const sf::FloatRect& hitbox, bool facingLeft, BossSlashType type, float duration = 0.22f);

    // Спавн сокрушительного удара в землю (Землетрясение / приземление слэма)
    void spawnGroundImpact(const sf::Vector2f& groundPos, float width = 340.0f, bool intense = false);

    // Спавн вертикального энергетического столба удара с воздуха по полу
    void spawnJumpSlamBeam(const sf::FloatRect& hitbox, float duration = 0.30f);

    // Спавн взрыва породы при врезании в стену
    void spawnWallCrash(const sf::Vector2f& impactPos, float dirX);

    // Спавн остаточного шлейфа при беге/таране
    void spawnAfterimage(const sf::Vector2f& pos, const sf::Vector2f& size, const sf::Vector2f& origin, bool facingLeft);

    void update(float dt);
    void draw(sf::RenderTarget& target);
    void clear();

    size_t getActiveCount() const;
};

