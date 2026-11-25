#pragma once
#include "Common.h"
#include <deque>
#include <memory>
#include <optional>
#include <unordered_set>
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
    ENEMY_DUMMY = 18,          // 'M' Спавн манекена (Dummy)
    LAMP = 19,                 // 'L' Тайл 4 (Люминесцентная лампа)
    WINDOW = 20,               // 'W' Тайл 5 (Окно и дневной свет)
    BG_DECOR = 21,             // 'B' Тайл 8 (Фоновый блок без хитбоксов)
    AUTOTILE = 22,             // 'A' Автоматический выбор тайла по соседям
    DECOR_BLOCK = 23,          // Блок декора
    INTERACTIVE_DOOR = 24,     // Физическая интерактивная дверь
    MOVE = 25,                 // 'M' Ручное перемещение блоков / объектов
    BARRIER = 26,              // Невидимый барьерный блок (код 9)
    ENEMY_BOSS_1 = 27,         // Спавн Босса 1 (4x3 тайла)
    TRIGGER_BOSS_FIGHT = 28    // Триггер запуска боссфайта
};

enum class EditorScreen {
    WORLD_LAYOUT = 0, // Обзорный экран: карта комнат, перемещение, изменение размеров, управление дверьми
    ROOM_DETAIL = 1   // Редактор тайлов комнаты: детальный редактор одной комнаты
};

enum class PaletteTab {
    BLOCKS = 0,     // 🧱 Блоки и Стены (из assets/tiles/blocks/)
    DECOR = 1,      // 🎨 Декор и Лампы (из assets/tiles/decor/)
    BACKGROUND = 2, // 🌌 Фон (из assets/tiles/background/)
    ENEMIES = 3,    // 👾 Враги (из assets/tiles/enemies/ и assets/sprites/enemies/)
    MECHANICS = 4,  // ⚡ Механики и Инструменты (Переход, Дверь, Джамп-пад, Шипы, Игрок, Триггеры)
    ROOMS = 5       // 📁 Комнаты
};

struct RoomMeta {
    std::string id;            // "room_1", "boss_arena" и т.д.
    std::string title;         // Название для отображения
    std::string path;
    int width = 0;
    int height = 0;
    std::shared_ptr<sf::Texture> thumbnail;
    FloorType floorType = FloorType::NORMAL;
};

struct EditorDoor {
    int x = 0;
    int y = 0;
    int targetRoom = -1;
    int targetDoorX = -1; // X координата связанной двери в целевой комнате (-1 если не задано)
    int targetDoorY = -1; // Y координата связанной двери в целевой комнате (-1 если не задано)
    std::string targetRoomName = ""; // Имя целевой комнаты (например "room_1", "Test")
};

struct EditorInteractiveDoor {
    int x = 0;
    int y = 0;
    bool isOpen = false;
    bool isBroken = false;
};

struct EditorEnemySpawn {
    int x = 0;
    int y = 0;
    int type = 0; // 0 = Normal, 1 = Tank, 2 = Duelist Aggressive, 3 = Duelist Passive, 4 = Dummy
    bool facingRight = true;
    std::string enemyId = ""; // Имя папки в assets/sprites/enemies (например "duelist_aggressive")
};

struct EditorSpawnBlock {
    int x = 0;
    int y = 0;
    int tileCode = 9; // По умолчанию невидимый барьер
};

