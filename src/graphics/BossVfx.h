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

// Экземпляр рубящего взмаха босса с ослепительным энергетическим шлейфом и снопом искр
struct BossSlashInstance {
    sf::FloatRect hitbox;
    bool facingLeft = false;
    BossSlashType type = BossSlashType::HORIZONTAL_CLEAVE;
    float timer = 0.0f;
    float maxDuration = 0.28f;
    bool active = true;

    // Внутренние процедурные искры со скоростными шлейфами
    struct Spark {
        sf::Vector2f pos;
        sf::Vector2f prevPos;
        sf::Vector2f vel;
        float timer = 0.0f;
        float maxLife = 0.35f;
        sf::Color color = sf::Color(255, 230, 80);
    };
    std::vector<Spark> sparks;
};

// Клубящиеся частицы объемной пыли и дыма (Volumetric Dust Plume)
struct BossDustPuff {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float currentRadius = 14.0f;
    float maxRadius = 48.0f;
    float rot = 0.0f;
    float rotSpeed = 0.0f;
    float timer = 0.0f;
    float maxDuration = 1.2f;
    sf::Color color = sf::Color(140, 115, 90);
    bool active = true;
};

// Разломы и пылающие трещины в земле
struct BossGroundCrack {
    sf::Vector2f center;
    float length = 120.0f;
    std::vector<sf::Vector2f> jaggedPoints;
    float timer = 0.0f;
    float maxDuration = 1.8f;
    bool active = true;
};

// Бегущая по всей арене волна сотрясения земли (Earthquake Arena Wave)
struct BossArenaShockwave {
    float x = 0.0f;
    float groundY = 0.0f;
    float dir = 1.0f;
    float speed = 680.0f;
    float currentDist = 0.0f;
    float maxDist = 1100.0f;
    float puffTimer = 0.0f;
    bool active = true;
};

// Вырывающиеся из земли массивные каменные шипы
struct BossGroundSpike {
    sf::Vector2f basePos;
    float width = 30.0f;
    float targetHeight = 65.0f;
    float currentHeight = 0.0f;
    float tilt = 0.0f;
    float timer = 0.0f;
    float maxDuration = 1.1f;
    sf::Color rockColor = sf::Color(75, 65, 60);
    bool active = true;
};

// Разлетающиеся осколки камней и комья породы
struct BossDebrisChunk {
    sf::Vector2f pos;
    sf::Vector2f vel;
    float rot = 0.0f;
    float rotSpeed = 0.0f;
    float size = 8.0f;
    float timer = 0.0f;
    float maxDuration = 0.90f;
    sf::Color color = sf::Color(100, 85, 75);
    bool active = true;
};

// Расширяющиеся кольца ударной волны по полу
struct BossShockwaveRing {
    sf::Vector2f center;
    float radiusX = 15.0f;
    float radiusY = 6.0f;
    float maxRadiusX = 220.0f;
    float speed = 520.0f;
    float timer = 0.0f;
    float maxDuration = 0.50f;
    sf::Color color = sf::Color(255, 60, 30);
    bool active = true;
};

// Сокрушительный энергетический столб удара из воздуха до пола (Jump Slam)
struct BossJumpSlamPillar {
    sf::FloatRect hitbox;
    float timer = 0.0f;
    float maxDuration = 0.38f;
    bool active = true;
    std::vector<std::vector<sf::Vector2f>> lightningBranches;
};

// Взрыв породы и пыли при врезании тарана в стену
struct BossWallCrashEffect {
    sf::Vector2f pos;
    float dirX = -1.0f;
    float timer = 0.0f;
    float maxDuration = 0.75f;
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
    float maxDuration = 0.22f;
    bool active = true;
};

class BossVfxManager {
private:
    std::vector<BossSlashInstance> m_slashes;
    std::vector<BossGroundSpike> m_spikes;
    std::vector<BossDebrisChunk> m_debris;
    std::vector<BossDustPuff> m_dustPuffs;
    std::vector<BossGroundCrack> m_cracks;
    std::vector<BossArenaShockwave> m_arenaWaves;
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
    void spawnSlash(const sf::FloatRect& hitbox, bool facingLeft, BossSlashType type, float duration = 0.26f);

    // Спавн сокрушительного удара в землю: взрыв пыли, волны на всю арену, шипы и трещины
    void spawnGroundImpact(const sf::Vector2f& groundPos, float width = 420.0f, bool intense = false);

    // Спавн бегущих волн сотрясения на всю арену с непрерывным поднятием клубов пыли
    void spawnArenaShockwaves(const sf::Vector2f& groundPos, float maxDist = 1100.0f);

    // Спавн объемного клубящегося облака пыли
    void spawnDustCloud(const sf::Vector2f& pos, const sf::Vector2f& vel, float radius, sf::Color color, float life = 1.1f);

    // Спавн вертикального энергетического столба удара с воздуха по полу
    void spawnJumpSlamBeam(const sf::FloatRect& hitbox, float duration = 0.35f);

    // Спавн взрыва породы при врезании в стену
    void spawnWallCrash(const sf::Vector2f& impactPos, float dirX);

    // Спавн остаточного шлейфа при беге/таране
    void spawnAfterimage(const sf::Vector2f& pos, const sf::Vector2f& size, const sf::Vector2f& origin, bool facingLeft);

    void update(float dt);
    void draw(sf::RenderTarget& target);
    void clear();

    size_t getActiveCount() const;
};
