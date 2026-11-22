#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>
#include <functional>
#include "core/Common.h"

class Player;

struct DoorBlock {
    int x = 0;
    int y = 0;
    int targetRoom = -1;
    int targetDoorX = -1; // X координата связанной двери в целевой комнате (-1 если не задано)
    int targetDoorY = -1; // Y координата связанной двери в целевой комнате (-1 если не задано)
    std::string targetRoomName = ""; // Имя целевой комнаты (например "room_1", "Test")
};

struct InteractiveDoor {
    int x = 0;
    int y = 0;
    bool isOpen = false;
    bool isBroken = false;
    float breakTimer = 0.0f;
    float interactCooldown = 0.0f;
};

struct DoorSplinterParticle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float rotation = 0.0f;
    float angularVelocity = 0.0f;
    float lifetime = 1.6f;
    sf::Vector2f size{5.0f, 3.0f};
    sf::Color color{120, 80, 50};
    bool active = true;
};

namespace TileId {
    constexpr int AIR = 0;
    constexpr int PLATFORM = 1;
    constexpr int WALL = 2;
    constexpr int DOOR = 3;
    constexpr int LAMP = 4;
    constexpr int WINDOW = 5;
    constexpr int PILLAR = 6;
    constexpr int HAZARD = 7;
    constexpr int BG_DECOR = 8;
    constexpr int BARRIER = 9; // Невидимый барьерный блок (твердый в игре, невидимый, но виден в редакторе)
}

enum class FloorType {
    NORMAL = 0,   // Обычный этаж: дневной фон (голубое небо снаружи), окна освещают комнату естественным светом
    BASEMENT = 1  // Подвальный этаж: черный фон, подземный уровень без окон
};

struct WindowLight {
    int tileX = 0;
    int tileY = 0;
    sf::Vector2f worldPos;
    float radius = 1750.0f;          // Дальность дневного света (светит далеко через комнату)
    float intensity = 0.55f;         // Мягкая натуральная сила дневного света
    sf::Color color{245, 250, 255};  // Натуральный дневной солнечный свет
    float coneAngle = 135.0f;        // Угол раскрытия потока дневного света

    // Сверхбыстрый GPU-кэш для стабильных 1500+ FPS:
    mutable std::shared_ptr<sf::Texture> bakedTexture;
    mutable sf::Vector2f bakedWorldPos{0.0f, 0.0f};
    mutable sf::Vector2f bakedScale{1.0f, 1.0f};
    mutable bool dirty = true;
};

struct LampLight {
    int tileX = 0;
    int tileY = 0;
    sf::Vector2f worldPos;
    float radius = 980.0f;
    float intensity = 1.45f;
    sf::Color color{240, 250, 255};
    bool enabled = true;
    bool flickering = false;       // Постоянное аварийное мерцание (скрипт/сломанная лампа)
    float flickerTimer = 0.0f;     // Таймер для режима flickering
    float flickerDip = 0.0f;       // Текущий спад яркости [0.0 = норма, >0.0 = притухание]
    float nextFlickerTimer = 3.0f; // Таймер до следующего редкого моргания (5-7 сек)
    float flickerEventTimer = 0.0f;// Таймер текущего события моргания
    float flickerDuration = 0.2f;  // Длительность текущего моргания (~0.12 - 0.22 сек)
    int flickerPattern = 0;        // Паттерн моргания (0: одиночный, 1: двойной щелчок, 2: трепет)
    std::string tag = "";
    float coneAngle = 130.0f; // Угол раскрытия конуса света вниз в градусах (~130°)

    // Сверхбыстрый GPU-кэш для стабильных 1500+ FPS:
    mutable std::shared_ptr<sf::Texture> bakedTexture;
    mutable sf::Vector2f bakedWorldPos{0.0f, 0.0f};
    mutable sf::Vector2f bakedScale{1.0f, 1.0f};
    mutable bool dirty = true;
};

struct EnemySpawnConfig {
    sf::Vector2f position;
    int type = 0; // 0 = NORMAL, 1 = TANK, 2 = DUELIST (Aggressive), 3 = DUELIST_PASSIVE (Passive)
    std::string enemyId = ""; // Идентификатор папки/врага ("duelist_aggressive", "tank", etc.)
};

