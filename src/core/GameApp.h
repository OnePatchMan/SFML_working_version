#pragma once

#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "combat/CombatSystem.h"
#include "graphics/Crosshair.h"
#include "ui/MenuSystem.h"
#include <memory>
#include <vector>
#include <filesystem>
#include <optional>

enum class FpsMode {
    UNCAPPED,
    FPS_200,
    FPS_240,
    FPS_144,
    FPS_60,
    VSYNC
};

class GameApp {
public:
    GameApp();
    ~GameApp() = default;

    int run();

private:
    void initWindow();
    void initSubsystems();
    void applyFpsMode(FpsMode mode);
    void spawnEnemyFromConfig(const EnemySpawnConfig& sp);
    void respawnEnemies();
    void checkHotReload();
    void processEvents();
    void update(float dt, const sf::Vector2f& mouseWorld);
    void render();

    sf::RenderWindow _window;
    sf::View _camera;
    Arena _arena;
    Player _player;
    CombatSystem _combat;
    MenuSystem _menuSystem;
    std::unique_ptr<Crosshair> _crosshair;
    std::vector<Enemy> _enemies;

    bool _isSmartAIMode = true;
    bool _isTrainingMode = false;
    DummyBlockMode _dummyBlockMode = DummyBlockMode::NONE;
    FpsMode _fpsMode = FpsMode::UNCAPPED;

    float _doorCooldown = 0.0f;
    float _playerDeathTimer = 2.0f;

    // Кэшированные объекты HUD (чтобы избежать переаллокаций каждый кадр)
    sf::RectangleShape _hudOverlayRect;
    sf::RectangleShape _trainingBannerRect;
    sf::RectangleShape _fpsBadgeRect;
    std::optional<sf::Text> _deathTitleText;
    std::optional<sf::Text> _deathSubText;
    std::optional<sf::Text> _trainingBannerText;
    std::optional<sf::Text> _fpsText;

    // Мониторинг производительности (FPS / CPU / Swap)
    sf::Clock _fpsClock;
    int _fpsFrameCounter = 0;
    float _currentFps = 144.0f;
    float _accumulatedCpuMs = 0.0f;
    float _accumulatedSwapMs = 0.0f;
    float _avgCpuMs = 0.4f;
    float _avgSwapMs = 6.5f;

    // Горячая перезагрузка конфигураций
    sf::Clock _kvWatchClock;
    std::filesystem::file_time_type _lastCombatTime;
    std::filesystem::file_time_type _lastPlayerTime;
    std::filesystem::file_time_type _lastEnemyTime;
};