struct EditorTrigger {
    int x = 0;
    int y = 0;
    int type = 0; // 0 = JumpPad, 1 = Spikes, 2 = TriggerZone, 3 = LightSwitch, 4 = ScriptZone, 5 = BossFight
    float power = 1000.0f;
    std::string tag = "";
    int width = 1;
    int height = 1;
    int condition = 0; // 0: OVERLAP / Касание, 1: CROSS_X / Пересечение X, 2: CROSS_Y / Пересечение Y, 3: IN_VIEW / Поле зрения
    bool isBossFight = false;
    std::string bossName = "БОСС 1";
    bool lockCamera = false;
    int cameraLockMode = 0; // 0: Свободная, 1: Точка/Блок, 2: Враг, 3: Боссфайт
    sf::Vector2f cameraLockPos{0.0f, 0.0f};
    bool cameraLockOnBoss = false;
    int cameraTargetEnemyIndex = -1;
    std::string cameraTargetEnemyTag = "";
    sf::Vector2f cameraSize{1280.0f, 720.0f};
    int spawnEnemyType = 0; // 0: Нет, 1: Normal, 2: Tank, 3: Duelist, 4: Dummy, 5: Boss 1
    std::vector<EditorSpawnBlock> spawnBlocks;
    std::vector<EditorEnemySpawn> spawnEnemies;
};

struct EditorLamp {
    int x = 0;
    int y = 0;
    sf::Color color{240, 250, 255};
    float intensity = 1.0f;
    float radius = 980.0f;
    float coneAngle = 130.0f;
    bool enabled = true;

    // Сверхбыстрый GPU-кэш для стабильных 1500+ FPS в редакторе:
    mutable std::shared_ptr<sf::Texture> bakedTexture;
    mutable sf::Vector2f bakedWorldPos{0.0f, 0.0f};
    mutable sf::Vector2f bakedScale{1.0f, 1.0f};
    mutable bool dirty = true;
};

struct EditorWindow {
    int x = 0;
    int y = 0;
    sf::Color color{245, 250, 255};
    float intensity = 0.55f;
    float radius = 1750.0f;
    float coneAngle = 135.0f;

    // Сверхбыстрый GPU-кэш для стабильных 1500+ FPS в редакторе:
    mutable std::shared_ptr<sf::Texture> bakedTexture;
    mutable sf::Vector2f bakedWorldPos{0.0f, 0.0f};
    mutable sf::Vector2f bakedScale{1.0f, 1.0f};
    mutable bool dirty = true;
};

struct CanvasRoom {
    std::string name = "room_1";
    int index = 1;
    int width = 80;
    int height = 20;
    sf::Vector2f offset{0.0f, 0.0f};
    bool hasCustomLayoutPos = false;
    FloorType floorType = FloorType::NORMAL;
    std::vector<std::vector<int>> map;
    std::vector<std::vector<int>> bgMap;
    std::vector<std::vector<int>> decorMap;
    std::vector<EditorDoor> doors;
    std::vector<EditorInteractiveDoor> interactiveDoors;
    std::vector<EditorEnemySpawn> enemySpawns;
    std::vector<EditorTrigger> triggers;
    std::vector<EditorLamp> lamps;
    std::vector<EditorWindow> windows;
    bool hasPlayerSpawn = false;
    sf::Vector2i playerSpawn{6, 14};
    std::optional<sf::Vector2i> testSpawn;
};

struct AutoTileSet {
    std::string stem;
    int cols = 0;
    int rows = 0;
    int masterCode = 0;
    std::vector<std::vector<int>> gridCodes; // [ry][rx] -> tileCode (0 if empty/transparent)
    std::unordered_set<int> memberCodes;
};

struct AutoStairSet {
    std::string stem;
    int upRightSlopeCode = 0;   // (rx=0, ry=0): полублок наклон вверх-вправо
    int downRightSlopeCode = 0; // (rx=1, ry=0): полублок наклон вниз-вправо
    int upRightBaseCode = 0;    // (rx=0, ry=1): полный блок основание
    int downRightBaseCode = 0;  // (rx=1, ry=1): полный блок основание
    std::unordered_set<int> allCodes;
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
    std::string enemyId = "";
    bool isAutoTileMaster = false;
    int autoTileSetIdx = -1;
};

