#include "world/MapEditor.h"
#include "world/MapEditorInternal.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>

using namespace map_editor_internal;

MapEditor::MapEditor()
    : _active(false), _isPlaytesting(false), _tool(EditorTool::PLATFORM), _brushSize(1), 
      _doorTargetRoom(2), _viewScale(1.0f), _cameraPos(0.0f, 0.0f), _camSpeed(1200.0f), 
      _isPanning(false), _showGrid(true), _showJumpRuler(false), _showScreenFrame(false)
{
    scanTileSprites();
}

void MapEditor::scanTileSprites() {
    _paletteSprites.clear();

    // 1. Базовые инструменты
    {
        IngameTileSpriteItem item;
        item.name = "Платформа";
        item.filename = "01_platform.png";
        item.tool = EditorTool::PLATFORM;
        item.tileCode = 1;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Стена";
        item.filename = "02_wall.png";
        item.tool = EditorTool::WALL;
        item.tileCode = 2;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Дверь";
        item.filename = "03_door.png";
        item.tool = EditorTool::DOOR;
        item.tileCode = 3;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Лампа";
        item.filename = "04_lamp.png";
        item.tool = EditorTool::LAMP;
        item.tileCode = 4;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Ящик (Wood)";
        item.filename = "06_wood.png";
        item.tool = EditorTool::WALL;
        item.tileCode = 5;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Колонна";
        item.filename = "04_pillar.png";
        item.tool = EditorTool::WALL;
        item.tileCode = 6;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Лава (Hazard)";
        item.filename = "05_hazard.png";
        item.tool = EditorTool::WALL;
        item.tileCode = 7;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Игрок (P)";
        item.filename = "player_spawn.png";
        item.tool = EditorTool::PLAYER_SPAWN;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Обычный";
        item.filename = "enemy_normal.png";
        item.tool = EditorTool::ENEMY_SPAWN;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Жирный";
        item.filename = "enemy_tank.png";
        item.tool = EditorTool::ENEMY_TANK;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Дуэлянт (Агр)";
        item.filename = "enemy_duelist.png";
        item.tool = EditorTool::ENEMY_DUELIST;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Дуэлянт (Пасс)";
        item.filename = "enemy_duelist_passive.png";
        item.tool = EditorTool::ENEMY_DUELIST_PASSIVE;
        _paletteSprites.push_back(std::move(item));
    }

    // 2. Сканирование папок тайлов
    std::string tilesDir = resolveAssetPath("assets/tiles");
    std::string blocksDir = resolveAssetPath("assets/tiles/blocks");

    // Подгружаем текстуры для базовых инструментов
    for (auto& baseItem : _paletteSprites) {
        std::string p = tilesDir + "/" + baseItem.filename;
        if (!std::filesystem::exists(p) && !blocksDir.empty()) {
            p = blocksDir + "/" + baseItem.filename;
        }
        if (std::filesystem::exists(p)) {
            auto tex = std::make_shared<sf::Texture>();
            if (tex->loadFromFile(p)) {
                baseItem.texture = tex;
                baseItem.sprite = std::make_unique<sf::Sprite>(*baseItem.texture);
                float sx = 32.0f / static_cast<float>(tex->getSize().x);
                float sy = 32.0f / static_cast<float>(tex->getSize().y);
                baseItem.sprite->setScale({sx, sy});
            }
        }
    }

    // Автоматическое разделение по 32px по оси X спрайтшитов с блоками из assets/tiles/blocks/
    if (std::filesystem::exists(blocksDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(blocksDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                std::string fname = entry.path().filename().string();
                std::string stem = entry.path().stem().string();

                auto tex = std::make_shared<sf::Texture>();
                if (tex->loadFromFile(entry.path().string())) {
                    unsigned int sheetW = tex->getSize().x;
                    unsigned int sheetH = tex->getSize().y;
                    int numSlices = static_cast<int>(sheetW / 32);
                    if (numSlices <= 0) numSlices = 1;

                    for (int s = 0; s < numSlices; ++s) {
                        IngameTileSpriteItem item;
                        item.texture = tex;
                        item.filename = fname;
                        item.name = stem + (numSlices > 1 ? (" #" + std::to_string(s + 1)) : "");
                        item.tool = (stem.find("BlocksTest") != std::string::npos) ? (s == 1 ? EditorTool::PLATFORM : EditorTool::WALL) : EditorTool::WALL;

                        item.sprite = std::make_unique<sf::Sprite>(*tex);
                        item.sprite->setTextureRect(sf::IntRect({s * 32, 0}, {32, static_cast<int>(std::min(32u, sheetH))}));
                        item.sprite->setScale({1.0f, 1.0f});
                        _paletteSprites.push_back(std::move(item));
                    }
                }
            }
        }
    }

    // Декоративные элементы из assets/tiles/decor/ и assets/tiles/
    std::vector<std::string> decorDirs = { resolveAssetPath("assets/tiles/decor"), tilesDir };
    for (const auto& dDir : decorDirs) {
        if (!std::filesystem::exists(dDir)) continue;
        for (const auto& entry : std::filesystem::directory_iterator(dDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                std::string stem = entry.path().stem().string();
                std::string fname = entry.path().filename().string();

                bool isBase = false;
                for (const auto& bi : _paletteSprites) {
                    if (bi.filename == fname) { isBase = true; break; }
                }
                if (isBase) continue;

                IngameTileSpriteItem item;
                item.filename = fname;
                item.name = stem;
                item.tool = EditorTool::WALL;

                if (stem.find("hazard") != std::string::npos || stem.find("lava") != std::string::npos) {
                    item.name = "Лава";
                } else if (stem.find("wood") != std::string::npos || stem.find("crate") != std::string::npos) {
                    item.name = "Ящик";
                } else if (stem.find("pillar") != std::string::npos) {
                    item.name = "Колонна";
                }

                auto tex = std::make_shared<sf::Texture>();
                if (tex->loadFromFile(entry.path().string())) {
                    item.texture = tex;
                    item.sprite = std::make_unique<sf::Sprite>(*item.texture);
                    float sx = 32.0f / static_cast<float>(tex->getSize().x);
                    float sy = 32.0f / static_cast<float>(tex->getSize().y);
                    item.sprite->setScale({sx, sy});
                    _paletteSprites.push_back(std::move(item));
                }
            }
        }
    }
}

