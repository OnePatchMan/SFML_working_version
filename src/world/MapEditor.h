#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <deque>
#include <functional>
#include <optional>
#include <memory>
#include <filesystem>
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
    ERASER = 9,                // '.' Ластик (стереть блок / врага)
    BOX_FILL = 10,             // Прямоугольная заливка
    LINE = 11,                 // Линия
    FLOOD_FILL = 12,           // Заливка замкнутой области (Bucket)
    EYEDROPPER = 13,           // Пипетка
    LAMP = 14                  // 'L' Тайл 4 (Люминесцентная лампа)
};

struct EditorSnapshot {
    int width = 0;
    int height = 0;
    std::vector<int> tiles;
    std::vector<DoorBlock> doors;
    std::vector<EnemySpawnConfig> enemySpawns;
    sf::Vector2f playerSpawn;
};

struct ClipboardTile {
    int relX = 0;
    int relY = 0;
    int tile = 0;
    int doorTarget = -1;
    int enemyType = -1;
};

struct ClipboardRegion {
    int w = 0;
    int h = 0;
    std::vector<ClipboardTile> items;
    bool hasData = false;
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

    // Quick Playtest methods
    bool isPlaytesting() const { return _isPlaytesting; }
    void startPlaytest(Arena& arena, Player& player, sf::RenderWindow& window, sf::View& camera);
    void stopPlaytest(sf::RenderWindow& window, sf::View& camera);
    void handlePlaytestInput(const std::optional<sf::Event>& event, sf::RenderWindow& window, sf::View& camera);
    void drawPlaytestUI(sf::RenderWindow& window, const sf::Font* font);
    void setRespawnCallback(std::function<void()> cb) { _respawnCallback = std::move(cb); }

    // History (Undo / Redo)
    void pushUndo(const Arena& arena);
    bool undo(Arena& arena, Player& player);
    bool redo(Arena& arena, Player& player);
    size_t getUndoCount() const { return _undoStack.size(); }
    size_t getRedoCount() const { return _redoStack.size(); }

    // Tools
    void floodFill(int startX, int startY, Arena& arena, int fillTile);
    bool eyedropAt(int tx, int ty, Arena& arena);
    void copySelection(const Arena& arena);
    void pasteClipboard(int destX, int destY, Arena& arena, Player& player);
    void cancelPaste() { _pasteModeActive = false; }
    bool isPasteMode() const { return _pasteModeActive; }

    // Room Actions
    bool duplicateRoom(int targetRoom, Arena& arena);
    void clearRoom(Arena& arena, Player& player);

    // Toggles
    void toggleGrid() { _showGrid = !_showGrid; }
    bool isGridVisible() const { return _showGrid; }
    void toggleJumpRuler() { _showJumpRuler = !_showJumpRuler; }
    bool isJumpRulerVisible() const { return _showJumpRuler; }
    void toggleScreenFrame() { _showScreenFrame = !_showScreenFrame; }
    bool isScreenFrameVisible() const { return _showScreenFrame; }

    EditorTool getCurrentTool() const { return _tool; }
    void setCurrentTool(EditorTool tool) { _tool = tool; }
    int getBrushSize() const { return _brushSize; }
    void cycleBrushSize();

    void nextRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera);
    void prevRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera);
    void switchRoom(int targetRoom, Arena& arena, Player& player, Enemy& enemy, sf::View& camera);

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
        int tileCode = 1;
    };

    void scanTileSprites();
    bool handlePaletteClick(const sf::Vector2i& mousePixel, Arena& arena, Player& player, Enemy& enemy, sf::View& camera, float winW, float winH);
    bool handleLeftToolbarClick(const sf::Vector2i& mousePixel, float winW, float winH);
    void drawLeftToolbar(sf::RenderWindow& window, const sf::Font* font, float winW, float winH);

private:
    bool _active = false;
    bool _isPlaytesting = false;
    sf::Vector2f _savedEditorCamPos;
    float _savedViewScale = 1.0f;
    std::function<void()> _respawnCallback;

    EditorTool _tool = EditorTool::PLATFORM;
    int _currentTileCode = 1;
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

    // History Stack
    static constexpr size_t MAX_UNDO_STEPS = 50;
    std::deque<EditorSnapshot> _undoStack;
    std::deque<EditorSnapshot> _redoStack;

    // Box Fill & Box Erase
    bool _boxDragActive = false;
    bool _boxIsErase = false;
    sf::Vector2i _boxStartTile{-1, -1};
    sf::Vector2i _boxCurTile{-1, -1};

    // Line Tool
    bool _lineDragActive = false;
    sf::Vector2i _lineStartTile{-1, -1};
    sf::Vector2i _lineCurTile{-1, -1};

    // Selection & Clipboard
    bool _selectionActive = false;
    sf::Vector2i _selStart{-1, -1};
    sf::Vector2i _selEnd{-1, -1};
    ClipboardRegion _clipboard;
    bool _pasteModeActive = false;

    // View Options
    bool _showGrid = true;
    bool _showJumpRuler = false;
    bool _showScreenFrame = false;

    // Параметры интерактивной миникарты
    sf::FloatRect _minimapRect{{0.0f, 0.0f}, {240.0f, 135.0f}};
    bool handleMinimapClick(const sf::Vector2i& mousePixel, Arena& arena, sf::View& camera);

    void applyToolAt(int tx, int ty, Arena& arena, Player& player, Enemy& enemy);
    void eraseAt(int tx, int ty, Arena& arena);
    void saveCurrentRoom(const Arena& arena);
    EditorSnapshot createSnapshot(const Arena& arena) const;
    void restoreSnapshot(const EditorSnapshot& snap, Arena& arena, Player& player);

    void drawJumpArc(sf::RenderWindow& window, const Arena& arena, const sf::Font* font);
};
