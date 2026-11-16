#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>
#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"

enum class EditorTool {
    PLATFORM = 1,              // '=' Тайл 1 (платформа / пол)
    WALL = 2,                  // '#' Тайл 2 (монолитная стена / блок)
    DOOR = 3,                  // 'D' Тайл 3 (дверь / переход между комнатами)
    PLAYER_SPAWN = 4,          // 'P' Спавн игрока
    ENEMY_SPAWN = 5,           // 'N' Спавн обычного врага (Normal)
    ENEMY_TANK = 6,            // 'T' Спавн жирного врага (Tank)
    ENEMY_DUELIST = 7,         // 'U' Спавн дуэлянта агрессивного (Duelist Aggressive)
    ENEMY_DUELIST_PASSIVE = 8, // 'V' Спавн дуэлянта пассивного (Duelist Passive)
    ERASER = 9                 // '.' Ластик (стереть блок / врага)
};

class MapEditor {
public:
    MapEditor();

    bool isActive() const { return _active; }
    void toggle(sf::RenderWindow& window, sf::View& camera, const Player& player);

    void handleInput(const std::optional<sf::Event>& event, Arena& arena, 
                     Player& player, Enemy& enemy, sf::RenderWindow& window,
                     sf::View& camera);

    void update(float dt, Arena& arena, Player& player, Enemy& enemy, 
                sf::RenderWindow& window, sf::View& camera);

    void drawWorld(sf::RenderWindow& window, const Arena& arena, const sf::Vector2f& mouseWorld,
                   const sf::Font* font);

    void drawUI(sf::RenderWindow& window, const Arena& arena, const sf::Font* font,
                const sf::View& camera);

    EditorTool getCurrentTool() const { return _tool; }
    void setCurrentTool(EditorTool tool) { _tool = tool; }
    int getBrushSize() const { return _brushSize; }
    void cycleBrushSize();

    void nextRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera);
    void prevRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera);

    void zoomIn(sf::View& camera);
    void zoomOut(sf::View& camera);
    void resetZoom(sf::View& camera);

    float getViewScale() const { return _viewScale; }

    struct IngameTileSpriteItem {
        std::string name;
        std::string filename;
        std::shared_ptr<sf::Texture> texture;
        std::unique_ptr<sf::Sprite> sprite;
        EditorTool tool = EditorTool::PLATFORM;
    };

    void scanTileSprites();
    bool handlePaletteClick(const sf::Vector2i& mousePixel, Arena& arena, float winW, float winH);

private:
    bool _active = false;
    EditorTool _tool = EditorTool::PLATFORM;
    int _brushSize = 1; // 1, 2, 3, 5
    int _doorTargetRoom = 2; // Целевая комната для новых дверей (1..10)
    std::vector<IngameTileSpriteItem> _paletteSprites;
    float _viewScale = 1.0f; // 0.4f .. 3.0f (масштаб камеры для огромных карт)
    sf::Vector2f _baseCameraSize{1920.0f, 1080.0f};

    sf::Vector2f _cameraPos;
    float _camSpeed = 1200.0f;
    bool _isPanning = false;
    sf::Vector2i _lastMousePixel{0, 0};

    std::string _statusMessage = "";
    float _statusTimer = 0.0f;
    sf::Vector2i _lastHoverTile{-1, -1};

    // Параметры интерактивной миникарты
    sf::FloatRect _minimapRect{{0.0f, 0.0f}, {240.0f, 135.0f}};
    bool handleMinimapClick(const sf::Vector2i& mousePixel, Arena& arena, sf::View& camera);

    void applyToolAt(int tx, int ty, Arena& arena, Player& player, Enemy& enemy);
    void eraseAt(int tx, int ty, Arena& arena);
    void saveCurrentRoom(const Arena& arena);
};