void MapEditor::toggle(sf::RenderWindow& window, sf::View& camera, const Player& player) {
    if (_isPlaytesting) {
        stopPlaytest(window, camera);
        return;
    }

    _active = !_active;
    if (_active) {
        window.setMouseCursorVisible(true);
        _baseCameraSize = sf::Vector2f(static_cast<float>(window.getSize().x),
                                       static_cast<float>(window.getSize().y));
        _viewScale = 1.0f;
        camera.setSize(_baseCameraSize);
        _cameraPos = player.position;
        camera.setCenter(_cameraPos);

        _statusMessage = "Режим редактора комнат [1..10]! Tab/P - Тест геймплея, Ctrl+Z/Y - Отмена/Повтор, Shift - Box Fill, J - Линейка";
        _statusTimer = 5.0f;
    } else {
        window.setMouseCursorVisible(false);
        resetZoom(camera);
        camera.setCenter(player.position);
    }
}

void MapEditor::cycleBrushSize() {
    if (_brushSize == 1) _brushSize = 2;
    else if (_brushSize == 2) _brushSize = 3;
    else if (_brushSize == 3) _brushSize = 5;
    else _brushSize = 1;

    _statusMessage = "Размер кисти: " + std::to_string(_brushSize) + "x" + std::to_string(_brushSize);
    _statusTimer = 2.0f;
}

void MapEditor::zoomIn(sf::View& camera) {
    _viewScale = std::max(0.4f, _viewScale - 0.2f);
    camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    _statusMessage = "Масштаб камеры: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "%";
    _statusTimer = 1.5f;
}

void MapEditor::zoomOut(sf::View& camera) {
    _viewScale = std::min(3.5f, _viewScale + 0.25f);
    camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    _statusMessage = "Масштаб камеры: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "%";
    _statusTimer = 1.5f;
}

void MapEditor::resetZoom(sf::View& camera) {
    _viewScale = 1.0f;
    camera.setSize(_baseCameraSize);
    _statusMessage = "Масштаб сброшен на 100%";
    _statusTimer = 1.5f;
}

void MapEditor::nextRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera) {
    switchRoom(arena.getCurrentRoomIndex() >= arena.getTotalRooms() ? 1 : arena.getCurrentRoomIndex() + 1, arena, player, enemy, camera);
}