struct StandaloneSnapshot {
    int width = 0;
    int height = 0;
    std::vector<int> tiles;
    std::vector<int> bgTiles;
    std::vector<int> decorTiles;
    std::vector<EditorDoor> doors;
    std::vector<EditorInteractiveDoor> interactiveDoors;
    std::vector<EditorEnemySpawn> enemySpawns;
    std::vector<EditorTrigger> triggers;
    std::vector<EditorLamp> lamps;
    std::vector<EditorWindow> windows;
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
    enum class TargetType { NONE, DOOR, ENEMY, TEST_SPAWN, TRIGGER, TILE, LAMP } type = TargetType::NONE;
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
    std::vector<std::vector<int>> _bgMap;
    std::vector<std::vector<int>> _decorMap;
    sf::Vector2i _playerSpawn{6, 14};
    std::optional<sf::Vector2i> _testSpawn; // Точка старта симуляции
    FloorType _floorType = FloorType::NORMAL;
    std::vector<EditorEnemySpawn> _enemySpawns;
    std::vector<EditorDoor> _doors;
    std::vector<EditorInteractiveDoor> _interactiveDoors;
    std::vector<EditorTrigger> _triggers;
    std::vector<EditorLamp> _lamps;
    std::vector<EditorWindow> _windows;
    int _doorTargetRoom = 2;

    enum class FloorFilterTab {
        ALL = 0,
        NORMAL = 1,
        BASEMENT = 2
    };
    FloorFilterTab _worldFloorTab = FloorFilterTab::ALL;
    FloorFilterTab _browserFloorTab = FloorFilterTab::ALL;

    // Вкладки палитры
    PaletteTab _activeTab = PaletteTab::BLOCKS;
    std::vector<TileSpriteItem> _paletteSprites;
    std::vector<EnemyTypeDef> _scannedEnemies;
    int _selectedPaletteIndex = 0;

    const EnemyTypeDef* findEnemyDef(const std::string& enemyId, int legacyType = -1) const;

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
    std::shared_ptr<sf::Texture> _texDoor;
    std::shared_ptr<sf::Texture> _texLamp;
    std::shared_ptr<sf::Texture> _texWindow;

    ToolType _currentTool = ToolType::PLATFORM;
    int _currentTileCode = 1;
    int _brushSize = 1;

    // Кешированные спрайты блоков по коду тайла (1=платформа, 2=стена, 3=дверь, 4=лампа, 5=ящик, 6=колонна, 7=лава...)
    std::unordered_map<int, std::shared_ptr<sf::Sprite>> _tileCodeSprites;

    // Навигация и мышь
    bool _isPanning = false;
    bool _isSpacePanning = false;
    bool _isMouseCapturedByUI = false;
    sf::Vector2i _lastMousePixel{0, 0};
    sf::Vector2i _lastHoverTile{-1, -1};
    sf::Vector2f _hoverWorldPos{0.0f, 0.0f};

    // Инструмент ручного перемещения (ToolType::MOVE)
    bool _isMovingObject = false;
    sf::Vector2i _moveSourceTile{-1, -1};
    int _moveTileCode = 0;
    int _moveBgCode = 0;
    int _moveDecorCode = 0;
    std::optional<EditorEnemySpawn> _moveEnemy;
    std::optional<EditorInteractiveDoor> _moveInteractiveDoor;
    std::optional<EditorDoor> _moveDoor;
    std::optional<EditorTrigger> _moveTrigger;
    std::optional<EditorLamp> _moveLamp;
    std::optional<EditorWindow> _moveWindow;

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

    // Интерактивные режимы настройки триггера на карте:
    bool _isPickingCameraPos = false;
    sf::Vector2i _pickingCameraTriggerTile{-1, -1};

    bool _isPickingEnemyTarget = false;
    sf::Vector2i _pickingEnemyTriggerTile{-1, -1};

    bool _isEditingTriggerBlocks = false;
    sf::Vector2i _editingTriggerTile{-1, -1};
    int _triggerBlockTypeToPlace = 9; // 9 = Барьер, 2 = Стена, 1 = Платформа
    sf::Vector2i _manualBlockCoord{0, 0};

