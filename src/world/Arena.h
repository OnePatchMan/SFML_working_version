#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>
#include "core/Common.h"

struct DoorBlock {
    int x = 0;
    int y = 0;
    int targetRoom = 1;
};

struct EnemySpawnConfig {
    sf::Vector2f position;
    int type = 0; // 0 = NORMAL, 1 = TANK, 2 = DUELIST
};

enum class BloodDecalOrientation {
    HORIZONTAL, // Прямая горизонтальная полоса на верхней/нижней грани блока (толщина 2-3px)
    VERTICAL    // Прямая вертикальная полоса на левой/правой грани блока (толщина 2-3px)
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

    void spawnFinisherBlood(sf::Vector2f pos, sf::Vector2f burstDir, int count = 65, float maxHp = 45.0f);
    void spawnKillBlood(sf::Vector2f pos, sf::Vector2f knockDir = {0.0f, -1.0f}, float maxHp = 45.0f);
    void update(float dt);
    void clearBlood() { _bloodDecals.clear(); _bloodParticles.clear(); _bgBloodStreaks.clear(); }
    const std::vector<BloodDecal>& getBloodDecals() const { return _bloodDecals; }
    size_t getBloodDecalCount() const { return _bloodDecals.size(); }
    const std::vector<BloodParticle>& getBloodParticles() const { return _bloodParticles; }
    const std::vector<BackgroundBloodStreak>& getBackgroundBloodStreaks() const { return _bgBloodStreaks; }
    size_t getBackgroundBloodStreakCount() const { return _bgBloodStreaks.size(); }

    bool isSolid(int tx, int ty) const;
    bool isBlockSolid(int tx, int ty) const;
    bool raycastSolid(sf::Vector2f start, sf::Vector2f target, sf::Vector2f& hitPoint, sf::Vector2i& hitTile, sf::Vector2f& hitNormal) const;
    bool isDoor(int tx, int ty) const;
    int getDoorTargetRoom(int tx, int ty) const;
    void setDoor(int tx, int ty, int targetRoom);
    void removeDoor(int tx, int ty);
    void clearDoors() { _doors.clear(); }
    const std::vector<DoorBlock>& getDoors() const { return _doors; }
    bool overlapsSolid(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    bool hasLineOfSight(sf::Vector2f from, sf::Vector2f to) const;
    bool checkPlayerDoorOverlap(float px, float py, float halfW, float topOffset, float bottomOffset) const;
    std::optional<int> getOverlappingDoorTargetRoom(float px, float py, float halfW, float topOffset, float bottomOffset) const;

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
    bool nextRoom();
    bool prevRoom();
    int getCurrentRoomIndex() const { return _currentRoomIndex; }
    int getTotalRooms() const { return _totalRooms; }
    std::string getCurrentRoomPath() const;

    void draw(sf::RenderWindow& window);
    void markTilemapDirty() { _tilemapDirty = true; }

private:
    int _width;
    int _height;
    int _floorY;
    float _tileSize = TILE_SIZE;
    std::vector<std::vector<int>> _map;
    sf::Texture _tileTexture;
    std::vector<sf::Sprite> _tileSprites;
    sf::VertexArray _tilemapVertices{sf::PrimitiveType::Triangles};
    bool _tilemapDirty = true;
    void rebuildTilemapVertices();
    sf::Vector2f _spawnPosition;
    sf::Vector2f _enemySpawnPosition;
    std::vector<EnemySpawnConfig> _enemySpawns;
    std::vector<sf::Vector2f> _enemySpawnPositions;

    int _currentRoomIndex = 1;
    int _totalRooms = 10;
    std::vector<DoorBlock> _doors;
    sf::Texture _doorTexture;
    bool _doorTextureLoaded = false;
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

    void loadTileset(const std::string& texturePath = "assets/sprites/BlocksTest.png");
    void buildDefaultMap();
    bool parseKVConfig(const std::string& filepath);
    bool applyAsciiGrid(const std::vector<std::string>& rows);
    void saveDefaultFilesIfMissing();
};