void MapEditor::prevRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera) {
    switchRoom(arena.getCurrentRoomIndex() <= 1 ? arena.getTotalRooms() : arena.getCurrentRoomIndex() - 1, arena, player, enemy, camera);
}

void MapEditor::switchRoom(int targetRoom, Arena& arena, Player& player, Enemy& enemy, sf::View& camera) {
    if (targetRoom < 1 || targetRoom > arena.getTotalRooms()) return;
    saveCurrentRoom(arena);
    arena.loadRoom(targetRoom);
    player.reset(arena.getSpawnPosition());
    enemy.spawnPosition = arena.getEnemySpawnPosition();
    enemy.reset();
    _cameraPos = player.position;
    camera.setCenter(_cameraPos);

    _statusMessage = "Загружена Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                     " / " + std::to_string(arena.getTotalRooms()) + " (" + arena.getCurrentRoomPath() + ")";
    _statusTimer = 3.5f;
}

EditorSnapshot MapEditor::createSnapshot(const Arena& arena) const {
    EditorSnapshot snap;
    snap.width = arena.getWidth();
    snap.height = arena.getHeight();
    snap.tiles.resize(snap.width * snap.height);
    for (int y = 0; y < snap.height; ++y) {
        for (int x = 0; x < snap.width; ++x) {
            snap.tiles[y * snap.width + x] = arena.getTile(x, y);
        }
    }
    snap.doors = arena.getDoors();
    snap.enemySpawns = arena.getEnemySpawns();
    snap.playerSpawn = arena.getSpawnPosition();
    return snap;
}

void MapEditor::restoreSnapshot(const EditorSnapshot& snap, Arena& arena, Player& player) {
    arena.restoreFromSnapshot(snap.width, snap.height, snap.tiles, snap.doors, snap.enemySpawns, snap.playerSpawn);
    player.position = snap.playerSpawn;
}

void MapEditor::pushUndo(const Arena& arena) {
    if (_undoStack.size() >= MAX_UNDO_STEPS) {
        _undoStack.pop_front();
    }
    _undoStack.push_back(createSnapshot(arena));
    _redoStack.clear();
}

bool MapEditor::undo(Arena& arena, Player& player) {
    if (_undoStack.empty()) {
        _statusMessage = "Стек отмены пуст!";
        _statusTimer = 1.5f;
        return false;
    }
    _redoStack.push_back(createSnapshot(arena));
    EditorSnapshot snap = _undoStack.back();
    _undoStack.pop_back();
    restoreSnapshot(snap, arena, player);
    _statusMessage = "Отмена действия (Undo: Ctrl+Z)! Шагов: " + std::to_string(_undoStack.size());
    _statusTimer = 2.0f;
    return true;
}

bool MapEditor::redo(Arena& arena, Player& player) {
    if (_redoStack.empty()) {
        _statusMessage = "Стек возврата пуст!";
        _statusTimer = 1.5f;
        return false;
    }
    _undoStack.push_back(createSnapshot(arena));
    EditorSnapshot snap = _redoStack.back();
    _redoStack.pop_back();
    restoreSnapshot(snap, arena, player);
    _statusMessage = "Повтор действия (Redo: Ctrl+Y)! Шагов: " + std::to_string(_redoStack.size());
    _statusTimer = 2.0f;
    return true;
}

void MapEditor::floodFill(int startX, int startY, Arena& arena, int fillTile) {
    if (startX < 0 || startX >= arena.getWidth() || startY < 0 || startY >= arena.getHeight()) return;
    int targetTile = arena.getTile(startX, startY);
    if (targetTile == fillTile) return;

    pushUndo(arena);
    int w = arena.getWidth();
    int h = arena.getHeight();
    std::vector<bool> visited(w * h, false);
    std::deque<sf::Vector2i> q;
    q.push_back({startX, startY});
    visited[startY * w + startX] = true;

    int filledCount = 0;
    while (!q.empty()) {
        sf::Vector2i p = q.front();
        q.pop_front();
        arena.setTile(p.x, p.y, fillTile);
        if (fillTile != 3) arena.removeDoor(p.x, p.y);
        if (fillTile != 0) arena.removeEnemySpawnAtTile(p.x, p.y);
        filledCount++;

        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int idx = ny * w + nx;
                if (!visited[idx] && arena.getTile(nx, ny) == targetTile) {
                    visited[idx] = true;
                    q.push_back({nx, ny});
                }
            }
        }
    }
    arena.markTilemapDirty();
    _statusMessage = "Заливка Bucket: изменено " + std::to_string(filledCount) + " тайлов!";
    _statusTimer = 2.0f;
}

