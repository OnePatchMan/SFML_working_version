#pragma once
#include "Common.h"
#include <deque>
#include <memory>
#include <optional>
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "combat/CombatSystem.h"

enum class ToolType {
    PLATFORM = 1,              // '=' Тайл 1 (платформа)
    WALL = 2,                  // '#' Тайл 2 (монолитная стена)
    DOOR = 3,                  // 'D' Тайл 3 (дверь с целевой комнатой)
    PLAYER_SPAWN = 4,          // 'P' Спавн игрока
    ENEMY_SPAWN = 5,           // 'N' Спавн врага (Обычный)
    ENEMY_TANK = 6,            // 'T' Спавн врага (Жирный)
    ENEMY_DUELIST = 7,         // 'U' Спавн врага (Дуэлянт Агр)
    ENEMY_DUELIST_PASSIVE = 8, // 'V' Спавн врага (Дуэлянт Пасс)
    EDGE_PASSAGE = 9,          // '.' Ластик / Пустота
    BOX_FILL = 10,             // Прямоугольная заливка
    LINE = 11,                 // Линия
    FLOOD_FILL = 12,           // Заливка области (Bucket)
    EYEDROPPER = 13,           // Пипетка
    TEST_SPAWN = 14,           // '▶' Точка старта симуляции (Test Start Pos)
    JUMP_PAD = 15,             // '▲' Батут / Джамп-пад
    SPIKES = 16,               // '▲▲▲' Шипы / Ловушка
    TRIGGER_ZONE = 17,         // '⚡' Триггерная зона
    ENEMY_DUMMY = 18           // 'M' Спавн манекена (Dummy)
};

enum class EditorScreen {
    WORLD_LAYOUT = 0, // Обзорный экран: карта комнат, перемещение, изменение размеров, управление дверьми
    ROOM_DETAIL = 1   // Редактор тайлов комнаты: детальный редактор одной комнаты
};

enum class PaletteTab {
    BLOCKS = 0,    // 🧱 Блоки и Геометрия
    ENTITIES = 1,  // ⚔ Сущности и Враги
    MECHANICS = 2, // ⚡ Механики и Триггеры
    ROOMS = 3      // 📁 Комнаты (Галерея 2 столбца)
};

struct RoomMeta {
    std::string id;            // "room_1", "boss_arena" и т.д.
    std::string title;         // Название для отображения
    std::string path;
    int width = 0;
    int height = 0;
    std::shared_ptr<sf::Texture> thumbnail;
};

struct EditorDoor {
    int x = 0;
    int y = 0;
    int targetRoom = -1;
    int targetDoorX = -1; // X координата связанной двери в целевой комнате (-1 если не задано)
    int targetDoorY = -1; // Y координата связанной двери в целевой комнате (-1 если не задано)
    std::string targetRoomName = ""; // Имя целевой комнаты (например "room_1", "Test")
};

struct EditorEnemySpawn {
    int x = 0;
    int y = 0;
    int type = 0; // 0 = Normal, 1 = Tank, 2 = Duelist Aggressive, 3 = Duelist Passive, 4 = Dummy
    bool facingRight = true;
};

struct EditorTrigger {
    int x = 0;
    int y = 0;
    int type = 0; // 0 = JumpPad, 1 = Spikes, 2 = TriggerZone
    float power = 1000.0f;
    std::string tag = "";
};

struct CanvasRoom {
    std::string name = "room_1";
    int index = 1;
    int width = 80;
    int height = 20;
    sf::Vector2f offset{0.0f, 0.0f};
    bool hasCustomLayoutPos = false;
    std::vector<std::vector<int>> map;
    std::vector<EditorDoor> doors;
    std::vector<EditorEnemySpawn> enemySpawns;
    std::vector<EditorTrigger> triggers;
    bool hasPlayerSpawn = false;
    sf::Vector2i playerSpawn{6, 14};
    std::optional<sf::Vector2i> testSpawn;
};

struct TileSpriteItem {
    int id = 0;
    std::string name;
    std::string filename;
    std::shared_ptr<sf::Texture> texture;
    std::unique_ptr<sf::Sprite> sprite;
    int tileCode = 1;
    ToolType tool = ToolType::PLATFORM;
    PaletteTab tab = PaletteTab::BLOCKS;
};