struct ArenaTrigger {
    int x = 0;
    int y = 0;
    int width = 1;     // Ширина зоны в тайлах
    int height = 1;    // Высота зоны в тайлах
    int type = 0;      // 0 = JumpPad, 1 = Spikes, 2 = TriggerZone, 3 = LightSwitch, 4 = ScriptZone, 5 = BossFight
    float power = 1000.0f; // Сила батута / урон шипов
    std::string tag = "";       // Идентификатор триггера (например "lab_switch_1")
    std::string targetTag = ""; // Целевой объект (например "lamp_1")
    std::string action = "";    // Команда / микро-скрипт (например "light:toggle lamp_1; msg:show \"Свет включен\" 2.0")
    int condition = 0;          // 0 = TOUCH, 1 = CROSS_X, 2 = CROSS_Y, 3 = IN_VIEW, 4 = ON_INTERACT (E)
    bool triggerOnce = false;   // Одноразовый триггер
    bool hasTriggered = false;  // Флаг срабатывания
    float cooldown = 1.0f;      // Перезарядка в секундах
    float currentCooldown = 0.0f;

    // Свойства боссфайта и камеры
    bool isBossFight = false;
    std::string bossName = "БОСС 1";
    bool lockCamera = false;
    int cameraLockMode = 0; // 0 = Free, 1 = Fixed Pos/Block, 2 = Enemy, 3 = Boss
    sf::Vector2f cameraLockPos{0.0f, 0.0f};
    bool cameraLockOnBoss = false;
    int cameraTargetEnemyIndex = -1;
    std::string cameraTargetEnemyTag = "";
    sf::Vector2f cameraSize{1280.0f, 720.0f};
    std::vector<std::tuple<int, int, int>> spawnBlocks; // x, y, tileCode (установка блоков при срабатывании)
    std::vector<EnemySpawnConfig> spawnEnemies; // Спавн врагов при срабатывании
};

enum class BloodDecalOrientation {
    HORIZONTAL,       // Прямая горизонтальная полоса на верхней/нижней грани блока (толщина 2-3px)
    VERTICAL,         // Прямая вертикальная полоса на левой/правой грани блока (толщина 2-3px)
    SLOPE_UP_RIGHT,   // Наклонная полоса на наклонной грани вверх-вправо (/)
    SLOPE_DOWN_RIGHT  // Наклонная полоса на наклонной грани вниз-вправо (\)
};

struct BloodDecal {
    sf::Vector2f position;
    float length = 8.0f;
    float thickness = 2.5f;
    BloodDecalOrientation orientation = BloodDecalOrientation::HORIZONTAL;
    sf::Color color;

    // Поля для обратной совместимости
    float radiusX = 4.0f;
    float radiusY = 4.0f;
    float rotation = 0.0f;
};

struct BackgroundBloodStreak {
    sf::Vector2f startPos;   // Начальная точка удара/брызг на задний фон
    sf::Vector2f currentPos; // Текущая нижняя точка стекающей капли
    float length = 0.0f;     // Текущая длина потека вниз
    float maxLength = 45.0f; // Максимальная длина потека вниз
    float dripSpeed = 28.0f; // Скорость стекания вниз (px/s)
    float width = 3.5f;      // Толщина ручейка/пятна (1.5..10px)
    sf::Color color;         // Цвет стекающей крови
    bool active = true;      // Продолжает ли еще стекать
    bool isBlotch = false;   // Плотный начальный сгусток на месте удара
    bool isSpray = false;    // Мелкие спреевидные брызги на задней стене (не стекают)
    float blotchH = 0.0f;    // Высота сгустка
};

struct BloodParticle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius = 3.0f;
    sf::Color color;
    float lifetime = 2.5f;
    bool active = true;
};

class Arena {
public:
    Arena(int width = ARENA_WIDTH, int height = ARENA_HEIGHT);
    ~Arena();

    static Arena* getActive() { return s_activeArena; }
    static void setActive(Arena* a) { s_activeArena = a; }