bool MapEditor::eyedropAt(int tx, int ty, Arena& arena) {
    if (tx < 0 || tx >= arena.getWidth() || ty < 0 || ty >= arena.getHeight()) return false;
    float ts = arena.getTileSize();

    // 1. Проверяем дверь
    if (arena.isDoor(tx, ty)) {
        _tool = EditorTool::DOOR;
        _doorTargetRoom = arena.getDoorTargetRoom(tx, ty);
        _statusMessage = "Пипетка: Дверь перехода -> Цель: Комната " + std::to_string(_doorTargetRoom);
        _statusTimer = 2.0f;
        return true;
    }

    // 2. Проверяем врагов
    const auto& spawns = arena.getEnemySpawns();
    for (const auto& sp : spawns) {
        int stx = static_cast<int>(std::floor(sp.position.x / ts));
        int sty = static_cast<int>(std::floor((sp.position.y + 28.0f) / ts) - 1.0f);
        if (stx == tx && sty == ty) {
            if (sp.type == 1) _tool = EditorTool::ENEMY_TANK;
            else if (sp.type == 2) _tool = EditorTool::ENEMY_DUELIST;
            else if (sp.type == 3) _tool = EditorTool::ENEMY_DUELIST_PASSIVE;
            else _tool = EditorTool::ENEMY_SPAWN;

            std::string tName = (sp.type == 1) ? "Жирный (Tank)" :
                                (sp.type == 2) ? "Дуэлянт Агрессивный" :
                                (sp.type == 3) ? "Дуэлянт Пассивный" : "Обычный (Normal)";
            _statusMessage = "Пипетка: Враг [" + tName + "]";
            _statusTimer = 2.0f;
            return true;
        }
    }

    // 3. Тайл
    int t = arena.getTile(tx, ty);
    if (t == 1) {
        _tool = EditorTool::PLATFORM;
        _statusMessage = "Пипетка: Платформа (=)";
    } else if (t == 2) {
        _tool = EditorTool::WALL;
        _statusMessage = "Пипетка: Стена / Монолитный блок (#)";
    } else {
        _tool = EditorTool::ERASER;
        _statusMessage = "Пипетка: Пустота / Ластик";
    }
    _statusTimer = 2.0f;
    return true;
}

void MapEditor::copySelection(const Arena& arena) {
    if (!_selectionActive) {
        if (_lastHoverTile.x < 0 || _lastHoverTile.y < 0) {
            _statusMessage = "Сначала выделите область: Зажмите Ctrl + ЛКМ Drag!";
            _statusTimer = 2.0f;
            return;
        }
        _selStart = {_lastHoverTile.x - 1, _lastHoverTile.y - 1};
        _selEnd   = {_lastHoverTile.x + 1, _lastHoverTile.y + 1};
    }

    int x1 = std::max(0, std::min(_selStart.x, _selEnd.x));
    int x2 = std::min(arena.getWidth() - 1, std::max(_selStart.x, _selEnd.x));
    int y1 = std::max(0, std::min(_selStart.y, _selEnd.y));
    int y2 = std::min(arena.getHeight() - 1, std::max(_selStart.y, _selEnd.y));

    _clipboard.w = x2 - x1 + 1;
    _clipboard.h = y2 - y1 + 1;
    _clipboard.items.clear();
    _clipboard.hasData = true;

    float ts = arena.getTileSize();
    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            ClipboardTile ct;
            ct.relX = x - x1;
            ct.relY = y - y1;
            ct.tile = arena.getTile(x, y);
            ct.doorTarget = arena.isDoor(x, y) ? arena.getDoorTargetRoom(x, y) : -1;
            ct.enemyType = -1;
            for (const auto& sp : arena.getEnemySpawns()) {
                int stx = static_cast<int>(std::floor(sp.position.x / ts));
                int sty = static_cast<int>(std::floor((sp.position.y + 28.0f) / ts) - 1.0f);
                if (stx == x && sty == y) {
                    ct.enemyType = sp.type;
                    break;
                }
            }
            _clipboard.items.push_back(ct);
        }
    }
    _selectionActive = false;
    _statusMessage = "Скопировано: " + std::to_string(_clipboard.w) + "x" + std::to_string(_clipboard.h) + " тайлов (Ctrl+V для вставки)";
    _statusTimer = 2.5f;
}