struct StandaloneSnapshot {
    int width = 0;
    int height = 0;
    std::vector<int> tiles;
    std::vector<EditorDoor> doors;
    std::vector<EditorEnemySpawn> enemySpawns;
    std::vector<EditorTrigger> triggers;
    sf::Vector2i playerSpawn;
    std::optional<sf::Vector2i> testSpawn;
};

struct StandaloneClipboardTile {
    int relX = 0;
    int relY = 0;
    int tile = 0;
    int doorTarget = -1;
    int enemyType = -1;
};

struct StandaloneClipboard {
    int w = 0;
    int h = 0;
    std::vector<StandaloneClipboardTile> items;
    bool hasData = false;
};

struct SettingsModal {
    bool active = false;
    sf::Vector2f screenPos;
    sf::Vector2i tile{-1, -1};
    enum class TargetType { NONE, DOOR, ENEMY, TEST_SPAWN, TRIGGER, TILE } type = TargetType::NONE;
    int targetIndex = -1;
};

class EditorApp {
public:
    EditorApp();
    void run();

    bool isMouseOverUI(const sf::Vector2i& mousePixel) const;
    void openSettingsModal(int tx, int ty, const sf::Vector2f& screenPos);
    void closeSettingsModal();

private:
    sf::RenderWindow _window;
    sf::View _camera;
    sf::Vector2f _baseCameraSize{1600.0f, 900.0f};
    sf::Vector2f _cameraPos{2500.0f, 700.0f};
    float _camSpeed = 1200.0f;
    float _viewScale = 1.0f;
    bool _isFullscreen = true;

    int _width = 80;
    int _height = 20;
    float _tileSize = EDITOR_TILE_SIZE;
    std::vector<std::vector<int>> _map;
    sf::Vector2i _playerSpawn{6, 14};
    std::optional<sf::Vector2i> _testSpawn; // Точка старта симуляции
    std::vector<EditorEnemySpawn> _enemySpawns;
    std::vector<EditorDoor> _doors;
    std::vector<EditorTrigger> _triggers;
    int _doorTargetRoom = 2;

    // Вкладки палитры
    PaletteTab _activeTab = PaletteTab::BLOCKS;
    std::vector<TileSpriteItem> _paletteSprites;
    int _selectedPaletteIndex = 0;

    int _currentRoomIndex = 1;
    int _totalRooms = 10;
    std::string _currentRoomName = "room_1";

    // Браузер комнат (2 столбца превью + названия)
    std::vector<RoomMeta> _roomList;
    int _roomBrowserSelected = 0;
    float _roomBrowserScrollY = 0.0f;
    bool _isNamingRoom = false;
    std::string _newRoomNameInput = "";

    // Текстуры для батута и шипов
    std::shared_ptr<sf::Texture> _texJumpPad;
    std::shared_ptr<sf::Texture> _texSpikes;

    ToolType _currentTool = ToolType::PLATFORM;
    int _brushSize = 1;

    // Навигация и мышь
    bool _isPanning = false;
    bool _isSpacePanning = false;
    bool _isMouseCapturedByUI = false;
    sf::Vector2i _lastMousePixel{0, 0};
    sf::Vector2i _lastHoverTile{-1, -1};

    // Стек Undo / Redo
    static constexpr size_t MAX_UNDO = 50;
    std::deque<StandaloneSnapshot> _undoStack;
    std::deque<StandaloneSnapshot> _redoStack;

    // Box Fill / Erase
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
    StandaloneClipboard _clipboard;
    bool _pasteModeActive = false;

    // Режимы отображения
    bool _showGrid = true;
    bool _showJumpRuler = false;
    bool _showScreenFrame = false;

    // Модальное окно настроек объекта (на ПКМ)
    SettingsModal _modal;

    // Встроенная симуляция игры прямо из редактора
    bool _isSimulating = false;
    std::unique_ptr<Arena> _simArena;
    std::unique_ptr<Player> _simPlayer;
    std::unique_ptr<CombatSystem> _simCombat;
    std::vector<Enemy> _simEnemies;
    sf::Vector2f _savedEditorCameraPos;
    float _savedEditorViewScale = 1.0f;
    float _simDoorCooldown = 0.0f;

    sf::Font _font;
    bool _fontLoaded = false;
    std::string _statusMessage = "";
    float _statusTimer = 0.0f;

    sf::FloatRect _minimapRect{{0.0f, 0.0f}, {260.0f, 140.0f}};