    void spawnFinisherBlood(sf::Vector2f pos, sf::Vector2f burstDir, int count = 65, float maxHp = 45.0f);
    void spawnKillBlood(sf::Vector2f pos, sf::Vector2f knockDir = {0.0f, -1.0f}, float maxHp = 45.0f);
    void spawnWallBounceBlood(sf::Vector2f pos, sf::Vector2f bounceDir);
    void update(float dt);
    void clearBlood() { _bloodDecals.clear(); _bloodParticles.clear(); _bgBloodStreaks.clear(); }
    const std::vector<BloodDecal>& getBloodDecals() const { return _bloodDecals; }
    size_t getBloodDecalCount() const { return _bloodDecals.size(); }
    const std::vector<BloodParticle>& getBloodParticles() const { return _bloodParticles; }
    const std::vector<BackgroundBloodStreak>& getBackgroundBloodStreaks() const { return _bgBloodStreaks; }
    size_t getBackgroundBloodStreakCount() const { return _bgBloodStreaks.size(); }

    enum class TileShape { FULL, HALF_BOTTOM, SLOPE_UP_RIGHT, SLOPE_DOWN_RIGHT };
    TileShape getTileShape(int tx, int ty) const;
    bool isSlopeNear(float wx, float wy, float radius = 96.0f) const;

    bool isSolid(int tx, int ty) const;
    bool isBlockSolid(int tx, int ty) const;
    bool isPointSolid(float wx, float wy) const;
    bool raycastSolid(sf::Vector2f start, sf::Vector2f target, sf::Vector2f& hitPoint, sf::Vector2i& hitTile, sf::Vector2f& hitNormal) const;
    bool isDoor(int tx, int ty) const;
    int getDoorTargetRoom(int tx, int ty) const;
    void setDoor(int tx, int ty, int targetRoom, int targetDoorX = -1, int targetDoorY = -1);
    void removeDoor(int tx, int ty);
    void clearDoors() { _doors.clear(); }
    const std::vector<DoorBlock>& getDoors() const { return _doors; }