void MapEditor::pasteClipboard(int destX, int destY, Arena& arena, Player& player) {
    if (!_clipboard.hasData || _clipboard.items.empty()) return;
    pushUndo(arena);

    for (const auto& item : _clipboard.items) {
        int tx = destX + item.relX;
        int ty = destY + item.relY;
        if (tx >= 0 && tx < arena.getWidth() && ty >= 0 && ty < arena.getHeight()) {
            arena.setTile(tx, ty, item.tile);
            if (item.doorTarget > 0) {
                arena.setDoor(tx, ty, item.doorTarget);
            } else {
                arena.removeDoor(tx, ty);
            }
            if (item.enemyType >= 0) {
                arena.setEnemySpawnAt(tx, ty, item.enemyType);
            } else {
                arena.removeEnemySpawnAtTile(tx, ty);
            }
        }
    }
    arena.markTilemapDirty();
    _pasteModeActive = false;
    _statusMessage = "Фрагмент карты успешно вставлен!";
    _statusTimer = 2.0f;
}

bool MapEditor::duplicateRoom(int targetRoom, Arena& arena) {
    if (targetRoom < 1 || targetRoom > arena.getTotalRooms() || targetRoom == arena.getCurrentRoomIndex()) return false;
    saveCurrentRoom(arena);
    bool ok = arena.duplicateRoom(targetRoom);
    if (ok) {
        _statusMessage = "Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                         " скопирована в Комнату " + std::to_string(targetRoom) + "!";
    } else {
        _statusMessage = "Ошибка дублирования комнаты!";
    }
    _statusTimer = 3.0f;
    return ok;
}

void MapEditor::clearRoom(Arena& arena, Player& player) {
    pushUndo(arena);
    arena.clearRoom(0);
    player.reset(arena.getSpawnPosition());
    _statusMessage = "Комната очищена! (Ctrl+Z для отмены)";
    _statusTimer = 3.0f;
}

void MapEditor::startPlaytest(Arena& arena, Player& player, sf::RenderWindow& window, sf::View& camera) {
    _savedEditorCamPos = _cameraPos;
    _savedViewScale = _viewScale;

    float ts = arena.getTileSize();
    if (_lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < arena.getHeight()) {
        player.position = { static_cast<float>(_lastHoverTile.x * ts + ts / 2.0f),
                            static_cast<float>((_lastHoverTile.y + 1) * ts - 30.0f) };
        player.velocity = {0.0f, 0.0f};
    }

    if (_respawnCallback) {
        _respawnCallback();
    }

    _isPlaytesting = true;
    camera.setSize(_baseCameraSize);
    camera.setCenter(player.position);
    window.setMouseCursorVisible(false);

    _statusMessage = "ТЕСТ ГЕЙМПЛЕЯ АКТИВЕН! [Tab] или [F6] - Вернуться в редактор";
    _statusTimer = 4.0f;
}

void MapEditor::stopPlaytest(sf::RenderWindow& window, sf::View& camera) {
    _isPlaytesting = false;
    _cameraPos = _savedEditorCamPos;
    _viewScale = _savedViewScale;
    camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    camera.setCenter(_cameraPos);
    window.setMouseCursorVisible(true);

    _statusMessage = "Возврат в редактор комнат.";
    _statusTimer = 2.0f;
}


void MapEditor::saveCurrentRoom(const Arena& arena) {
    bool ok = arena.saveCurrentRoom();
    if (ok) {
        _statusMessage = "Комната " + std::to_string(arena.getCurrentRoomIndex()) + " сохранена в " + arena.getCurrentRoomPath() + "!";
        std::cout << "[MAP EDITOR] " << _statusMessage << std::endl;
    } else {
        _statusMessage = "Ошибка сохранения комнаты!";
        std::cerr << "[MAP EDITOR] " << _statusMessage << std::endl;
    }
    _statusTimer = 3.5f;
}