    void initDefaultRoom();
    bool loadRoom(int index);
    bool loadRoomByName(const std::string& name);
    void scanRooms();
    void createNamedRoom(const std::string& name);
    bool saveToGame();
    void createNewRoom();
    void drawRoomBrowser();
    bool handleRoomBrowserClick(const sf::Vector2i& mousePixel);

    void scanTileSprites();
    bool handlePaletteClick(const sf::Vector2i& mousePixel);
    bool handleModalClick(const sf::Vector2i& mousePixel);

    void nextRoom();
    void prevRoom();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void cycleBrushSize();
    void resizeMap(int newW, int newH);
    void toggleFullscreen();

    // Симуляция игры
    void startSimulation(std::optional<sf::Vector2i> startTile = std::nullopt);
    void stopSimulation();

    // Undo / Redo & Tools
    void pushUndo();
    bool undo();
    bool redo();
    void floodFill(int startX, int startY, int fillTile);
    bool eyedropAt(int tx, int ty);
    void copySelection();
    void pasteClipboard(int destX, int destY);
    bool duplicateRoom(int targetRoom);
    void clearRoom();

    void handleEvents();
    void update(float dt);
    void render();

    void applyToolAt(int tx, int ty);
    void eraseAt(int tx, int ty);
    void addEnemySpawn(int tx, int ty, int type = 0);
    void removeEnemySpawn(int tx, int ty);
    void setDoor(int tx, int ty, int targetRoom, int targetDoorX = -1, int targetDoorY = -1);
    void removeDoor(int tx, int ty);
    int getDoorTargetRoom(int tx, int ty) const;
    void addTrigger(int tx, int ty, int type);
    void removeTrigger(int tx, int ty);
    bool handleMinimapClick(const sf::Vector2i& mousePixel);

    // Мульти-комнатный единый холст и связывание дверей
    std::vector<CanvasRoom> _canvasRooms;
    std::string _globalPlayerSpawnRoom = "room_1";
    bool _hasPlayerSpawn = true;
    bool _isLinkingDoor = false;
    bool _isDraggingDoorLink = false;
    sf::Vector2f _doorLinkMouseStart{0.0f, 0.0f};
    std::string _linkingSourceRoom = "";
    int _linkingSourceDoorX = -1;
    int _linkingSourceDoorY = -1;

    void syncCurrentRoomToCanvas();
    void syncCanvasToCurrentRoom(const std::string& roomName);
    void updateCanvasRoomLayout();
    int getCanvasRoomAt(sf::Vector2f worldPos) const;
    sf::Vector2f getActiveRoomOffset() const;
    void linkDoors(const std::string& roomA, int ax, int ay, const std::string& roomB, int bx, int by);
    void unlinkDoor(const std::string& roomName, int dx, int dy);
    const CanvasRoom* getTargetCanvasRoom(const EditorDoor& d) const;

    // Режимы экранов: Редактор комнат (WORLD_LAYOUT) и Редактор тайлов комнаты (ROOM_DETAIL)
    EditorScreen _currentScreen = EditorScreen::WORLD_LAYOUT;

    // Перемещение комнат (Drag & Drop) в режиме карты мира
    bool _isDraggingRoom = false;
    int _draggedRoomIdx = -1;
    sf::Vector2f _roomDragStartOffset{0.0f, 0.0f};
    sf::Vector2f _roomDragMouseStart{0.0f, 0.0f};

    // Изменение размеров комнат мышкой (Resize Corner Handle)
    bool _isResizingRoom = false;
    int _resizingRoomIdx = -1;
    sf::Vector2i _resizeStartDims{0, 0};
    sf::Vector2f _resizeMouseStart{0.0f, 0.0f};

    // Двойной клик для входа в комнату и инспектор дверей
    sf::Clock _clickClock;
    float _lastClickTime = 0.0f;
    int _lastClickedRoomIdx = -1;
    int _selectedDoorIdx = -1;

    void switchToScreen(EditorScreen screen);
    void resizeCanvasRoom(int roomIdx, int newW, int newH);
    bool saveRoomByIndex(int roomIdx);
    bool isMouseOverResizeHandle(int roomIdx, sf::Vector2f mouseWorld) const;

    void drawWorldLayout();
    void drawWorldLayoutUI();
    bool handleWorldLayoutClick(const sf::Vector2i& mousePixel, sf::Mouse::Button button);

    void drawWorld();
    void drawUI();
    void drawModal();
    void drawNamingModal();
    void drawJumpArc();
    void drawSimulationHUD();
};