    // Интерактивные физические двери (разрушаемые и открываемые на [E])
    const std::vector<InteractiveDoor>& getInteractiveDoors() const { return _interactiveDoors; }
    std::vector<InteractiveDoor>& getInteractiveDoors() { return _interactiveDoors; }
    void addInteractiveDoor(int tx, int ty, bool isOpen = false);
    bool removeInteractiveDoor(int tx, int ty);
    void clearInteractiveDoors() { _interactiveDoors.clear(); }
    InteractiveDoor* findInteractiveDoor(int tx, int ty);
    const InteractiveDoor* findInteractiveDoor(int tx, int ty) const;
    bool isInteractiveDoor(int tx, int ty) const;
    bool toggleInteractiveDoor(int tx, int ty);
    bool interactDoorNear(sf::Vector2f playerPos, float maxDist = 96.0f);
    bool breakDoorAt(int tx, int ty, sf::Vector2f impactVelocity = {1.0f, 0.0f});
    bool checkAndBreakDoorOverlap(const sf::FloatRect& bounds, const sf::Vector2f& velocity, bool isKnockback);
    void updateInteractiveDoors(float dt, Player* player = nullptr);
    void spawnDoorSplinters(sf::Vector2f pos, sf::Vector2f impactDir, int count = 25);
    const std::vector<DoorSplinterParticle>& getDoorSplinters() const { return _doorSplinters; }
    bool overlapsSolid(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    bool hasLineOfSight(sf::Vector2f from, sf::Vector2f to) const;
    bool checkPlayerDoorOverlap(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    std::optional<int> getOverlappingDoorTargetRoom(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    std::optional<DoorBlock> getOverlappingDoor(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    void drawBlackoutMask(sf::RenderWindow& window) const;

    // Гладкое органичное освещение флуоресцентных ламп (общее для игры и редактора, с преградами и отскоком от стен)
    static const sf::Texture& getSharedLampConeTexture();
    static void renderSmoothLampBeam(sf::RenderTarget& target, float lx, float ly, float tileSize,
                                     float radius, float coneAngle, const sf::Color& color,
                                     float intensity, bool isLightmap,
                                     std::function<bool(float wx, float wy)> isSolidWorldFn = nullptr,
                                     std::shared_ptr<sf::Texture>* outTextureCache = nullptr,
                                     sf::Vector2f* outPosCache = nullptr,
                                     sf::Vector2f* outScaleCache = nullptr,
                                     bool* inoutDirty = nullptr,
                                     float flickerFactor = 1.0f);

    sf::Vector2f getSpawnPosition() const { return _spawnPosition; }
    sf::Vector2f getEnemySpawnPosition() const { return _enemySpawnPosition; }
    const std::vector<sf::Vector2f>& getEnemySpawnPositions() const { return _enemySpawnPositions; }
    const std::vector<EnemySpawnConfig>& getEnemySpawns() const { return _enemySpawns; }

    void setSpawnPosition(sf::Vector2f pos) { _spawnPosition = pos; }
    void setEnemySpawnPosition(sf::Vector2f pos) { 
        _enemySpawnPosition = pos; 
        if (_enemySpawns.empty()) _enemySpawns.push_back({pos, 0});
        else _enemySpawns[0].position = pos;
        syncEnemySpawnPositions();
    }
    void setEnemySpawnPositions(const std::vector<sf::Vector2f>& spawns) {
        _enemySpawns.clear();
        for (const auto& sp : spawns) {
            _enemySpawns.push_back({sp, 0});
        }
        syncEnemySpawnPositions();
    }
    void setEnemySpawns(const std::vector<EnemySpawnConfig>& spawns) {
        _enemySpawns = spawns;
        syncEnemySpawnPositions();
    }
    void addEnemySpawnPosition(sf::Vector2f pos) {
        addEnemySpawn(pos, 0);
    }
    void addEnemySpawn(sf::Vector2f pos, int type = 0);
    void setEnemySpawnAt(int tx, int ty, int type = 0);
    bool removeEnemySpawnAt(float wx, float wy, float radius = 24.0f);
    bool removeEnemySpawnAtTile(int tx, int ty);
    void clearEnemySpawns();
    void syncEnemySpawnPositions();
    int getWidth() const { return _width; }
    int getHeight() const { return _height; }
    int getFloorY() const { return _floorY; }
    float getTileSize() const { return _tileSize; }

    int getTile(int tx, int ty) const;
    void setTile(int tx, int ty, int tile);

    void resize(int newWidth, int newHeight);
    void clearRoom(int fillTile = 0);
    bool duplicateRoom(int targetRoom);
    void restoreFromSnapshot(int w, int h, const std::vector<int>& tiles,
                             const std::vector<DoorBlock>& doors,
                             const std::vector<EnemySpawnConfig>& enemySpawns,
                             sf::Vector2f playerSpawn);

    bool reload();
    bool saveToAsciiFile(const std::string& asciiPath = "assets/config/map.txt") const;
    bool saveToImageFile(const std::string& imagePath) const;
    bool saveToKVCoordinates(const std::string& kvPath) const;
    bool saveCurrentRoom() const;

    bool loadMapConfig(const std::string& configPath = "assets/config/map.kv");
    bool loadFromAsciiFile(const std::string& asciiPath);
    bool loadFromImageFile(const std::string& imagePath);
    bool loadFromKVCoordinates(const std::string& kvPath);

    bool loadRoom(int roomIndex);
    bool loadRoomByName(const std::string& name);
    bool nextRoom();
    bool prevRoom();
    int getCurrentRoomIndex() const { return _currentRoomIndex; }
    int getTotalRooms() const { return _totalRooms; }
    std::string getCurrentRoomPath() const;
    std::string getCurrentRoomName() const { return _currentRoomName; }
    void setCurrentRoomName(const std::string& name) { _currentRoomName = name; }
    const std::vector<ArenaTrigger>& getTriggers() const { return _triggers; }
    std::vector<ArenaTrigger>& getTriggers() { return _triggers; }
    void addTrigger(const ArenaTrigger& tr) { _triggers.push_back(tr); }
    void clearTriggers() { _triggers.clear(); }
    void setTriggers(const std::vector<ArenaTrigger>& trs) { _triggers = trs; }
    bool removeTriggerAtTile(int tx, int ty);
    void updateTriggers(float dt, Player& player);
    void executeTriggerAction(const std::string& action, Player* player = nullptr, const ArenaTrigger* tr = nullptr);

    // Тип этажа / локации (Обычный этаж с окнами и небом vs Подвальный темный этаж)
    FloorType getFloorType() const { return _floorType; }
    void setFloorType(FloorType ft);
    bool isBasement() const { return _floorType == FloorType::BASEMENT; }

    // Окна и естественный дневной свет
    void scanWindows();
    const std::vector<WindowLight>& getWindows() const { return _windows; }
    std::vector<WindowLight>& getWindows() { return _windows; }
    void addWindow(int tx, int ty);
    void removeWindow(int tx, int ty);
    static void renderSmoothWindowBeam(sf::RenderTarget& target, float wx, float wy, float tileSize,
                                       float radius, float coneAngle, const sf::Color& color,
                                       float intensity, bool isLightmap,
                                       std::function<bool(float wx, float wy)> isSolidWorldFn,
                                       std::shared_ptr<sf::Texture>* outTextureCache = nullptr,
                                       sf::Vector2f* outPosCache = nullptr,
                                       sf::Vector2f* outScaleCache = nullptr,
                                       bool* inoutDirty = nullptr);

    // Лабораторное освещение и флуоресцентные лампы
    void scanLamps();
    const std::vector<LampLight>& getLamps() const { return _lamps; }
    std::vector<LampLight>& getLamps() { return _lamps; }
    LampLight* findLamp(int tx, int ty);
    const LampLight* findLamp(int tx, int ty) const;
    void addLamp(int tx, int ty, const std::string& tag = "", bool flickering = false);
    void removeLamp(int tx, int ty);
    void setLampState(const std::string& tag, bool enabled);
    void toggleLamp(const std::string& tag);
    void setLampFlicker(const std::string& tag, bool flicker);
    void setAmbientColor(sf::Color col) { _ambientColor = col; }
    sf::Color getAmbientColor() const { return _ambientColor; }
    bool isLightingEnabled() const { return _lightingEnabled; }
    void setLightingEnabled(bool enabled) { _lightingEnabled = enabled; }
    void renderLighting(sf::RenderWindow& window, const sf::View& camera, sf::Vector2f playerPos, bool playerArmed = false);

    // Боссфайт и управление камерой
    void startBossFight(const std::string& bossName = "БОСС 1");
    void endBossFight();
    bool isBossFightActive() const { return _isBossFightActive; }
    const std::string& getActiveBossName() const { return _activeBossName; }
    bool isCameraLocked() const { return _isCameraLocked; }
    sf::Vector2f getCameraLockPosition() const { return _cameraLockPos; }
    bool isCameraLockOnBoss() const { return _cameraLockOnBoss; }
    int getCameraLockMode() const { return _cameraLockMode; }
    int getCameraTargetEnemyIndex() const { return _cameraTargetEnemyIndex; }
    const std::string& getCameraTargetEnemyTag() const { return _cameraTargetEnemyTag; }
    sf::Vector2f getCameraSize() const { return _cameraSize; }
    bool hasCustomCameraSize() const { return _hasCustomCameraSize; }
    void setCameraLock(sf::Vector2f pos, bool onBoss = false) { 
        _isCameraLocked = true; 
        _cameraLockPos = pos; 
        _cameraLockOnBoss = onBoss; 
        _cameraLockMode = onBoss ? 3 : 1; 
    }
    void setCameraLockMode(int mode, sf::Vector2f pos, int enIdx = -1, const std::string& enTag = "") {
        _isCameraLocked = (mode > 0);
        _cameraLockMode = mode;
        _cameraLockPos = pos;
        _cameraTargetEnemyIndex = enIdx;
        _cameraTargetEnemyTag = enTag;
        _cameraLockOnBoss = (mode == 3);
    }
    void unlockCamera() { _isCameraLocked = false; _cameraLockMode = 0; _hasCustomCameraSize = false; }
    void resetCameraLock() {
        _isCameraLocked = false;
        _cameraLockMode = 0;
        _hasCustomCameraSize = false;
        _cameraSize = {1280.0f, 720.0f};
        _cameraLockPos = {0.0f, 0.0f};
        _isBossFightActive = false;
        _activeBossName = "";
        _cameraLockOnBoss = false;
        _cameraTargetEnemyIndex = -1;
        _cameraTargetEnemyTag = "";
        if (!_activeBossSpawnedBlocks.empty()) {
            for (const auto& sb : _activeBossSpawnedBlocks) {
                int bx = std::get<0>(sb);
                int by = std::get<1>(sb);
                int origTile = std::get<2>(sb);
                if (by >= 0 && by < _height && bx >= 0 && bx < _width) {
                    _map[by][bx] = origTile;
                }
            }
            _activeBossSpawnedBlocks.clear();
            _tilemapDirty = true;
        }
    }

    const std::vector<EnemySpawnConfig>& getPendingEnemySpawns() const { return _pendingEnemySpawns; }
    bool hasPendingEnemySpawns() const { return !_pendingEnemySpawns.empty(); }
    std::vector<EnemySpawnConfig> consumePendingEnemySpawns() {
        auto sp = std::move(_pendingEnemySpawns);
        _pendingEnemySpawns.clear();
        return sp;
    }
    void queueEnemySpawn(const EnemySpawnConfig& sp) { _pendingEnemySpawns.push_back(sp); }

    // Кинематографические сообщения / уведомления лаборатории
    void showNotification(const std::string& text, float duration = 3.0f, sf::Color col = sf::Color(100, 255, 180));
    std::string getActiveNotification() const { return _notificationText; }
    float getNotificationTimer() const { return _notificationTimer; }
    void drawNotificationUI(sf::RenderWindow& window, const sf::Font* font);

    const std::vector<std::vector<int>>& getMap() const { return _map; }
    std::vector<std::vector<int>>& getMap() { return _map; }
    const std::vector<std::vector<int>>& getBackgroundMap() const { return _bgMap; }
    std::vector<std::vector<int>>& getBackgroundMap() { return _bgMap; }
    void setBackgroundMap(const std::vector<std::vector<int>>& bgMap) { _bgMap = bgMap; _tilemapDirty = true; }
    const std::vector<std::vector<int>>& getDecorMap() const { return _decorMap; }
    std::vector<std::vector<int>>& getDecorMap() { return _decorMap; }
    void setDecorMap(const std::vector<std::vector<int>>& decorMap) { _decorMap = decorMap; _tilemapDirty = true; }
    void setBackgroundTile(int x, int y, int code) {
        if (y >= 0 && y < _height && x >= 0 && x < _width) {
            if (_bgMap.size() != static_cast<size_t>(_height)) _bgMap.assign(_height, std::vector<int>(_width, 0));
            _bgMap[y][x] = code;
            _tilemapDirty = true;
        }
    }
    void setDecorTile(int x, int y, int code) {
        if (y >= 0 && y < _height && x >= 0 && x < _width) {
            if (_decorMap.size() != static_cast<size_t>(_height)) _decorMap.assign(_height, std::vector<int>(_width, 0));
            _decorMap[y][x] = code;
            _tilemapDirty = true;
        }
    }

    void draw(sf::RenderWindow& window);
    void markTilemapDirty() { 
        _tilemapDirty = true; 
        for (auto& l : _lamps) l.dirty = true;
        for (auto& w : _windows) w.dirty = true;
    }

private:
    int _width;
    int _height;
    int _floorY;
    float _tileSize = TILE_SIZE;
    std::vector<std::vector<int>> _map;
    std::vector<std::vector<int>> _bgMap;
    std::vector<std::vector<int>> _decorMap;
    sf::Texture _tileTexture;
    std::vector<sf::Sprite> _tileSprites;
    std::vector<bool> _tileSolidFlags;
    std::unordered_map<int, TileShape> _tileShapes;
    sf::VertexArray _bgVertices{sf::PrimitiveType::Triangles};
    sf::VertexArray _decorVertices{sf::PrimitiveType::Triangles};
    sf::VertexArray _tilemapVertices{sf::PrimitiveType::Triangles};
    bool _tilemapDirty = true;
    void rebuildTilemapVertices();
    sf::Vector2f _spawnPosition;
    sf::Vector2f _enemySpawnPosition;
    std::vector<EnemySpawnConfig> _enemySpawns;
    std::vector<sf::Vector2f> _enemySpawnPositions;

    int _currentRoomIndex = 1;
    int _totalRooms = 1;
    std::string _currentRoomName = "room_1";
    std::vector<DoorBlock> _doors;
    std::vector<InteractiveDoor> _interactiveDoors;
    std::vector<DoorSplinterParticle> _doorSplinters;
    sf::Texture _texDoorClosed;
    sf::Texture _texDoorOpen;
    sf::Texture _texDoorBroken;
    bool _texDoorsLoaded = false;
    std::vector<ArenaTrigger> _triggers;
    sf::Texture _doorTexture;
    bool _doorTextureLoaded = false;
    sf::Texture _jumpPadTexture;
    bool _jumpPadTextureLoaded = false;
    sf::Texture _spikesTexture;
    bool _spikesTextureLoaded = false;
    sf::Font _doorFont;
    bool _doorFontLoaded = false;
    mutable std::optional<sf::Text> _cachedDoorText;

    std::string _configPath = "assets/config/map.kv";
    std::string _layoutFilePath = "assets/config/map.txt";
    std::string _tilesetPath = "assets/sprites/BlocksTest.png";

    static Arena* s_activeArena;
    std::vector<BloodDecal> _bloodDecals;
    std::vector<BloodParticle> _bloodParticles;
    std::vector<BackgroundBloodStreak> _bgBloodStreaks;

    // Динамическое лабораторное освещение
    FloorType _floorType = FloorType::NORMAL;
    std::vector<WindowLight> _windows;
    std::vector<LampLight> _lamps;
    sf::Color _ambientColor{26, 30, 40}; // Темный лабораторный полумрак
    bool _lightingEnabled = true;
    mutable sf::RenderTexture _lightmap;
    bool _lightmapInitialized = false;
    sf::Texture _lampConeTexture;
    bool _lampConeTextureInitialized = false;
    void ensureLampConeTextureInitialized();
    mutable sf::VertexArray _coneVA{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray _lampHaloVA{sf::PrimitiveType::TriangleFan};
    mutable sf::VertexArray _volBeamsVA{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray _bgBloodVA{sf::PrimitiveType::Triangles};
    mutable sf::VertexArray _fgBloodVA{sf::PrimitiveType::Triangles};

    // Всплывающие уведомления / лог лаборатории
    std::string _notificationText = "";
    float _notificationTimer = 0.0f;
    float _notificationDuration = 3.0f;
    sf::Color _notificationColor{100, 255, 180};

    // Модульные фазы освещения (SRP)
    void renderLightingLightmapPass(const sf::View& camera);
    void renderLightingMultiplyPass(sf::RenderWindow& window, const sf::View& camera);
    void renderLightingHalos(sf::RenderWindow& window);
    void renderLightingAtmosphere(sf::RenderWindow& window);

    mutable sf::RectangleShape _cachedRoomBg;

    // Состояние боссфайта и камеры
    bool _isBossFightActive = false;
    std::string _activeBossName = "БОСС 1";
    bool _isCameraLocked = false;
    int _cameraLockMode = 0; // 0: None, 1: Point/Block, 2: Enemy, 3: Boss
    sf::Vector2f _cameraLockPos{0.0f, 0.0f};
    bool _cameraLockOnBoss = false;
    int _cameraTargetEnemyIndex = -1;
    std::string _cameraTargetEnemyTag = "";
    sf::Vector2f _cameraSize{1280.0f, 720.0f};
    bool _hasCustomCameraSize = false;
    std::vector<EnemySpawnConfig> _pendingEnemySpawns;
    std::vector<std::tuple<int, int, int>> _activeBossSpawnedBlocks;
    sf::Vector2f _prevPlayerPos{0.0f, 0.0f};

    void loadTileset(const std::string& texturePath = "assets/sprites/BlocksTest.png");
    void buildDefaultMap();
    bool parseKVConfig(const std::string& filepath);
    bool applyAsciiGrid(const std::vector<std::string>& rows);
    void loadKVOverlays(const std::string& kvPath);
    void saveDefaultFilesIfMissing();
};