    // Встроенная симуляция игры прямо из редактора
    bool _isSimulating = false;
    std::unique_ptr<Arena> _simArena;
    std::unique_ptr<Player> _simPlayer;
    std::unique_ptr<CombatSystem> _simCombat;
    std::vector<Enemy> _simEnemies;
    sf::Vector2f _savedEditorCameraPos;
    float _savedEditorViewScale = 1.0f;
    float _simDoorCooldown = 0.0f;
    float _simDeathTimer = 1.8f;

    sf::Font _font;
    bool _fontLoaded = false;
    std::string _statusMessage = "";
    float _statusTimer = 0.0f;

    sf::FloatRect _minimapRect{{0.0f, 0.0f}, {260.0f, 140.0f}};

public:
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

    const std::vector<TileSpriteItem>& getPaletteSprites() const { return _paletteSprites; }
    const std::vector<AutoStairSet>& getAutoStairSets() const { return _autoStairSets; }
    const std::vector<AutoTileSet>& getAutoTileSets() const { return _autoTileSets; }
    const std::vector<std::vector<int>>& getMap() const { return _map; }
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    const std::vector<std::vector<int>>& getBackgroundMap() const { return _bgMap; }
    const std::vector<std::vector<int>>& getDecorMap() const { return _decorMap; }
    void setCurrentTool(ToolType t, int tileCode = 0) { _currentTool = t; if (tileCode > 0) _currentTileCode = tileCode; }
    ToolType getCurrentTool() const { return _currentTool; }
    const std::vector<EditorEnemySpawn>& getEnemySpawns() const { return _enemySpawns; }
    const std::vector<EditorInteractiveDoor>& getInteractiveDoors() const { return _interactiveDoors; }
    const std::vector<EditorLamp>& getLamps() const { return _lamps; }

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
    void cancelMove();
    void eraseAt(int tx, int ty);
    void addEnemySpawn(int tx, int ty, int type = 0, const std::string& enemyId = "");
    void removeEnemySpawn(int tx, int ty);
    void setDoor(int tx, int ty, int targetRoom, int targetDoorX = -1, int targetDoorY = -1);
    void removeDoor(int tx, int ty);
    int getDoorTargetRoom(int tx, int ty) const;
    void addInteractiveDoor(int tx, int ty);
    void removeInteractiveDoor(int tx, int ty);
    void addTrigger(int tx, int ty, int type);
    void removeTrigger(int tx, int ty);
    EditorLamp* findOrCreateLamp(int tx, int ty);
    EditorLamp* findLamp(int tx, int ty);
    const EditorLamp* findLamp(int tx, int ty) const;
    void removeLamp(int tx, int ty);
    EditorWindow* findOrCreateWindow(int tx, int ty);
    EditorWindow* findWindow(int tx, int ty);
    const EditorWindow* findWindow(int tx, int ty) const;
    void removeWindow(int tx, int ty);
    void transferRoomFloor(int roomIdx, FloorType newFloor);
    bool handleMinimapClick(const sf::Vector2i& mousePixel);

    // Авто-тайлинг (Autotiling)
    int resolveAutoTile(int tx, int ty, int setIdx) const;
    int findAutoTileSetForCode(int tileCode) const;
    void updateAutoTilesAround(int tx, int ty);
    void autoTileCurrentRoom();

    // Авто-лестницы (Auto-stairs 64x64)
    int resolveAutoStair(int tx, int ty, int stairSetIdx, int desiredDir = -1) const;
    int findAutoStairSetForCode(int tileCode) const;

    std::vector<AutoTileSet> _autoTileSets;
    std::vector<AutoStairSet> _autoStairSets;
    std::vector<TileSpriteItem> _extraTileSprites;
    bool _autoTileEnabled = true;
    int _selectedAutoSetIdx = -1;

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
    void drawLeftToolbar();
    bool handleLeftToolbarClick(const sf::Vector2i& mousePixel);
    void drawModal();
    void drawNamingModal();
    void drawJumpArc();
    void drawSimulationHUD();
};
