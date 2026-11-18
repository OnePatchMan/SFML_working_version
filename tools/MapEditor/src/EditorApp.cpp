#include "EditorApp.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>

namespace {
std::vector<sf::Vector2i> getEditorLineTiles(int x0, int y0, int x1, int y1) {
    std::vector<sf::Vector2i> tiles;
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    int curX = x0;
    int curY = y0;
    while (true) {
        tiles.push_back({curX, curY});
        if (curX == x1 && curY == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            curX += sx;
        }
        if (e2 < dx) {
            err += dx;
            curY += sy;
        }
    }
    return tiles;
}
}

EditorApp::EditorApp()
    : _window(sf::VideoMode::getDesktopMode(), "2D Fighting Game - Standalone Map Editor", sf::State::Fullscreen),
      _camera(sf::Vector2f(2500.0f, 700.0f), sf::Vector2f(static_cast<float>(sf::VideoMode::getDesktopMode().size.x), static_cast<float>(sf::VideoMode::getDesktopMode().size.y)))
{
    _window.setFramerateLimit(120);
    _baseCameraSize = _camera.getSize();
    _isFullscreen = true;

    std::string fontPath = resolveEditorFont();
    if (!fontPath.empty()) {
        _fontLoaded = _font.openFromFile(fontPath);
    }

    scanTileSprites();
    initDefaultRoom();
    loadRoom(1);
}

void EditorApp::toggleFullscreen() {
    _isFullscreen = !_isFullscreen;
    if (_isFullscreen) {
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        _window.create(desktop, "2D Fighting Game - Standalone Map Editor", sf::State::Fullscreen);
    } else {
        _window.create(sf::VideoMode({1280, 720}), "2D Fighting Game - Standalone Map Editor", sf::Style::Default, sf::State::Windowed);
    }
    _window.setFramerateLimit(120);
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);
    _camera.setSize({winW * _viewScale, winH * _viewScale});
    _baseCameraSize = {winW, winH};
    _statusMessage = _isFullscreen ? "Режим: Полноэкранный [F11 - Окно]" : "Режим: Оконный 1280x720 [F11 - Полный экран]";
    _statusTimer = 2.5f;
}

void EditorApp::scanTileSprites() {
    _paletteSprites.clear();

    // 1. ВКЛАДКА: 🧱 БЛОКИ И ГЕОМЕТРИЯ (BLOCKS)
    {
        TileSpriteItem item;
        item.name = "Платформа";
        item.filename = "01_platform.png";
        item.tileCode = 1;
        item.tool = ToolType::PLATFORM;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Стена";
        item.filename = "02_wall.png";
        item.tileCode = 2;
        item.tool = ToolType::WALL;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Заливка Box";
        item.filename = "box_fill.png";
        item.tileCode = 0;
        item.tool = ToolType::BOX_FILL;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Линия Line";
        item.filename = "line_tool.png";
        item.tileCode = 0;
        item.tool = ToolType::LINE;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Ведро Bucket";
        item.filename = "bucket_fill.png";
        item.tileCode = 0;
        item.tool = ToolType::FLOOD_FILL;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Ластик [E]";
        item.filename = "eraser.png";
        item.tileCode = 0;
        item.tool = ToolType::EDGE_PASSAGE;
        item.tab = PaletteTab::BLOCKS;
        _paletteSprites.push_back(std::move(item));
    }

    // Дополнительные декоративные блоки из assets/tiles/
    std::string tilesDir = getGameTilesDir();
    if (std::filesystem::exists(tilesDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(tilesDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") {
                std::string fname = entry.path().filename().string();
                std::string stem = entry.path().stem().string();

                bool isBase = false;
                for (const auto& bi : _paletteSprites) {
                    if (bi.filename == fname) { isBase = true; break; }
                }
                if (isBase) continue;

                TileSpriteItem item;
                item.filename = fname;
                item.name = stem;
                item.tool = ToolType::WALL;
                item.tab = PaletteTab::BLOCKS;

                if (stem.find("lava") != std::string::npos || stem.find("hazard") != std::string::npos) item.name = "Лава";
                else if (stem.find("wood") != std::string::npos || stem.find("crate") != std::string::npos) item.name = "Ящик";
                else if (stem.find("pillar") != std::string::npos) item.name = "Колонна";

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

    // 2. ВКЛАДКА: ⚔ СУЩНОСТИ И ВРАГИ (ENTITIES)
    {
        TileSpriteItem item;
        item.name = "Игрок (P)";
        item.filename = "player_spawn.png";
        item.tileCode = 0;
        item.tool = ToolType::PLAYER_SPAWN;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Обычный";
        item.filename = "enemy_normal.png";
        item.tileCode = 0;
        item.tool = ToolType::ENEMY_SPAWN;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Жирный";
        item.filename = "enemy_tank.png";
        item.tileCode = 0;
        item.tool = ToolType::ENEMY_TANK;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Дуэлянт Агр";
        item.filename = "enemy_duelist.png";
        item.tileCode = 0;
        item.tool = ToolType::ENEMY_DUELIST;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Дуэлянт Пасс";
        item.filename = "enemy_duelist_passive.png";
        item.tileCode = 0;
        item.tool = ToolType::ENEMY_DUELIST_PASSIVE;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Манекен (Dummy)";
        item.filename = "enemy_dummy.png";
        item.tileCode = 0;
        item.tool = ToolType::ENEMY_DUMMY;
        item.tab = PaletteTab::ENTITIES;
        _paletteSprites.push_back(std::move(item));
    }

    // 3. ВКЛАДКА: ⚡ МЕХАНИКИ И ТРИГГЕРЫ (MECHANICS)
    {
        TileSpriteItem item;
        item.name = "Дверь (Door)";
        item.filename = "03_door.png";
        item.tileCode = 3;
        item.tool = ToolType::DOOR;
        item.tab = PaletteTab::MECHANICS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Тест Чекпоинт";
        item.filename = "test_spawn.png";
        item.tileCode = 0;
        item.tool = ToolType::TEST_SPAWN;
        item.tab = PaletteTab::MECHANICS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Батут (JumpPad)";
        item.filename = "jump_pad.png";
        item.tileCode = 0;
        item.tool = ToolType::JUMP_PAD;
        item.tab = PaletteTab::MECHANICS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Шипы (Spikes)";
        item.filename = "spikes.png";
        item.tileCode = 0;
        item.tool = ToolType::SPIKES;
        item.tab = PaletteTab::MECHANICS;
        _paletteSprites.push_back(std::move(item));
    }
    {
        TileSpriteItem item;
        item.name = "Триггер (Zone)";
        item.filename = "trigger_zone.png";
        item.tileCode = 0;
        item.tool = ToolType::TRIGGER_ZONE;
        item.tab = PaletteTab::MECHANICS;
        _paletteSprites.push_back(std::move(item));
    }

    // Подгружаем текстуры для всех базовых
    if (std::filesystem::exists(tilesDir)) {
        for (auto& item : _paletteSprites) {
            if (item.sprite) continue;
            std::string p = tilesDir + "/" + item.filename;
            if (std::filesystem::exists(p)) {
                auto tex = std::make_shared<sf::Texture>();
                if (tex->loadFromFile(p)) {
                    item.texture = tex;
                    item.sprite = std::make_unique<sf::Sprite>(*item.texture);
                    float sx = 32.0f / static_cast<float>(tex->getSize().x);
                    float sy = 32.0f / static_cast<float>(tex->getSize().y);
                    item.sprite->setScale({sx, sy});
                }
            }
        }
    }

    // Выделенная загрузка текстур для батута и шипов (из assets/sprites/ или assets/tiles/)
    if (!_texJumpPad) {
        _texJumpPad = std::make_shared<sf::Texture>();
        std::string p = resolveAssetPath("assets/sprites/jump_pad.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/jump_pad.png");
        _texJumpPad->loadFromFile(p);
    }
    if (!_texSpikes) {
        _texSpikes = std::make_shared<sf::Texture>();
        std::string p = resolveAssetPath("assets/sprites/spikes.png");
        if (!std::filesystem::exists(p)) p = resolveAssetPath("assets/tiles/spikes.png");
        _texSpikes->loadFromFile(p);
    }

    for (auto& item : _paletteSprites) {
        if (item.tool == ToolType::JUMP_PAD && _texJumpPad && _texJumpPad->getSize().x > 0) {
            item.texture = _texJumpPad;
            item.sprite = std::make_unique<sf::Sprite>(*item.texture);
            float sx = 32.0f / static_cast<float>(_texJumpPad->getSize().x);
            float sy = 32.0f / static_cast<float>(_texJumpPad->getSize().y);
            item.sprite->setScale({sx, sy});
        } else if (item.tool == ToolType::SPIKES && _texSpikes && _texSpikes->getSize().x > 0) {
            item.texture = _texSpikes;
            item.sprite = std::make_unique<sf::Sprite>(*item.texture);
            float sx = 32.0f / static_cast<float>(_texSpikes->getSize().x);
            float sy = 32.0f / static_cast<float>(_texSpikes->getSize().y);
            item.sprite->setScale({sx, sy});
        }
    }
}

void EditorApp::pushUndo() {
    if (_undoStack.size() >= MAX_UNDO) {
        _undoStack.pop_front();
    }
    StandaloneSnapshot snap;
    snap.width = _width;
    snap.height = _height;
    snap.tiles.resize(_width * _height);
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            snap.tiles[y * _width + x] = _map[y][x];
        }
    }
    snap.doors = _doors;
    snap.enemySpawns = _enemySpawns;
    snap.triggers = _triggers;
    snap.playerSpawn = _playerSpawn;
    snap.testSpawn = _testSpawn;
    _undoStack.push_back(std::move(snap));
    _redoStack.clear();
}

bool EditorApp::undo() {
    if (_undoStack.empty()) {
        _statusMessage = "Стек отмены пуст!";
        _statusTimer = 1.5f;
        return false;
    }

    StandaloneSnapshot cur;
    cur.width = _width;
    cur.height = _height;
    cur.tiles.resize(_width * _height);
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            cur.tiles[y * _width + x] = _map[y][x];
        }
    }
    cur.doors = _doors;
    cur.enemySpawns = _enemySpawns;
    cur.triggers = _triggers;
    cur.playerSpawn = _playerSpawn;
    cur.testSpawn = _testSpawn;
    _redoStack.push_back(std::move(cur));

    StandaloneSnapshot snap = _undoStack.back();
    _undoStack.pop_back();

    _width = snap.width;
    _height = snap.height;
    _map.assign(_height, std::vector<int>(_width, 0));
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            _map[y][x] = snap.tiles[y * _width + x];
        }
    }
    _doors = snap.doors;
    _enemySpawns = snap.enemySpawns;
    _triggers = snap.triggers;
    _playerSpawn = snap.playerSpawn;
    _testSpawn = snap.testSpawn;

    _statusMessage = "Отмена (Undo: Ctrl+Z)! Шагов: " + std::to_string(_undoStack.size());
    _statusTimer = 2.0f;
    return true;
}

bool EditorApp::redo() {
    if (_redoStack.empty()) {
        _statusMessage = "Стек возврата пуст!";
        _statusTimer = 1.5f;
        return false;
    }

    StandaloneSnapshot cur;
    cur.width = _width;
    cur.height = _height;
    cur.tiles.resize(_width * _height);
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            cur.tiles[y * _width + x] = _map[y][x];
        }
    }
    cur.doors = _doors;
    cur.enemySpawns = _enemySpawns;
    cur.triggers = _triggers;
    cur.playerSpawn = _playerSpawn;
    cur.testSpawn = _testSpawn;
    _undoStack.push_back(std::move(cur));

    StandaloneSnapshot snap = _redoStack.back();
    _redoStack.pop_back();

    _width = snap.width;
    _height = snap.height;
    _map.assign(_height, std::vector<int>(_width, 0));
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            _map[y][x] = snap.tiles[y * _width + x];
        }
    }
    _doors = snap.doors;
    _enemySpawns = snap.enemySpawns;
    _triggers = snap.triggers;
    _playerSpawn = snap.playerSpawn;
    _testSpawn = snap.testSpawn;

    _statusMessage = "Повтор (Redo: Ctrl+Y)! Шагов: " + std::to_string(_redoStack.size());
    _statusTimer = 2.0f;
    return true;
}

void EditorApp::floodFill(int startX, int startY, int fillTile) {
    if (startX < 0 || startX >= _width || startY < 0 || startY >= _height) return;
    int targetTile = _map[startY][startX];
    if (targetTile == fillTile) return;

    pushUndo();
    std::vector<bool> visited(_width * _height, false);
    std::deque<sf::Vector2i> q;
    q.push_back({startX, startY});
    visited[startY * _width + startX] = true;

    int count = 0;
    while (!q.empty()) {
        sf::Vector2i p = q.front();
        q.pop_front();
        _map[p.y][p.x] = fillTile;
        if (fillTile != 3) removeDoor(p.x, p.y);
        if (fillTile != 0) {
            removeEnemySpawn(p.x, p.y);
            removeTrigger(p.x, p.y);
        }
        count++;

        const int dx[4] = {1, -1, 0, 0};
        const int dy[4] = {0, 0, 1, -1};
        for (int i = 0; i < 4; ++i) {
            int nx = p.x + dx[i];
            int ny = p.y + dy[i];
            if (nx >= 0 && nx < _width && ny >= 0 && ny < _height) {
                int idx = ny * _width + nx;
                if (!visited[idx] && _map[ny][nx] == targetTile) {
                    visited[idx] = true;
                    q.push_back({nx, ny});
                }
            }
        }
    }
    _statusMessage = "Заливка Bucket: изменено " + std::to_string(count) + " тайлов!";
    _statusTimer = 2.0f;
}

bool EditorApp::eyedropAt(int tx, int ty) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return false;

    if (_testSpawn && _testSpawn->x == tx && _testSpawn->y == ty) {
        _currentTool = ToolType::TEST_SPAWN;
        _statusMessage = "Пипетка: Тестовый старт (Test Spawn)";
        _statusTimer = 2.0f;
        return true;
    }

    for (const auto& tr : _triggers) {
        if (tr.x == tx && tr.y == ty) {
            _currentTool = (tr.type == 0) ? ToolType::JUMP_PAD : (tr.type == 1) ? ToolType::SPIKES : ToolType::TRIGGER_ZONE;
            _statusMessage = "Пипетка: " + std::string(tr.type == 0 ? "Батут (JumpPad)" : tr.type == 1 ? "Шипы (Spikes)" : "Триггер (Zone)");
            _statusTimer = 2.0f;
            return true;
        }
    }

    for (const auto& d : _doors) {
        if (d.x == tx && d.y == ty) {
            _currentTool = ToolType::DOOR;
            _doorTargetRoom = d.targetRoom;
            _statusMessage = "Пипетка: Дверь -> Цель: Комната " + std::to_string(_doorTargetRoom);
            _statusTimer = 2.0f;
            return true;
        }
    }

    for (const auto& es : _enemySpawns) {
        if (es.x == tx && es.y == ty) {
            if (es.type == 1) _currentTool = ToolType::ENEMY_TANK;
            else if (es.type == 2) _currentTool = ToolType::ENEMY_DUELIST;
            else if (es.type == 3) _currentTool = ToolType::ENEMY_DUELIST_PASSIVE;
            else _currentTool = ToolType::ENEMY_SPAWN;

            std::string tName = (es.type == 1) ? "Танк" : (es.type == 2) ? "Дуэлянт Агр" : (es.type == 3) ? "Дуэлянт Пасс" : "Обычный";
            _statusMessage = "Пипетка: Враг [" + tName + "]";
            _statusTimer = 2.0f;
            return true;
        }
    }

    int t = _map[ty][tx];
    if (t == 1) {
        _currentTool = ToolType::PLATFORM;
        _statusMessage = "Пипетка: Платформа (=)";
    } else if (t == 2) {
        _currentTool = ToolType::WALL;
        _statusMessage = "Пипетка: Стена (#)";
    } else {
        _currentTool = ToolType::EDGE_PASSAGE;
        _statusMessage = "Пипетка: Пустота / Ластик";
    }
    _statusTimer = 2.0f;
    return true;
}

void EditorApp::copySelection() {
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
    int x2 = std::min(_width - 1, std::max(_selStart.x, _selEnd.x));
    int y1 = std::max(0, std::min(_selStart.y, _selEnd.y));
    int y2 = std::min(_height - 1, std::max(_selStart.y, _selEnd.y));

    _clipboard.w = x2 - x1 + 1;
    _clipboard.h = y2 - y1 + 1;
    _clipboard.items.clear();
    _clipboard.hasData = true;

    for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
            StandaloneClipboardTile ct;
            ct.relX = x - x1;
            ct.relY = y - y1;
            ct.tile = _map[y][x];
            ct.doorTarget = -1;
            for (const auto& d : _doors) {
                if (d.x == x && d.y == y) { ct.doorTarget = d.targetRoom; break; }
            }
            ct.enemyType = -1;
            for (const auto& es : _enemySpawns) {
                if (es.x == x && es.y == y) { ct.enemyType = es.type; break; }
            }
            _clipboard.items.push_back(ct);
        }
    }
    _selectionActive = false;
    _statusMessage = "Скопировано в буфер: " + std::to_string(_clipboard.w) + "x" + std::to_string(_clipboard.h) + " (Ctrl+V для вставки)";
    _statusTimer = 2.5f;
}

void EditorApp::pasteClipboard(int destX, int destY) {
    if (!_clipboard.hasData || _clipboard.items.empty()) return;
    pushUndo();

    for (const auto& item : _clipboard.items) {
        int tx = destX + item.relX;
        int ty = destY + item.relY;
        if (tx >= 0 && tx < _width && ty >= 0 && ty < _height) {
            _map[ty][tx] = item.tile;
            if (item.doorTarget > 0) setDoor(tx, ty, item.doorTarget);
            else removeDoor(tx, ty);
            if (item.enemyType >= 0) addEnemySpawn(tx, ty, item.enemyType);
            else removeEnemySpawn(tx, ty);
        }
    }
    _pasteModeActive = false;
    _statusMessage = "Фрагмент карты успешно вставлен!";
    _statusTimer = 2.0f;
}

bool EditorApp::duplicateRoom(int targetRoom) {
    if (targetRoom < 1 || targetRoom > _totalRooms || targetRoom == _currentRoomIndex) return false;
    saveToGame();
    std::string mapsDir = getGameMapsDir();
    std::string srcBase = mapsDir + "/room_" + std::to_string(_currentRoomIndex);
    std::string dstBase = mapsDir + "/room_" + std::to_string(targetRoom);

    try {
        if (std::filesystem::exists(srcBase + ".txt")) std::filesystem::copy_file(srcBase + ".txt", dstBase + ".txt", std::filesystem::copy_options::overwrite_existing);
        if (std::filesystem::exists(srcBase + ".kv"))  std::filesystem::copy_file(srcBase + ".kv", dstBase + ".kv", std::filesystem::copy_options::overwrite_existing);
        if (std::filesystem::exists(srcBase + ".png")) std::filesystem::copy_file(srcBase + ".png", dstBase + ".png", std::filesystem::copy_options::overwrite_existing);
        _statusMessage = "Комната " + std::to_string(_currentRoomIndex) + " скопирована в Комнату " + std::to_string(targetRoom) + "!";
        _statusTimer = 3.0f;
        return true;
    } catch (const std::exception& e) {
        _statusMessage = "Ошибка дублирования: " + std::string(e.what());
        _statusTimer = 3.0f;
        return false;
    }
}

void EditorApp::clearRoom() {
    pushUndo();
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            _map[y][x] = 0;
        }
    }
    _doors.clear();
    _enemySpawns.clear();
    _triggers.clear();
    _testSpawn.reset();
    _statusMessage = "Комната очищена! (Ctrl+Z для отмены)";
    _statusTimer = 3.0f;
}

void EditorApp::startSimulation(std::optional<sf::Vector2i> startTile) {
    saveToGame();
    _savedEditorCameraPos = _cameraPos;
    _savedEditorViewScale = _viewScale;

    _simArena = std::make_unique<Arena>(_width, _height);

    std::vector<int> flatTiles(_width * _height, 0);
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            flatTiles[y * _width + x] = _map[y][x];
        }
    }
    std::vector<DoorBlock> dBlocks;
    for (const auto& d : _doors) dBlocks.push_back({d.x, d.y, d.targetRoom});

    std::vector<EnemySpawnConfig> eConfigs;
    for (const auto& es : _enemySpawns) {
        sf::Vector2f pos(es.x * _tileSize + _tileSize * 0.5f, (es.y + 1) * _tileSize - 14.0f);
        eConfigs.push_back({pos, es.type});
    }

    sf::Vector2f pSpawnPos(_playerSpawn.x * _tileSize + _tileSize * 0.5f, (_playerSpawn.y + 1) * _tileSize - 30.0f);
    _simArena->restoreFromSnapshot(_width, _height, flatTiles, dBlocks, eConfigs, pSpawnPos);

    // Определяем точку старта
    sf::Vector2f spawnPos = pSpawnPos;
    if (startTile.has_value()) {
        spawnPos = { static_cast<float>(startTile->x * _tileSize + _tileSize * 0.5f),
                     static_cast<float>((startTile->y + 1) * _tileSize - 30.0f) };
    } else if (_testSpawn.has_value()) {
        spawnPos = { static_cast<float>(_testSpawn->x * _tileSize + _tileSize * 0.5f),
                     static_cast<float>((_testSpawn->y + 1) * _tileSize - 30.0f) };
    }

    _simPlayer = std::make_unique<Player>(spawnPos);
    _simCombat = std::make_unique<CombatSystem>();
    _simEnemies.clear();

    for (const auto& cfg : eConfigs) {
        EnemyType et = (cfg.type == 1) ? EnemyType::TANK :
                       (cfg.type == 2) ? EnemyType::DUELIST :
                       (cfg.type == 3) ? EnemyType::DUELIST_PASSIVE :
                       (cfg.type == 4) ? EnemyType::DUMMY : EnemyType::NORMAL;
        _simEnemies.emplace_back(cfg.position, et);
    }
    for (auto& e : _simEnemies) {
        e.setSmartAI(true);
        e.reset();
    }

    _viewScale = 1.0f;
    _camera.setSize(_baseCameraSize);
    _camera.setCenter(_simPlayer->position);
    _window.setMouseCursorVisible(false);

    _isSimulating = true;
    _statusMessage = "СИМУЛЯЦИЯ АКТИВНА! [Tab] или [Esc] — Вернуться в редактор";
    _statusTimer = 4.0f;
}

void EditorApp::stopSimulation() {
    _isSimulating = false;
    _cameraPos = _savedEditorCameraPos;
    _viewScale = _savedEditorViewScale;
    _camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    _camera.setCenter(_cameraPos);
    _window.setMouseCursorVisible(true);

    _simArena.reset();
    _simPlayer.reset();
    _simCombat.reset();
    _simEnemies.clear();

    _statusMessage = "Возврат в редактор комнат.";
    _statusTimer = 2.0f;
}

void EditorApp::initDefaultRoom() {
    _width = 80;
    _height = 20;
    _map.assign(_height, std::vector<int>(_width, 0));
    _doors.clear();
    _enemySpawns.clear();
    _triggers.clear();
    _testSpawn.reset();

    for (int x = 0; x < _width; ++x) {
        _map[_height - 1][x] = 2;
    }
    _playerSpawn = {6, _height - 3};
    _enemySpawns.push_back({15, _height - 3, 0, true});
}

void EditorApp::scanRooms() {
    _roomList.clear();
    std::string mapsDir = getGameMapsDir();
    if (!std::filesystem::exists(mapsDir)) {
        return;
    }

    std::set<std::string> roomNames;
    for (const auto& entry : std::filesystem::directory_iterator(mapsDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".txt" || ext == ".kv" || ext == ".png") {
                std::string stem = entry.path().stem().string();
                roomNames.insert(stem);
            }
        }
    }

    if (roomNames.empty()) {
        roomNames.insert("room_1");
    }

    std::vector<std::string> sortedNames(roomNames.begin(), roomNames.end());
    std::sort(sortedNames.begin(), sortedNames.end(), [](const std::string& a, const std::string& b) {
        bool aIsRoom = (a.rfind("room_", 0) == 0);
        bool bIsRoom = (b.rfind("room_", 0) == 0);
        if (aIsRoom && bIsRoom) {
            try {
                int numA = std::stoi(a.substr(5));
                int numB = std::stoi(b.substr(5));
                return numA < numB;
            } catch (...) {}
        }
        if (aIsRoom && !bIsRoom) return true;
        if (!aIsRoom && bIsRoom) return false;
        return a < b;
    });

    for (const auto& rName : sortedNames) {
        RoomMeta meta;
        meta.id = rName;
        meta.title = rName;
        if (rName.rfind("room_", 0) == 0) {
            meta.title = "Комната " + rName.substr(5) + " (" + rName + ")";
        }
        meta.path = mapsDir + "/" + rName;

        std::string pngPath = mapsDir + "/" + rName + ".png";
        if (std::filesystem::exists(pngPath)) {
            auto tex = std::make_shared<sf::Texture>();
            if (tex->loadFromFile(pngPath)) {
                meta.thumbnail = tex;
                meta.width = tex->getSize().x;
                meta.height = tex->getSize().y;
            }
        }
        _roomList.push_back(meta);
    }

    _totalRooms = static_cast<int>(_roomList.size());
    if (_roomBrowserSelected >= static_cast<int>(_roomList.size())) {
        _roomBrowserSelected = std::max(0, static_cast<int>(_roomList.size()) - 1);
    }
}

void EditorApp::createNamedRoom(const std::string& name) {
    std::string cleanName = name;
    cleanName.erase(std::remove_if(cleanName.begin(), cleanName.end(), [](char c) {
        return !(std::isalnum(c) || c == '_' || c == '-');
    }), cleanName.end());

    if (cleanName.empty()) {
        cleanName = "room_" + std::to_string(_roomList.size() + 1);
    }

    _currentRoomName = cleanName;
    if (cleanName.rfind("room_", 0) == 0) {
        try { _currentRoomIndex = std::stoi(cleanName.substr(5)); } catch (...) {}
    }

    initDefaultRoom();
    saveToGame();
    scanRooms();

    for (size_t i = 0; i < _roomList.size(); ++i) {
        if (_roomList[i].id == cleanName) {
            _roomBrowserSelected = static_cast<int>(i);
            break;
        }
    }

    _statusMessage = "Создана и открыта комната: " + cleanName + "!";
    _statusTimer = 3.5f;
}

bool EditorApp::loadRoomByName(const std::string& name) {
    _currentRoomName = name;
    if (name.rfind("room_", 0) == 0) {
        try { _currentRoomIndex = std::stoi(name.substr(5)); } catch (...) {}
    }

    std::string mapsDir = getGameMapsDir();
    std::string base = mapsDir + "/" + name;
    std::string filename = base + ".txt";
    if (!std::filesystem::exists(filename)) {
        filename = getGameConfigDir() + "/map.txt";
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        _statusMessage = "Создана новая комната: " + name;
        _statusTimer = 2.5f;
        initDefaultRoom();
        return false;
    }

    std::vector<std::string> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() >= 2 && line[0] == '/' && line[1] == '/') continue;
        if (!line.empty()) rows.push_back(line);
    }

    if (rows.empty()) return false;

    _height = static_cast<int>(rows.size());
    _width = 0;
    for (const auto& r : rows) {
        _width = std::max(_width, static_cast<int>(r.size()));
    }

    _map.assign(_height, std::vector<int>(_width, 0));
    _doors.clear();
    _enemySpawns.clear();
    _triggers.clear();
    _testSpawn.reset();

    // Загрузка KV файла (если есть)
    std::string kvPath = base + ".kv";
    std::ifstream kvFile(kvPath);
    if (kvFile.is_open()) {
        std::string kvLine;
        std::string curSection = "";
        while (std::getline(kvFile, kvLine)) {
            if (!kvLine.empty() && kvLine.back() == '\r') kvLine.pop_back();
            if (kvLine.find("Doors") != std::string::npos || kvLine.find("doors") != std::string::npos) {
                curSection = "doors";
                continue;
            }
            if (kvLine.find("Enemies") != std::string::npos || kvLine.find("enemies") != std::string::npos) {
                curSection = "enemies";
                continue;
            }
            if (kvLine.find("Triggers") != std::string::npos || kvLine.find("triggers") != std::string::npos) {
                curSection = "triggers";
                continue;
            }
            if (kvLine.find("{") != std::string::npos || kvLine.find("}") != std::string::npos) {
                if (kvLine.find("}") != std::string::npos) curSection = "";
                continue;
            }
            if (curSection == "doors") {
                size_t q1 = kvLine.find('\"');
                if (q1 != std::string::npos) {
                    size_t q2 = kvLine.find('\"', q1 + 1);
                    if (q2 != std::string::npos) {
                        size_t q3 = kvLine.find('\"', q2 + 1);
                        size_t q4 = (q3 != std::string::npos) ? kvLine.find('\"', q3 + 1) : std::string::npos;
                        if (q3 != std::string::npos && q4 != std::string::npos) {
                            std::string val = kvLine.substr(q3 + 1, q4 - q3 - 1);
                            std::istringstream iss(val);
                            int dx = 0, dy = 0, dtgt = (_currentRoomIndex % _totalRooms) + 1;
                            if (iss >> dx >> dy) {
                                iss >> dtgt;
                                _doors.push_back({dx, dy, dtgt});
                            }
                        }
                    }
                }
            } else if (curSection == "enemies") {
                size_t q1 = kvLine.find('\"');
                if (q1 != std::string::npos) {
                    size_t q2 = kvLine.find('\"', q1 + 1);
                    if (q2 != std::string::npos) {
                        size_t q3 = kvLine.find('\"', q2 + 1);
                        size_t q4 = (q3 != std::string::npos) ? kvLine.find('\"', q3 + 1) : std::string::npos;
                        if (q3 != std::string::npos && q4 != std::string::npos) {
                            std::string val = kvLine.substr(q3 + 1, q4 - q3 - 1);
                            std::istringstream iss(val);
                            int ex = 0, ey = 0, etype = 0;
                            if (iss >> ex >> ey) {
                                iss >> etype;
                                _enemySpawns.push_back({ex, ey, etype, true});
                            }
                        }
                    }
                }
            } else if (curSection == "triggers") {
                size_t q1 = kvLine.find('\"');
                if (q1 != std::string::npos) {
                    size_t q2 = kvLine.find('\"', q1 + 1);
                    if (q2 != std::string::npos) {
                        size_t q3 = kvLine.find('\"', q2 + 1);
                        size_t q4 = (q3 != std::string::npos) ? kvLine.find('\"', q3 + 1) : std::string::npos;
                        if (q3 != std::string::npos && q4 != std::string::npos) {
                            std::string val = kvLine.substr(q3 + 1, q4 - q3 - 1);
                            std::istringstream iss(val);
                            int tx = 0, ty = 0, ttype = 0;
                            float tpow = 1000.0f;
                            if (iss >> tx >> ty >> ttype) {
                                iss >> tpow;
                                _triggers.push_back({tx, ty, ttype, tpow, ""});
                            }
                        }
                    }
                }
            }
        }
    }

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < static_cast<int>(rows[y].size()); ++x) {
            char c = rows[y][x];
            if (c == '=') _map[y][x] = 1;
            else if (c == '#') _map[y][x] = 2;
            else if (c == 'D') {
                _map[y][x] = 3;
                bool found = false;
                for (const auto& d : _doors) {
                    if (d.x == x && d.y == y) { found = true; break; }
                }
                if (!found) _doors.push_back({x, y, (_currentRoomIndex % _totalRooms) + 1});
            }
            else if (c == 'P') _playerSpawn = {x, y};
            else if (c == 'N') addEnemySpawn(x, y, 0);
            else if (c == 'T') addEnemySpawn(x, y, 1);
            else if (c == 'U') addEnemySpawn(x, y, 2);
            else if (c == 'V') addEnemySpawn(x, y, 3);
            else if (c == 'M') addEnemySpawn(x, y, 4); // Dummy (Манекен)
            else if (c == 'X' || c == '>') _testSpawn = sf::Vector2i(x, y);
            else if (c == '^' || c == 'J') addTrigger(x, y, 0);
            else if (c == 'S' || c == 'v') addTrigger(x, y, 1);
        }
    }

    _cameraPos = { static_cast<float>(_playerSpawn.x) * _tileSize, static_cast<float>(_playerSpawn.y) * _tileSize };
    _camera.setCenter(_cameraPos);

    _statusMessage = "Комната [" + name + "] (" + std::to_string(_width) + "x" + std::to_string(_height) + ") загружена!";
    _statusTimer = 3.0f;
    return true;
}

bool EditorApp::loadRoom(int index) {
    if (_roomList.empty()) scanRooms();
    if (index < 1) index = static_cast<int>(_roomList.size());
    if (index > static_cast<int>(_roomList.size())) index = 1;
    _currentRoomIndex = index;
    return loadRoomByName("room_" + std::to_string(index));
}

bool EditorApp::saveToGame() {
    std::string mapsDir = getGameMapsDir();
    std::filesystem::create_directories(mapsDir);
    std::string base = mapsDir + "/" + _currentRoomName;

    std::ofstream txt(base + ".txt");
    if (!txt.is_open()) return false;

    for (int y = 0; y < _height; ++y) {
        std::string row = "";
        for (int x = 0; x < _width; ++x) {
            if (_playerSpawn.x == x && _playerSpawn.y == y) {
                row += 'P';
                continue;
            }
            if (_testSpawn && _testSpawn->x == x && _testSpawn->y == y) {
                row += '>';
                continue;
            }
            bool isEnemy = false;
            for (const auto& es : _enemySpawns) {
                if (es.x == x && es.y == y) {
                    if (es.type == 1) row += 'T';
                    else if (es.type == 2) row += 'U';
                    else if (es.type == 3) row += 'V';
                    else if (es.type == 4) row += 'M'; // Dummy
                    else row += 'N';
                    isEnemy = true;
                    break;
                }
            }
            if (isEnemy) continue;

            bool isTrigger = false;
            for (const auto& tr : _triggers) {
                if (tr.x == x && tr.y == y) {
                    row += (tr.type == 0) ? '^' : (tr.type == 1) ? 'S' : '!';
                    isTrigger = true;
                    break;
                }
            }
            if (isTrigger) continue;

            int tile = _map[y][x];
            if (tile == 1) row += '=';
            else if (tile == 2) row += '#';
            else if (tile == 3) row += 'D';
            else row += '.';
        }
        txt << row << "\n";
    }
    txt.close();

    std::ofstream kv(base + ".kv");
    if (kv.is_open()) {
        kv << "\"Room\"\n{\n";
        kv << "    \"name\"    \"" << _currentRoomName << "\"\n";
        kv << "    \"width\"   \"" << _width << "\"\n";
        kv << "    \"height\"  \"" << _height << "\"\n";
        kv << "    \"player_spawn\"  \"" << _playerSpawn.x << " " << _playerSpawn.y << "\"\n";
        if (_testSpawn) {
            kv << "    \"test_spawn\"    \"" << _testSpawn->x << " " << _testSpawn->y << "\"\n";
        }

        if (!_doors.empty()) {
            kv << "\n    \"Doors\"\n    {\n";
            for (size_t i = 0; i < _doors.size(); ++i) {
                kv << "        \"door_" << i << "\"  \"" << _doors[i].x << " " << _doors[i].y << " " << _doors[i].targetRoom << "\"\n";
            }
            kv << "    }\n";
        }

        if (!_enemySpawns.empty()) {
            kv << "\n    \"Enemies\"\n    {\n";
            for (size_t i = 0; i < _enemySpawns.size(); ++i) {
                kv << "        \"enemy_" << i << "\"  \"" << _enemySpawns[i].x << " " << _enemySpawns[i].y << " " << _enemySpawns[i].type << "\"\n";
            }
            kv << "    }\n";
        }

        if (!_triggers.empty()) {
            kv << "\n    \"Triggers\"\n    {\n";
            for (size_t i = 0; i < _triggers.size(); ++i) {
                kv << "        \"trigger_" << i << "\"  \"" << _triggers[i].x << " " << _triggers[i].y << " " << _triggers[i].type << " " << _triggers[i].power << "\"\n";
            }
            kv << "    }\n";
        }

        kv << "}\n";
        kv.close();
    }

    // Сохранение цветного PNG-превью для браузера комнат
    sf::Image img({static_cast<unsigned int>(_width), static_cast<unsigned int>(_height)}, sf::Color(25, 30, 42));
    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int t = _map[y][x];
            if (t == 1) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(76, 175, 80)); // Platform
            else if (t == 2) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(120, 135, 160)); // Wall
            else if (t == 3) img.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(0, 220, 255)); // Door
        }
    }
    for (const auto& tr : _triggers) {
        if (tr.x >= 0 && tr.x < _width && tr.y >= 0 && tr.y < _height) {
            sf::Color tc = (tr.type == 0) ? sf::Color(255, 170, 0) : sf::Color(230, 40, 40);
            img.setPixel({static_cast<unsigned int>(tr.x), static_cast<unsigned int>(tr.y)}, tc);
        }
    }
    for (const auto& es : _enemySpawns) {
        if (es.x >= 0 && es.x < _width && es.y >= 0 && es.y < _height) {
            sf::Color ec = (es.type == 1) ? sf::Color(255, 40, 40) :
                           (es.type == 2) ? sf::Color(180, 50, 240) :
                           (es.type == 3) ? sf::Color(50, 190, 255) :
                           (es.type == 4) ? sf::Color(220, 175, 75) : sf::Color(180, 180, 180);
            img.setPixel({static_cast<unsigned int>(es.x), static_cast<unsigned int>(es.y)}, ec);
        }
    }
    if (_playerSpawn.x >= 0 && _playerSpawn.x < _width && _playerSpawn.y >= 0 && _playerSpawn.y < _height) {
        img.setPixel({static_cast<unsigned int>(_playerSpawn.x), static_cast<unsigned int>(_playerSpawn.y)}, sf::Color(0, 230, 118));
    }
    img.saveToFile(base + ".png");

    if (_currentRoomName == "room_1") {
        txt.open(getGameConfigDir() + "/map.txt");
        if (txt.is_open()) {
            for (int y = 0; y < _height; ++y) {
                std::string row = "";
                for (int x = 0; x < _width; ++x) {
                    if (_playerSpawn.x == x && _playerSpawn.y == y) { row += 'P'; continue; }
                    int t = _map[y][x];
                    if (t == 1) row += '=';
                    else if (t == 2) row += '#';
                    else if (t == 3) row += 'D';
                    else row += '.';
                }
                txt << row << "\n";
            }
            txt.close();
        }
        img.saveToFile(getGameConfigDir() + "/map.png");
    }

    _statusMessage = "Комната [" + _currentRoomName + "] успешно сохранена (.txt/.kv/.png)!";
    _statusTimer = 3.5f;
    return true;
}

void EditorApp::createNewRoom() {
    createNamedRoom("room_" + std::to_string(_roomList.size() + 1));
}

void EditorApp::nextRoom() {
    saveToGame();
    if (_roomList.empty()) scanRooms();
    int curIdx = 0;
    for (size_t i = 0; i < _roomList.size(); ++i) {
        if (_roomList[i].id == _currentRoomName) { curIdx = static_cast<int>(i); break; }
    }
    int nextIdx = (curIdx + 1) % _roomList.size();
    loadRoomByName(_roomList[nextIdx].id);
}

void EditorApp::prevRoom() {
    saveToGame();
    if (_roomList.empty()) scanRooms();
    int curIdx = 0;
    for (size_t i = 0; i < _roomList.size(); ++i) {
        if (_roomList[i].id == _currentRoomName) { curIdx = static_cast<int>(i); break; }
    }
    int prevIdx = (curIdx - 1 + _roomList.size()) % _roomList.size();
    loadRoomByName(_roomList[prevIdx].id);
}

void EditorApp::zoomIn() {
    _viewScale = std::max(0.35f, _viewScale - 0.2f);
    _camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    _statusMessage = "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "%";
    _statusTimer = 1.5f;
}

void EditorApp::zoomOut() {
    _viewScale = std::min(3.5f, _viewScale + 0.25f);
    _camera.setSize({ _baseCameraSize.x * _viewScale, _baseCameraSize.y * _viewScale });
    _statusMessage = "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "%";
    _statusTimer = 1.5f;
}

void EditorApp::resetZoom() {
    _viewScale = 1.0f;
    _camera.setSize(_baseCameraSize);
    _statusMessage = "Зум сброшен на 100%";
    _statusTimer = 1.5f;
}

void EditorApp::cycleBrushSize() {
    if (_brushSize == 1) _brushSize = 2;
    else if (_brushSize == 2) _brushSize = 3;
    else if (_brushSize == 3) _brushSize = 5;
    else _brushSize = 1;
    _statusMessage = "Кисть: " + std::to_string(_brushSize) + "x" + std::to_string(_brushSize);
    _statusTimer = 1.5f;
}

void EditorApp::resizeMap(int newW, int newH) {
    if (newW < 20) newW = 20;
    if (newH < 10) newH = 10;
    pushUndo();

    std::vector<std::vector<int>> newMap(newH, std::vector<int>(newW, 0));
    int copyH = std::min(_height, newH);
    int copyW = std::min(_width, newW);
    for (int y = 0; y < copyH; ++y) {
        for (int x = 0; x < copyW; ++x) {
            newMap[y][x] = _map[y][x];
        }
    }
    _width = newW;
    _height = newH;
    _map = std::move(newMap);
    _statusMessage = "Размер изменен на: " + std::to_string(_width) + "x" + std::to_string(_height);
    _statusTimer = 2.0f;
}

void EditorApp::applyToolAt(int tx, int ty) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;

    if (_currentTool == ToolType::PLAYER_SPAWN) {
        _map[ty][tx] = 0;
        _playerSpawn = {tx, ty};
        _statusMessage = "Спавн игрока: [" + std::to_string(tx) + ", " + std::to_string(ty) + "]";
        _statusTimer = 1.5f;
        return;
    }
    if (_currentTool == ToolType::TEST_SPAWN) {
        _map[ty][tx] = 0;
        _testSpawn = sf::Vector2i(tx, ty);
        _statusMessage = "Тестовый чекпоинт установлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "] (Нажмите Tab для старта)";
        _statusTimer = 2.5f;
        return;
    }
    if (_currentTool == ToolType::ENEMY_SPAWN || _currentTool == ToolType::ENEMY_TANK || 
        _currentTool == ToolType::ENEMY_DUELIST || _currentTool == ToolType::ENEMY_DUELIST_PASSIVE ||
        _currentTool == ToolType::ENEMY_DUMMY) {
        _map[ty][tx] = 0;
        int et = (_currentTool == ToolType::ENEMY_TANK) ? 1 : 
                 (_currentTool == ToolType::ENEMY_DUELIST) ? 2 : 
                 (_currentTool == ToolType::ENEMY_DUELIST_PASSIVE) ? 3 : 
                 (_currentTool == ToolType::ENEMY_DUMMY) ? 4 : 0;
        addEnemySpawn(tx, ty, et);
        return;
    }
    if (_currentTool == ToolType::DOOR) {
        setDoor(tx, ty, _doorTargetRoom);
        _statusMessage = "Дверь (D) -> Цель: Комната " + std::to_string(_doorTargetRoom);
        _statusTimer = 2.0f;
        return;
    }
    if (_currentTool == ToolType::JUMP_PAD || _currentTool == ToolType::SPIKES || _currentTool == ToolType::TRIGGER_ZONE) {
        int tt = (_currentTool == ToolType::JUMP_PAD) ? 0 : (_currentTool == ToolType::SPIKES) ? 1 : 2;
        addTrigger(tx, ty, tt);
        return;
    }
    if (_currentTool == ToolType::EDGE_PASSAGE) {
        eraseAt(tx, ty);
        return;
    }

    int tileVal = (_currentTool == ToolType::PLATFORM ? 1 : 2);
    int half = _brushSize / 2;
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            int cx = tx + dx;
            int cy = ty + dy;
            if (cx >= 0 && cx < _width && cy >= 0 && cy < _height) {
                _map[cy][cx] = tileVal;
                removeDoor(cx, cy);
                removeTrigger(cx, cy);
            }
        }
    }
}

void EditorApp::eraseAt(int tx, int ty) {
    int half = _brushSize / 2;
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            int cx = tx + dx;
            int cy = ty + dy;
            if (cx >= 0 && cx < _width && cy >= 0 && cy < _height) {
                _map[cy][cx] = 0;
                removeEnemySpawn(cx, cy);
                removeDoor(cx, cy);
                removeTrigger(cx, cy);
                if (_testSpawn && _testSpawn->x == cx && _testSpawn->y == cy) {
                    _testSpawn.reset();
                }
            }
        }
    }
}

void EditorApp::addEnemySpawn(int tx, int ty, int type) {
    for (auto& es : _enemySpawns) {
        if (es.x == tx && es.y == ty) {
            es.type = type;
            std::string tName = (type == 1) ? "Жирный (Tank)" : (type == 2) ? "Дуэлянт Агр" : (type == 3) ? "Дуэлянт Пасс" : "Обычный (Norm)";
            _statusMessage = "Враг на [" + std::to_string(tx) + ", " + std::to_string(ty) + "] изменен на " + tName;
            _statusTimer = 1.8f;
            return;
        }
    }
    _enemySpawns.push_back({tx, ty, type, true});
    std::string tName = (type == 1) ? "Жирный (Tank)" : (type == 2) ? "Дуэлянт Агр" : (type == 3) ? "Дуэлянт Пасс" : "Обычный (Norm)";
    _statusMessage = "Враг [" + tName + "] добавлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "] (Всего: " + std::to_string(_enemySpawns.size()) + ")";
    _statusTimer = 1.8f;
}

void EditorApp::removeEnemySpawn(int tx, int ty) {
    for (auto it = _enemySpawns.begin(); it != _enemySpawns.end(); ++it) {
        if (it->x == tx && it->y == ty) {
            _enemySpawns.erase(it);
            return;
        }
    }
}

void EditorApp::setDoor(int tx, int ty, int targetRoom) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;
    _map[ty][tx] = 3;
    for (auto& d : _doors) {
        if (d.x == tx && d.y == ty) {
            d.targetRoom = targetRoom;
            return;
        }
    }
    _doors.push_back({tx, ty, targetRoom});
}

void EditorApp::removeDoor(int tx, int ty) {
    _doors.erase(std::remove_if(_doors.begin(), _doors.end(),
        [tx, ty](const EditorDoor& d) { return d.x == tx && d.y == ty; }), _doors.end());
    if (tx >= 0 && tx < _width && ty >= 0 && ty < _height && _map[ty][tx] == 3) {
        _map[ty][tx] = 0;
    }
}

int EditorApp::getDoorTargetRoom(int tx, int ty) const {
    for (const auto& d : _doors) {
        if (d.x == tx && d.y == ty) return d.targetRoom;
    }
    return (_currentRoomIndex % _totalRooms) + 1;
}

void EditorApp::addTrigger(int tx, int ty, int type) {
    _map[ty][tx] = 0;
    for (auto& tr : _triggers) {
        if (tr.x == tx && tr.y == ty) {
            tr.type = type;
            std::string name = (type == 0) ? "Батут (JumpPad)" : (type == 1) ? "Шипы (Spikes)" : "Триггер (Zone)";
            _statusMessage = name + " обновлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "]";
            _statusTimer = 1.8f;
            return;
        }
    }
    float pow = (type == 0) ? 1150.0f : (type == 1) ? 15.0f : 1.0f;
    _triggers.push_back({tx, ty, type, pow, ""});
    std::string name = (type == 0) ? "Батут (JumpPad)" : (type == 1) ? "Шипы (Spikes)" : "Триггер (Zone)";
    _statusMessage = name + " добавлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "]";
    _statusTimer = 1.8f;
}

void EditorApp::removeTrigger(int tx, int ty) {
    for (auto it = _triggers.begin(); it != _triggers.end(); ++it) {
        if (it->x == tx && it->y == ty) {
            _triggers.erase(it);
            return;
        }
    }
}

bool EditorApp::handleMinimapClick(const sf::Vector2i& mousePixel) {
    if (_minimapRect.contains({static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y)})) {
        float localX = mousePixel.x - _minimapRect.position.x;
        float localY = mousePixel.y - _minimapRect.position.y;

        float mapW = _width * _tileSize;
        float mapH = _height * _tileSize;
        float scaleX = _minimapRect.size.x / mapW;
        float scaleY = _minimapRect.size.y / mapH;
        float scale = std::min(scaleX, scaleY);

        _cameraPos = { localX / scale, localY / scale };
        _camera.setCenter(_cameraPos);
        return true;
    }
    return false;
}

bool EditorApp::isMouseOverUI(const sf::Vector2i& mousePixel) const {
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);
    float mx = static_cast<float>(mousePixel.x);
    float my = static_cast<float>(mousePixel.y);

    // Вне окна
    if (mx < 0.0f || mx > winW || my < 0.0f || my > winH) {
        return true;
    }

    // 1. Верхняя панель управления и подсказок (высота 120px)
    if (my <= 122.0f) {
        return true;
    }
    // Всплывающее сообщение по центру под верхней панелью
    if (!_statusMessage.empty() && _statusTimer > 0.0f && my <= 175.0f) {
        float msgBgW = 550.0f;
        float msgBgX = (winW - msgBgW) / 2.0f;
        if (mx >= msgBgX && mx <= msgBgX + msgBgW) return true;
    }

    // 2. Вся нижняя область (палитра блоков, вкладки, кнопки действий, отступы)
    // Палитра начинается с winH - 130px. Берем с запасом winH - 138px.
    if (my >= winH - 138.0f) {
        return true;
    }

    // 3. Миникарта (в правом нижнем углу)
    if (_minimapRect.contains({mx, my})) {
        return true;
    }

    // 4. Модальное окно настроек объекта (на ПКМ)
    if (_modal.active) {
        float mw = 285.0f;
        float mh = 230.0f;
        sf::FloatRect modalBox({_modal.screenPos.x - 4.0f, _modal.screenPos.y - 4.0f}, {mw, mh});
        if (modalBox.contains({mx, my})) {
            return true;
        }
    }

    // 5. Вкладка комнат (ROOMS) или окно именования комнаты
    if (_activeTab == PaletteTab::ROOMS || _isNamingRoom) {
        return true;
    }

    return false;
}

void EditorApp::openSettingsModal(int tx, int ty, const sf::Vector2f& screenPos) {
    if (tx < 0 || tx >= _width || ty < 0 || ty >= _height) return;

    _modal.active = true;
    _modal.tile = {tx, ty};
    _modal.screenPos = screenPos;

    // Ограничиваем координаты модального окна в пределах экрана
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);
    if (_modal.screenPos.x + 280.0f > winW) _modal.screenPos.x = winW - 290.0f;
    if (_modal.screenPos.y + 240.0f > winH) _modal.screenPos.y = winH - 250.0f;

    // Определяем тип объекта
    if (_testSpawn && _testSpawn->x == tx && _testSpawn->y == ty) {
        _modal.type = SettingsModal::TargetType::TEST_SPAWN;
        _modal.targetIndex = 0;
        return;
    }
    for (size_t i = 0; i < _doors.size(); ++i) {
        if (_doors[i].x == tx && _doors[i].y == ty) {
            _modal.type = SettingsModal::TargetType::DOOR;
            _modal.targetIndex = static_cast<int>(i);
            return;
        }
    }
    for (size_t i = 0; i < _enemySpawns.size(); ++i) {
        if (_enemySpawns[i].x == tx && _enemySpawns[i].y == ty) {
            _modal.type = SettingsModal::TargetType::ENEMY;
            _modal.targetIndex = static_cast<int>(i);
            return;
        }
    }
    for (size_t i = 0; i < _triggers.size(); ++i) {
        if (_triggers[i].x == tx && _triggers[i].y == ty) {
            _modal.type = SettingsModal::TargetType::TRIGGER;
            _modal.targetIndex = static_cast<int>(i);
            return;
        }
    }

    _modal.type = SettingsModal::TargetType::TILE;
    _modal.targetIndex = _map[ty][tx];
}

void EditorApp::closeSettingsModal() {
    _modal.active = false;
    _modal.type = SettingsModal::TargetType::NONE;
}

bool EditorApp::handleModalClick(const sf::Vector2i& mousePixel) {
    if (!_modal.active) return false;

    float mx = _modal.screenPos.x;
    float my = _modal.screenPos.y;
    float mw = 280.0f;
    float mh = 220.0f;

    sf::FloatRect modalRect({mx, my}, {mw, mh});
    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    if (!modalRect.contains(mPos)) {
        closeSettingsModal();
        return false;
    }

    // Кнопка закрытия [X]
    sf::FloatRect closeBtn({mx + mw - 24.0f, my + 4.0f}, {20.0f, 20.0f});
    if (closeBtn.contains(mPos)) {
        closeSettingsModal();
        return true;
    }

    // 1. НАСТРОЙКИ ДВЕРИ
    if (_modal.type == SettingsModal::TargetType::DOOR) {
        if (_modal.targetIndex >= 0 && _modal.targetIndex < static_cast<int>(_doors.size())) {
            // Кнопки комнат 1..10
            for (int r = 1; r <= 10; ++r) {
                int col = (r - 1) % 5;
                int row = (r - 1) / 5;
                float bx = mx + 16.0f + col * 48.0f;
                float by = my + 42.0f + row * 34.0f;
                sf::FloatRect rBtn({bx, by}, {42.0f, 28.0f});
                if (rBtn.contains(mPos)) {
                    pushUndo();
                    _doors[_modal.targetIndex].targetRoom = r;
                    _doorTargetRoom = r;
                    _statusMessage = "Дверь: целевая комната установлена на " + std::to_string(r);
                    _statusTimer = 2.0f;
                    closeSettingsModal();
                    return true;
                }
            }
            // Кнопка удаления двери
            sf::FloatRect delBtn({mx + 16.0f, my + 130.0f}, {mw - 32.0f, 30.0f});
            if (delBtn.contains(mPos)) {
                pushUndo();
                removeDoor(_modal.tile.x, _modal.tile.y);
                closeSettingsModal();
                return true;
            }
        }
    }

    // 2. НАСТРОЙКИ ТЕСТОВОГО СТАРТА
    if (_modal.type == SettingsModal::TargetType::TEST_SPAWN) {
        // Большая зеленая кнопка: ЗАПУСК СИМУЛЯЦИИ ОТСЮДА
        sf::FloatRect playBtn({mx + 16.0f, my + 45.0f}, {mw - 32.0f, 44.0f});
        if (playBtn.contains(mPos)) {
            closeSettingsModal();
            startSimulation(_modal.tile);
            return true;
        }
        // Кнопка удаления
        sf::FloatRect delBtn({mx + 16.0f, my + 110.0f}, {mw - 32.0f, 30.0f});
        if (delBtn.contains(mPos)) {
            pushUndo();
            _testSpawn.reset();
            closeSettingsModal();
            return true;
        }
    }

    // 3. НАСТРОЙКИ ВРАГА
    if (_modal.type == SettingsModal::TargetType::ENEMY) {
        if (_modal.targetIndex >= 0 && _modal.targetIndex < static_cast<int>(_enemySpawns.size())) {
            const char* archNames[5] = {"Обычный", "Танк", "Дуэль-Агр", "Дуэль-Пасс", "Манекен"};
            for (int a = 0; a < 4; ++a) {
                float bx = mx + 16.0f + (a % 2) * 122.0f;
                float by = my + 38.0f + (a / 2) * 30.0f;
                sf::FloatRect aBtn({bx, by}, {116.0f, 26.0f});
                if (aBtn.contains(mPos)) {
                    pushUndo();
                    _enemySpawns[_modal.targetIndex].type = a;
                    _statusMessage = "Враг изменен: [" + std::string(archNames[a]) + "]";
                    _statusTimer = 2.0f;
                    closeSettingsModal();
                    return true;
                }
            }
            // Кнопка Манекен (Dummy)
            sf::FloatRect dummyBtn({mx + 16.0f, my + 100.0f}, {mw - 32.0f, 26.0f});
            if (dummyBtn.contains(mPos)) {
                pushUndo();
                _enemySpawns[_modal.targetIndex].type = 4;
                _statusMessage = "Враг изменен: [Манекен (Dummy)]";
                _statusTimer = 2.0f;
                closeSettingsModal();
                return true;
            }
            // Кнопка удаления
            sf::FloatRect delBtn({mx + 16.0f, my + 132.0f}, {mw - 32.0f, 26.0f});
            if (delBtn.contains(mPos)) {
                pushUndo();
                removeEnemySpawn(_modal.tile.x, _modal.tile.y);
                closeSettingsModal();
                return true;
            }
        }
    }

    // 4. НАСТРОЙКИ ТРИГГЕРА / БАТУТА / ШИПОВ
    if (_modal.type == SettingsModal::TargetType::TRIGGER) {
        sf::FloatRect b1({mx + 16.0f, my + 42.0f}, {116.0f, 28.0f});
        if (b1.contains(mPos)) {
            pushUndo();
            addTrigger(_modal.tile.x, _modal.tile.y, 0); // JumpPad
            closeSettingsModal();
            return true;
        }
        sf::FloatRect b2({mx + 140.0f, my + 42.0f}, {116.0f, 28.0f});
        if (b2.contains(mPos)) {
            pushUndo();
            addTrigger(_modal.tile.x, _modal.tile.y, 1); // Spikes
            closeSettingsModal();
            return true;
        }
        sf::FloatRect delBtn({mx + 16.0f, my + 120.0f}, {mw - 32.0f, 30.0f});
        if (delBtn.contains(mPos)) {
            pushUndo();
            removeTrigger(_modal.tile.x, _modal.tile.y);
            closeSettingsModal();
            return true;
        }
    }

    // 5. НАСТРОЙКИ БЛОКА
    if (_modal.type == SettingsModal::TargetType::TILE) {
        sf::FloatRect bWall({mx + 16.0f, my + 42.0f}, {mw - 32.0f, 28.0f});
        if (bWall.contains(mPos)) {
            pushUndo();
            _map[_modal.tile.y][_modal.tile.x] = 2;
            closeSettingsModal();
            return true;
        }
        sf::FloatRect bPlat({mx + 16.0f, my + 78.0f}, {mw - 32.0f, 28.0f});
        if (bPlat.contains(mPos)) {
            pushUndo();
            _map[_modal.tile.y][_modal.tile.x] = 1;
            closeSettingsModal();
            return true;
        }
        sf::FloatRect bErase({mx + 16.0f, my + 114.0f}, {mw - 32.0f, 28.0f});
        if (bErase.contains(mPos)) {
            pushUndo();
            eraseAt(_modal.tile.x, _modal.tile.y);
            closeSettingsModal();
            return true;
        }
    }

    return true;
}

bool EditorApp::handleRoomBrowserClick(const sf::Vector2i& mousePixel) {
    if (_activeTab != PaletteTab::ROOMS) return false;
    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);
    float panelW = std::min(winW - 60.0f, 960.0f);
    float panelH = winH - 160.0f;
    float panelX = (winW - panelW) / 2.0f;
    float panelY = 46.0f;

    // Кнопка [ + Создать новую комнату ]
    sf::FloatRect newBtn({panelX + panelW - 225.0f, panelY + 6.0f}, {210.0f, 32.0f});
    if (newBtn.contains(mPos)) {
        _isNamingRoom = true;
        _newRoomNameInput = "";
        return true;
    }

    float contentX = panelX + 16.0f;
    float contentY = panelY + 70.0f;
    float contentW = panelW - 32.0f;
    float colGap = 16.0f;
    float cardW = (contentW - colGap) / 2.0f;
    float cardH = 145.0f;
    float rowGap = 16.0f;

    for (size_t i = 0; i < _roomList.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        float cx = contentX + col * (cardW + colGap);
        float cy = contentY + row * (cardH + rowGap) - _roomBrowserScrollY;

        sf::FloatRect cardRect({cx, cy}, {cardW, cardH});
        if (cardRect.contains(mPos)) {
            _roomBrowserSelected = static_cast<int>(i);
            loadRoomByName(_roomList[i].id);
            _activeTab = PaletteTab::BLOCKS;
            return true;
        }
    }
    return true;
}

bool EditorApp::handlePaletteClick(const sf::Vector2i& mousePixel) {
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);
    float mmW = std::min(240.0f, winW * 0.22f);
    float mmX = winW - mmW - 15.0f;
    float palX = 15.0f;
    float palH = 118.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX - palX - 12.0f;

    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    if (mPos.x < palX || mPos.x > palX + palW || mPos.y < palY || mPos.y > palY + palH) {
        return false;
    }

    // 1. ВКЛАДКИ ПАЛИТРЫ (TABS)
    float tabW = 150.0f;
    float tabH = 22.0f;
    float tabY = palY + 4.0f;

    // Вкладка 0: Блоки
    sf::FloatRect tab0Rect({palX + 10.0f, tabY}, {tabW, tabH});
    if (tab0Rect.contains(mPos)) {
        _activeTab = PaletteTab::BLOCKS;
        return true;
    }
    // Вкладка 1: Сущности
    sf::FloatRect tab1Rect({palX + 168.0f, tabY}, {tabW, tabH});
    if (tab1Rect.contains(mPos)) {
        _activeTab = PaletteTab::ENTITIES;
        return true;
    }
    // Вкладка 2: Механики
    sf::FloatRect tab2Rect({palX + 326.0f, tabY}, {tabW + 20.0f, tabH});
    if (tab2Rect.contains(mPos)) {
        _activeTab = PaletteTab::MECHANICS;
        return true;
    }
    // Вкладка 3: Комнаты
    sf::FloatRect tab3Rect({palX + 504.0f, tabY}, {130.0f, tabH});
    if (tab3Rect.contains(mPos)) {
        _activeTab = PaletteTab::ROOMS;
        scanRooms();
        return true;
    }

    // 2. Кнопка запуска симуляции в правом верхнем углу палитры
    sf::FloatRect playBtn({palX + 642.0f, tabY}, {125.0f, tabH});
    if (playBtn.contains(mPos)) {
        startSimulation();
        return true;
    }

    // 3. Кнопка [Клон в R...]
    sf::FloatRect dupBtn({palX + 775.0f, tabY}, {95.0f, tabH});
    if (dupBtn.contains(mPos)) {
        duplicateRoom(_doorTargetRoom);
        return true;
    }

    // 4. Кнопка [Очистить]
    sf::FloatRect clrBtn({palX + 878.0f, tabY}, {75.0f, tabH});
    if (clrBtn.contains(mPos)) {
        clearRoom();
        return true;
    }

    // 5. Кнопка [F5 - Обновить]
    float reloadX = std::max(palX + 835.0f, palX + palW - 110.0f);
    sf::FloatRect reloadBtn({reloadX, tabY}, {105.0f, tabH});
    if (reloadBtn.contains(mPos)) {
        scanTileSprites();
        return true;
    }

    // 6. Карточки инструментов текущей вкладки
    float startX = palX + 10.0f;
    float cardY = palY + 30.0f;
    float cardW = 60.0f;
    float cardH = 78.0f;

    int slotIdx = 0;
    for (size_t i = 0; i < _paletteSprites.size(); ++i) {
        if (_paletteSprites[i].tab != _activeTab) continue;

        float cardX = startX + slotIdx * 65.0f;
        slotIdx++;
        if (cardX + cardW > palX + palW) break;

        sf::FloatRect cardRect({cardX, cardY}, {cardW, cardH});
        if (cardRect.contains(mPos)) {
            _selectedPaletteIndex = static_cast<int>(i);
            _currentTool = _paletteSprites[i].tool;
            _statusMessage = "Выбран инструмент: " + _paletteSprites[i].name;
            _statusTimer = 2.0f;
            return true;
        }
    }

    return true;
}

void EditorApp::drawJumpArc() {
    if (!_showJumpRuler) return;

    sf::Vector2f startPos;
    if (_lastHoverTile.x >= 0 && _lastHoverTile.x < _width &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < _height) {
        startPos = { static_cast<float>(_lastHoverTile.x * _tileSize + _tileSize / 2.0f),
                     static_cast<float>(_lastHoverTile.y * _tileSize) };
    } else {
        startPos = { static_cast<float>(_playerSpawn.x * _tileSize + _tileSize / 2.0f),
                     static_cast<float>(_playerSpawn.y * _tileSize) };
    }

    const float vx = 380.0f;
    const float vy0 = -800.0f;
    const float g = 980.0f;
    const float dt = 0.02f;
    const float maxT = 1.70f;

    for (int dir : {1, -1}) {
        sf::VertexArray arc(sf::PrimitiveType::LineStrip);
        sf::Vector2f apexPt = startPos;
        float apexY = startPos.y;

        for (float t = 0.0f; t <= maxT; t += dt) {
            float px = startPos.x + dir * vx * t;
            float py = startPos.y + vy0 * t + 0.5f * g * t * t;
            sf::Vector2f curPt(px, py);

            if (py < apexY) {
                apexY = py;
                apexPt = curPt;
            }

            int tx = static_cast<int>(std::floor(px / _tileSize));
            int ty = static_cast<int>(std::floor(py / _tileSize));
            if (t > 0.08f && ((tx >= 0 && tx < _width && ty >= 0 && ty < _height && _map[ty][tx] > 0) || py > startPos.y + 350.0f)) {
                arc.append(sf::Vertex({curPt, sf::Color(255, 215, 0, 180)}));
                break;
            }

            sf::Color col = (t < 0.8f) ? sf::Color(0, 230, 255, 210) : sf::Color(255, 200, 50, 160);
            arc.append(sf::Vertex({curPt, col}));
        }
        _window.draw(arc);

        sf::CircleShape apexCircle(4.0f);
        apexCircle.setFillColor(sf::Color(255, 215, 0));
        apexCircle.setOrigin({4.0f, 4.0f});
        apexCircle.setPosition(apexPt);
        _window.draw(apexCircle);

        if (_fontLoaded && dir == 1) {
            float jumpHeight = std::abs(apexPt.y - startPos.y);
            sf::Text aTxt(_font, "Apex: -" + std::to_string(static_cast<int>(jumpHeight)) + "px", 11);
            aTxt.setFillColor(sf::Color(255, 235, 100));
            aTxt.setPosition({apexPt.x + 6.0f, apexPt.y - 14.0f});
            _window.draw(aTxt);
        }
    }

    const float dashDist = 180.0f;
    for (int dir : {1, -1}) {
        sf::VertexArray dashLine(sf::PrimitiveType::Lines);
        dashLine.append(sf::Vertex({startPos, sf::Color(255, 80, 220, 220)}));
        dashLine.append(sf::Vertex({sf::Vector2f(startPos.x + dir * dashDist, startPos.y), sf::Color(255, 80, 220, 220)}));
        _window.draw(dashLine);

        sf::CircleShape dashEnd(3.0f);
        dashEnd.setFillColor(sf::Color(255, 80, 220));
        dashEnd.setOrigin({3.0f, 3.0f});
        dashEnd.setPosition({startPos.x + dir * dashDist, startPos.y});
        _window.draw(dashEnd);
    }
}

void EditorApp::handleEvents() {
    while (const std::optional<sf::Event> event = _window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            _window.close();
            return;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            float w = static_cast<float>(resized->size.x);
            float h = static_cast<float>(resized->size.y);
            _baseCameraSize = {w, h};
            _camera.setSize({w * _viewScale, h * _viewScale});
        }

        // Если активна симуляция игры
        if (_isSimulating) {
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Tab || kp->code == sf::Keyboard::Key::Escape) {
                    stopSimulation();
                    return;
                }
            }
            if (_simPlayer) _simPlayer->handleInput(event);
            if (_simCombat && _simPlayer) {
                sf::Vector2i mPixel = sf::Mouse::getPosition(_window);
                sf::Vector2f mWorld = _window.mapPixelToCoords(mPixel, _camera);
                _simCombat->handleInput(event, *_simPlayer, mWorld);
            }
            continue;
        }

        // Если открыто модальное окно настроек (на ПКМ)
        if (_modal.active) {
            if (const auto* mbP = event->getIf<sf::Event::MouseButtonPressed>()) {
                _isMouseCapturedByUI = true;
                if (mbP->button == sf::Mouse::Button::Left) {
                    handleModalClick(sf::Mouse::getPosition(_window));
                    continue;
                } else if (mbP->button == sf::Mouse::Button::Right) {
                    closeSettingsModal();
                    continue;
                }
            }
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    closeSettingsModal();
                    continue;
                }
            }
        }

        // Если открыт ввод названия новой комнаты
        if (_isNamingRoom) {
            if (const auto* te = event->getIf<sf::Event::TextEntered>()) {
                char32_t unicode = te->unicode;
                if (unicode == 8) { // Backspace
                    if (!_newRoomNameInput.empty()) _newRoomNameInput.pop_back();
                } else if (unicode == 13 || unicode == 10) { // Enter
                    if (!_newRoomNameInput.empty()) {
                        createNamedRoom(_newRoomNameInput);
                    }
                    _isNamingRoom = false;
                } else if (unicode == 27) { // Escape
                    _isNamingRoom = false;
                } else if (unicode >= 32 && unicode < 127) {
                    char c = static_cast<char>(unicode);
                    if (std::isalnum(c) || c == '_' || c == '-') {
                        if (_newRoomNameInput.size() < 24) _newRoomNameInput += c;
                    }
                }
                continue;
            }
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Escape) {
                    _isNamingRoom = false;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Enter) {
                    if (!_newRoomNameInput.empty()) {
                        createNamedRoom(_newRoomNameInput);
                    }
                    _isNamingRoom = false;
                    continue;
                }
            }
            continue;
        }

        // Если активна вкладка комнат (ROOMS)
        if (_activeTab == PaletteTab::ROOMS) {
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Up) {
                    if (_roomBrowserSelected >= 2) _roomBrowserSelected -= 2;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Down) {
                    if (_roomBrowserSelected + 2 < static_cast<int>(_roomList.size())) _roomBrowserSelected += 2;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Left) {
                    if (_roomBrowserSelected > 0) _roomBrowserSelected--;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Right) {
                    if (_roomBrowserSelected + 1 < static_cast<int>(_roomList.size())) _roomBrowserSelected++;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Enter) {
                    if (_roomBrowserSelected >= 0 && _roomBrowserSelected < static_cast<int>(_roomList.size())) {
                        loadRoomByName(_roomList[_roomBrowserSelected].id);
                        _activeTab = PaletteTab::BLOCKS;
                    }
                    continue;
                } else if (kp->code == sf::Keyboard::Key::Escape) {
                    _activeTab = PaletteTab::BLOCKS;
                    continue;
                } else if (kp->code == sf::Keyboard::Key::N) {
                    _isNamingRoom = true;
                    _newRoomNameInput = "";
                    continue;
                }
            }
            if (const auto* mw = event->getIf<sf::Event::MouseWheelScrolled>()) {
                _roomBrowserScrollY -= mw->delta * 40.0f;
                if (_roomBrowserScrollY < 0.0f) _roomBrowserScrollY = 0.0f;
                continue;
            }
        }

        if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
            bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
            bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
            bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);

            // Старт симуляции с тестового чекпоинта или спавна игрока (Tab / P)
            if (kp->code == sf::Keyboard::Key::Tab || (kp->code == sf::Keyboard::Key::P && !ctrlHeld)) {
                startSimulation();
                continue;
            }

            // Быстрый выбор Ластика [E] и Кисти [B]
            if (kp->code == sf::Keyboard::Key::E && !ctrlHeld) {
                _currentTool = ToolType::EDGE_PASSAGE;
                _statusMessage = "Инструмент: Ластик [E]";
                _statusTimer = 1.8f;
                continue;
            }
            if (kp->code == sf::Keyboard::Key::B && !ctrlHeld) {
                _currentTool = ToolType::PLATFORM;
                _statusMessage = "Инструмент: Кисть [B]";
                _statusTimer = 1.8f;
                continue;
            }

            if (kp->code == sf::Keyboard::Key::Escape) {
                if (_pasteModeActive) {
                    _pasteModeActive = false;
                    _statusMessage = "Вставка отменена.";
                    _statusTimer = 1.5f;
                    continue;
                }
                if (_selectionActive) {
                    _selectionActive = false;
                    _statusMessage = "Выделение снято.";
                    _statusTimer = 1.5f;
                    continue;
                }
                _window.close();
            }

            if (kp->code == sf::Keyboard::Key::F11) {
                toggleFullscreen();
            }

            // Undo / Redo
            if (ctrlHeld && kp->code == sf::Keyboard::Key::Z) {
                if (shiftHeld) redo();
                else undo();
                continue;
            }
            if (ctrlHeld && kp->code == sf::Keyboard::Key::Y) {
                redo();
                continue;
            }

            // Копирование и Вставка
            if (ctrlHeld && kp->code == sf::Keyboard::Key::C) {
                copySelection();
                continue;
            }
            if (ctrlHeld && kp->code == sf::Keyboard::Key::V) {
                if (_clipboard.hasData) {
                    _pasteModeActive = !_pasteModeActive;
                    _statusMessage = _pasteModeActive ? "Режим вставки: Клик ЛКМ для вставки, Esc для отмены." : "Вставка отменена.";
                    _statusTimer = 2.0f;
                } else {
                    _statusMessage = "Буфер обмена пуст! Сначала выделите (Ctrl+Drag) и нажмите Ctrl+C.";
                    _statusTimer = 2.0f;
                }
                continue;
            }

            // Сетка и видоискатель
            if (ctrlHeld && kp->code == sf::Keyboard::Key::G) {
                _showGrid = !_showGrid;
                _statusMessage = _showGrid ? "Сетка: ВКЛЮЧЕНА" : "Сетка: ВЫКЛЮЧЕНА";
                _statusTimer = 1.5f;
                continue;
            }
            if (ctrlHeld && kp->code == sf::Keyboard::Key::F) {
                _showScreenFrame = !_showScreenFrame;
                _statusMessage = _showScreenFrame ? "Рамка 1920x1080: ВКЛЮЧЕНА" : "Рамка 1920x1080: ВЫКЛЮЧЕНА";
                _statusTimer = 1.5f;
                continue;
            }

            // Дублирование комнаты
            if (ctrlHeld && kp->code == sf::Keyboard::Key::D) {
                duplicateRoom(_doorTargetRoom);
                continue;
            }

            // Линейка прыжка (J)
            if (kp->code == sf::Keyboard::Key::J) {
                _showJumpRuler = !_showJumpRuler;
                _statusMessage = _showJumpRuler ? "Линейка прыжка: ВКЛЮЧЕНА" : "Линейка прыжка: ВЫКЛЮЧЕНА";
                _statusTimer = 1.5f;
                continue;
            }

            // Пипетка (Q)
            if (kp->code == sf::Keyboard::Key::Q) {
                eyedropAt(_lastHoverTile.x, _lastHoverTile.y);
                continue;
            }

            // Ведро / Заливка (G без Ctrl)
            if (!ctrlHeld && kp->code == sf::Keyboard::Key::G) {
                _currentTool = ToolType::FLOOD_FILL;
                _statusMessage = "Инструмент: Заливка Bucket [G]";
                _statusTimer = 2.0f;
                continue;
            }

            // Линия (L)
            if (kp->code == sf::Keyboard::Key::L) {
                _currentTool = ToolType::LINE;
                _statusMessage = "Инструмент: Линия Line [L]";
                _statusTimer = 2.0f;
                continue;
            }

            // Box Fill (X)
            if (!ctrlHeld && kp->code == sf::Keyboard::Key::X) {
                _currentTool = ToolType::BOX_FILL;
                _statusMessage = "Инструмент: Прямоугольник Box Fill [X]";
                _statusTimer = 2.0f;
                continue;
            }

            // Цифровые инструменты
            if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::Numpad1) _currentTool = ToolType::PLATFORM;
            else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::Numpad2) _currentTool = ToolType::WALL;
            else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::Numpad3) _currentTool = ToolType::DOOR;
            else if (kp->code == sf::Keyboard::Key::Num4 || kp->code == sf::Keyboard::Key::Numpad4) _currentTool = ToolType::PLAYER_SPAWN;
            else if (kp->code == sf::Keyboard::Key::Num5 || kp->code == sf::Keyboard::Key::Numpad5) _currentTool = ToolType::ENEMY_SPAWN;
            else if (kp->code == sf::Keyboard::Key::Num6 || kp->code == sf::Keyboard::Key::Numpad6) _currentTool = ToolType::ENEMY_TANK;
            else if (kp->code == sf::Keyboard::Key::Num7 || kp->code == sf::Keyboard::Key::Numpad7) _currentTool = ToolType::ENEMY_DUELIST;
            else if (kp->code == sf::Keyboard::Key::Num8 || kp->code == sf::Keyboard::Key::Numpad8) _currentTool = ToolType::ENEMY_DUELIST_PASSIVE;
            else if (kp->code == sf::Keyboard::Key::Num9 || kp->code == sf::Keyboard::Key::Numpad9) _currentTool = ToolType::ENEMY_DUMMY;

            if (kp->code == sf::Keyboard::Key::PageUp) nextRoom();
            else if (kp->code == sf::Keyboard::Key::PageDown) prevRoom();

            if (kp->code == sf::Keyboard::Key::Z && !ctrlHeld) zoomIn();
            else if (kp->code == sf::Keyboard::Key::C && !ctrlHeld) resetZoom();

            if ((kp->code == sf::Keyboard::Key::S && ctrlHeld) || kp->code == sf::Keyboard::Key::F7) {
                saveToGame();
            }

            if (altHeld) {
                if (kp->code == sf::Keyboard::Key::Right) resizeMap(_width + 10, _height);
                else if (kp->code == sf::Keyboard::Key::Left) resizeMap(_width - 10, _height);
                else if (kp->code == sf::Keyboard::Key::Down) resizeMap(_width, _height + 5);
                else if (kp->code == sf::Keyboard::Key::Up) resizeMap(_width, _height - 5);
            }
        }

        if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
            bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
            if (ctrlHeld) {
                if (wheel->delta > 0) zoomIn();
                else if (wheel->delta < 0) zoomOut();
            }
        }

        if (const auto* mbP = event->getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);

            // Перехват клика элементами интерфейса (палитра, вкладки, кнопки, верхняя панель, миникарта)
            if (isMouseOverUI(mousePixel)) {
                _isMouseCapturedByUI = true;
                if (mbP->button == sf::Mouse::Button::Left) {
                    if (_activeTab == PaletteTab::ROOMS) {
                        if (handleRoomBrowserClick(mousePixel)) continue;
                    }
                    if (handlePaletteClick(mousePixel)) continue;
                    if (handleMinimapClick(mousePixel)) continue;
                }
                // Любой клик по UI поглощается и не просачивается в игровой мир!
                continue;
            }

            bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
            bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                           sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
            bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
            bool spaceHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);

            // Перемещение с зажатым Пробелом (Space + Drag) или СКМ
            if (mbP->button == sf::Mouse::Button::Middle || (mbP->button == sf::Mouse::Button::Left && spaceHeld)) {
                if (altHeld || ctrlHeld) {
                    eyedropAt(_lastHoverTile.x, _lastHoverTile.y);
                } else {
                    _isPanning = true;
                    _lastMousePixel = sf::Mouse::getPosition(_window);
                }
                continue;
            }

            if (mbP->button == sf::Mouse::Button::Left) {
                if (_pasteModeActive) {
                    pasteClipboard(_lastHoverTile.x, _lastHoverTile.y);
                    continue;
                }

                if (altHeld && !shiftHeld) {
                    if (_currentTool == ToolType::LINE) {
                        _lineDragActive = true;
                        _lineStartTile = _lineCurTile = _lastHoverTile;
                    } else {
                        eyedropAt(_lastHoverTile.x, _lastHoverTile.y);
                    }
                    continue;
                }

                if (ctrlHeld) {
                    _selectionActive = true;
                    _selStart = _selEnd = _lastHoverTile;
                    continue;
                }

                if (shiftHeld || _currentTool == ToolType::BOX_FILL) {
                    _boxDragActive = true;
                    _boxIsErase = false;
                    _boxStartTile = _boxCurTile = _lastHoverTile;
                    continue;
                }

                if (_currentTool == ToolType::LINE) {
                    _lineDragActive = true;
                    _lineStartTile = _lineCurTile = _lastHoverTile;
                    continue;
                }

                if (_currentTool == ToolType::FLOOD_FILL) {
                    floodFill(_lastHoverTile.x, _lastHoverTile.y, 1);
                    continue;
                }

                if (_currentTool == ToolType::EYEDROPPER) {
                    eyedropAt(_lastHoverTile.x, _lastHoverTile.y);
                    continue;
                }

                pushUndo();
                applyToolAt(_lastHoverTile.x, _lastHoverTile.y);
            } else if (mbP->button == sf::Mouse::Button::Right) {
                // Shift + RMB: Box Erase
                if (shiftHeld) {
                    _boxDragActive = true;
                    _boxIsErase = true;
                    _boxStartTile = _boxCurTile = _lastHoverTile;
                    continue;
                }

                // ПКМ: ОТКРЫТЬ НАСТРОЙКИ БЛОКА (Особенно важно для дверей и тестстартпозов)
                sf::Vector2i mPixel = sf::Mouse::getPosition(_window);
                if (_lastHoverTile.x >= 0 && _lastHoverTile.y >= 0) {
                    openSettingsModal(_lastHoverTile.x, _lastHoverTile.y, sf::Vector2f(static_cast<float>(mPixel.x), static_cast<float>(mPixel.y)));
                }
            }
        }

        if (const auto* mbR = event->getIf<sf::Event::MouseButtonReleased>()) {
            if (mbR->button == sf::Mouse::Button::Middle || mbR->button == sf::Mouse::Button::Left) {
                _isPanning = false;
            }

            if (mbR->button == sf::Mouse::Button::Left || mbR->button == sf::Mouse::Button::Right) {
                _isMouseCapturedByUI = false;
                if (_boxDragActive) {
                    pushUndo();
                    int x1 = std::max(0, std::min(_boxStartTile.x, _boxCurTile.x));
                    int x2 = std::min(_width - 1, std::max(_boxStartTile.x, _boxCurTile.x));
                    int y1 = std::max(0, std::min(_boxStartTile.y, _boxCurTile.y));
                    int y2 = std::min(_height - 1, std::max(_boxStartTile.y, _boxCurTile.y));

                    int tileVal = (_currentTool == ToolType::WALL ? 2 : 1);
                    for (int y = y1; y <= y2; ++y) {
                        for (int x = x1; x <= x2; ++x) {
                            if (_boxIsErase) {
                                _map[y][x] = 0;
                                removeDoor(x, y);
                                removeEnemySpawn(x, y);
                                removeTrigger(x, y);
                            } else {
                                _map[y][x] = tileVal;
                                removeDoor(x, y);
                                removeTrigger(x, y);
                            }
                        }
                    }
                    _statusMessage = _boxIsErase ? "Область Box стерта!" : "Область Box залита!";
                    _statusTimer = 2.0f;
                    _boxDragActive = false;
                }

                if (_lineDragActive) {
                    pushUndo();
                    auto lineTiles = getEditorLineTiles(_lineStartTile.x, _lineStartTile.y, _lineCurTile.x, _lineCurTile.y);
                    int tileVal = (_currentTool == ToolType::WALL ? 2 : 1);
                    for (const auto& pt : lineTiles) {
                        if (pt.x >= 0 && pt.x < _width && pt.y >= 0 && pt.y < _height) {
                            _map[pt.y][pt.x] = tileVal;
                            removeDoor(pt.x, pt.y);
                            removeTrigger(pt.x, pt.y);
                        }
                    }
                    _statusMessage = "Линия размещена (" + std::to_string(lineTiles.size()) + " блоков)!";
                    _statusTimer = 2.0f;
                    _lineDragActive = false;
                }
            }
        }
    }
}

void EditorApp::update(float dt) {
    if (_statusTimer > 0.0f) {
        _statusTimer -= dt;
        if (_statusTimer <= 0.0f) _statusMessage.clear();
    }

    if (!_window.hasFocus()) return;

    // ОБНОВЛЕНИЕ РЕЖИМА СИМУЛЯЦИИ ИГРЫ
    if (_isSimulating) {
        sf::Vector2i mPixel = sf::Mouse::getPosition(_window);
        sf::Vector2f mWorld = _window.mapPixelToCoords(mPixel, _camera);

        if (_simArena) _simArena->update(dt);
        if (_simPlayer && _simArena) {
            _simPlayer->update(dt, *_simArena, mWorld);

            // Обработка триггеров симуляции: Батуты (JumpPad) и Шипы (Spikes)
            float ts = _tileSize;
            int pTx = static_cast<int>(std::floor(_simPlayer->position.x / ts));
            int pTy = static_cast<int>(std::floor((_simPlayer->position.y + 20.0f) / ts));
            for (const auto& tr : _triggers) {
                if (tr.x == pTx && (tr.y == pTy || tr.y == pTy - 1)) {
                    if (tr.type == 0) { // JumpPad
                        _simPlayer->velocity.y = -tr.power;
                    } else if (tr.type == 1) { // Spikes
                        _simPlayer->takeDamage(tr.power * dt * 2.0f, 0.0f, -220.0f, 0.2f);
                    }
                }
            }

            bool playerIsAttacking = _simCombat ? (_simCombat->isAttacking() || _simCombat->getCooldown() > 0.0f) : false;
            for (auto& e : _simEnemies) {
                e.update(dt, *_simArena, _simPlayer->position, _simPlayer->isStunned(), playerIsAttacking);
            }
            if (_simCombat) {
                _simCombat->update(dt, *_simPlayer, _simEnemies);
                _camera.setCenter(_simPlayer->position + _simCombat->getScreenShake());
            } else {
                _camera.setCenter(_simPlayer->position);
            }
        }
        return;
    }

    // РЕЖИМ РЕДАКТОРА
    if (_isPanning) {
        sf::Vector2i cur = sf::Mouse::getPosition(_window);
        sf::Vector2i delta = cur - _lastMousePixel;
        _cameraPos -= sf::Vector2f(delta.x * _viewScale, delta.y * _viewScale);
        _camera.setCenter(_cameraPos);
        _lastMousePixel = cur;
    } else {
        sf::Vector2f moveDir(0.0f, 0.0f);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) moveDir.x -= 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) moveDir.x += 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) moveDir.y -= 1.0f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
            if (!ctrlHeld) moveDir.y += 1.0f;
        }

        if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
            float len = std::hypot(moveDir.x, moveDir.y);
            if (len > 0.0001f) moveDir /= len;

            bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                             sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
            float speed = shiftHeld ? (_camSpeed * 3.0f) : _camSpeed;

            _cameraPos += moveDir * (speed * _viewScale * dt);
            _camera.setCenter(_cameraPos);
        } else {
            _cameraPos = _camera.getCenter();
        }
    }

    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    if (_isMouseCapturedByUI || isMouseOverUI(mousePixel)) {
        _lastHoverTile = {-1, -1};
        return;
    }

    sf::Vector2f mouseWorld = _window.mapPixelToCoords(mousePixel, _camera);
    int tx = static_cast<int>(std::floor(mouseWorld.x / _tileSize));
    int ty = static_cast<int>(std::floor(mouseWorld.y / _tileSize));
    _lastHoverTile = {tx, ty};

    if (_boxDragActive) _boxCurTile = _lastHoverTile;
    if (_lineDragActive) _lineCurTile = _lastHoverTile;
    if (_selectionActive) _selEnd = _lastHoverTile;

    if (!_modal.active && !_boxDragActive && !_lineDragActive && !_selectionActive && !_pasteModeActive &&
        !sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) &&
        _currentTool != ToolType::BOX_FILL && _currentTool != ToolType::LINE && 
        _currentTool != ToolType::FLOOD_FILL && _currentTool != ToolType::EYEDROPPER) {
        if (tx >= 0 && tx < _width && ty >= 0 && ty < _height) {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                applyToolAt(tx, ty);
            }
        }
    }
}

void EditorApp::drawWorld() {
    sf::View view = _window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / _tileSize) - 1);
    int endX = std::min(_width, static_cast<int>((center.x + size.x / 2.0f) / _tileSize) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / _tileSize) - 1);
    int endY = std::min(_height, static_cast<int>((center.y + size.y / 2.0f) / _tileSize) + 2);

    // 1. Сетка тайлов 64x64
    if (_showGrid) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        for (int x = startX; x <= endX; ++x) {
            lines.append(sf::Vertex({sf::Vector2f(x * _tileSize, startY * _tileSize), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(x * _tileSize, endY * _tileSize), sf::Color(100, 120, 150, 40)}));
        }
        for (int y = startY; y <= endY; ++y) {
            lines.append(sf::Vertex({sf::Vector2f(startX * _tileSize, y * _tileSize), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(endX * _tileSize, y * _tileSize), sf::Color(100, 120, 150, 40)}));
        }
        _window.draw(lines);
    }

    // 2. Рамка экрана 1920x1080 (Viewfinder)
    if (_showScreenFrame) {
        sf::RectangleShape frame({1920.0f, 1080.0f});
        frame.setOrigin({960.0f, 540.0f});
        frame.setPosition(center);
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineColor(sf::Color(255, 80, 220, 180));
        frame.setOutlineThickness(2.0f);
        _window.draw(frame);
    }

    // 3. Блоки карты
    for (int y = startY; y < endY; ++y) {
        for (int x = startX; x < endX; ++x) {
            int tile = _map[y][x];
            if (tile == 0) continue;

            sf::RectangleShape r({_tileSize, _tileSize});
            r.setPosition({x * _tileSize, y * _tileSize});
            if (tile == 1) {
                r.setFillColor(sf::Color(76, 175, 80));
                r.setOutlineColor(sf::Color(120, 200, 120));
                r.setOutlineThickness(-1.0f);
                _window.draw(r);
            } else if (tile == 2) {
                r.setFillColor(sf::Color(70, 85, 110));
                r.setOutlineColor(sf::Color(100, 120, 150));
                r.setOutlineThickness(-1.0f);
                _window.draw(r);
            } else if (tile == 3) {
                r.setFillColor(sf::Color(0, 200, 255, 160));
                r.setOutlineColor(sf::Color(0, 255, 255));
                r.setOutlineThickness(2.0f);
                _window.draw(r);

                int targetRoom = getDoorTargetRoom(x, y);
                if (_fontLoaded) {
                    float bx = static_cast<float>(x) * _tileSize;
                    float by = static_cast<float>(y) * _tileSize;

                    sf::RectangleShape badge({_tileSize - 8.0f, 26.0f});
                    badge.setPosition({bx + 4.0f, by + (_tileSize - 26.0f) / 2.0f});
                    badge.setFillColor(sf::Color(12, 20, 32, 235));
                    badge.setOutlineColor(sf::Color(0, 255, 255, 220));
                    badge.setOutlineThickness(1.0f);
                    _window.draw(badge);

                    std::string label = "D->R" + std::to_string(targetRoom);
                    sf::Text dTxt(_font, toSfString(label), 13);
                    dTxt.setFillColor(sf::Color(0, 255, 240));
                    dTxt.setPosition({bx + 7.0f, by + (_tileSize - 26.0f) / 2.0f + 2.0f});
                    _window.draw(dTxt);
                }
            }
        }
    }

    // 4. Триггеры / Батуты / Шипы (с текстурами)
    for (const auto& tr : _triggers) {
        float tx = static_cast<float>(tr.x) * _tileSize;
        float ty = static_cast<float>(tr.y) * _tileSize;

        if (tr.type == 0 && _texJumpPad && _texJumpPad->getSize().x > 0) {
            sf::Sprite spr(*_texJumpPad);
            spr.setPosition({tx, ty});
            float sx = _tileSize / static_cast<float>(_texJumpPad->getSize().x);
            float sy = _tileSize / static_cast<float>(_texJumpPad->getSize().y);
            spr.setScale({sx, sy});
            _window.draw(spr);
        } else if (tr.type == 1 && _texSpikes && _texSpikes->getSize().x > 0) {
            sf::Sprite spr(*_texSpikes);
            spr.setPosition({tx, ty});
            float sx = _tileSize / static_cast<float>(_texSpikes->getSize().x);
            float sy = _tileSize / static_cast<float>(_texSpikes->getSize().y);
            spr.setScale({sx, sy});
            _window.draw(spr);
        } else {
            sf::RectangleShape tBox({_tileSize, _tileSize});
            tBox.setPosition({tx, ty});

            if (tr.type == 0) { // JumpPad
                tBox.setFillColor(sf::Color(255, 170, 0, 140));
                tBox.setOutlineColor(sf::Color(255, 200, 50, 240));
            } else if (tr.type == 1) { // Spikes
                tBox.setFillColor(sf::Color(230, 40, 40, 140));
                tBox.setOutlineColor(sf::Color(255, 80, 80, 240));
            } else { // TriggerZone
                tBox.setFillColor(sf::Color(160, 60, 255, 100));
                tBox.setOutlineColor(sf::Color(190, 100, 255, 240));
            }
            tBox.setOutlineThickness(2.0f);
            _window.draw(tBox);

            if (_fontLoaded) {
                std::string label = (tr.type == 0) ? "^PAD^" : (tr.type == 1) ? "^SPIKE^" : "TRIG";
                sf::Text tTxt(_font, label, 11);
                tTxt.setFillColor(sf::Color::White);
                tTxt.setPosition({tx + 6.0f, ty + 20.0f});
                _window.draw(tTxt);
            }
        }
    }

    // 5. Тестовый чекпоинт (Test Start Pos)
    if (_testSpawn) {
        float sx = static_cast<float>(_testSpawn->x) * _tileSize;
        float sy = static_cast<float>(_testSpawn->y) * _tileSize;

        sf::RectangleShape sBox({_tileSize, _tileSize});
        sBox.setPosition({sx, sy});
        sBox.setFillColor(sf::Color(255, 190, 0, 130));
        sBox.setOutlineColor(sf::Color(255, 220, 0, 255));
        sBox.setOutlineThickness(3.0f);
        _window.draw(sBox);

        if (_fontLoaded) {
            sf::Text sTxt(_font, ">TEST<", 12);
            sTxt.setFillColor(sf::Color(255, 235, 50));
            sTxt.setPosition({sx + 6.0f, sy + 20.0f});
            _window.draw(sTxt);
        }
    }

    // 6. Спавн игрока
    sf::RectangleShape pBox({_tileSize, _tileSize});
    pBox.setPosition({static_cast<float>(_playerSpawn.x) * _tileSize, static_cast<float>(_playerSpawn.y) * _tileSize});
    pBox.setFillColor(sf::Color(0, 230, 118, 70));
    pBox.setOutlineColor(sf::Color(0, 230, 118, 255));
    pBox.setOutlineThickness(2.0f);
    _window.draw(pBox);

    // 7. Спавны врагов
    for (size_t i = 0; i < _enemySpawns.size(); ++i) {
        const auto& es = _enemySpawns[i];
        float bw = (es.type == 1) ? _tileSize * 1.5f : _tileSize;
        float bh = (es.type == 1) ? _tileSize * 1.5f : _tileSize;
        float bx = (es.type == 1) ? (static_cast<float>(es.x) * _tileSize - _tileSize * 0.25f) : (static_cast<float>(es.x) * _tileSize);
        float by = (es.type == 1) ? (static_cast<float>(es.y) * _tileSize - _tileSize * 0.5f) : (static_cast<float>(es.y) * _tileSize);

        sf::RectangleShape eBox({bw, bh});
        eBox.setPosition({bx, by});

        sf::Color fillCol, outCol, txtCol;
        std::string typePrefix;
        if (es.type == 1) { // TANK
            fillCol = sf::Color(220, 40, 40, 75);
            outCol = sf::Color(255, 60, 60, 240);
            txtCol = sf::Color(255, 120, 120);
            typePrefix = "Tank#";
        } else if (es.type == 2) { // DUELIST AGGRESSIVE
            fillCol = sf::Color(170, 70, 230, 75);
            outCol = sf::Color(190, 90, 255, 240);
            txtCol = sf::Color(220, 150, 255);
            typePrefix = "D-Agr#";
        } else if (es.type == 3) { // DUELIST PASSIVE
            fillCol = sf::Color(35, 150, 225, 75);
            outCol = sf::Color(45, 185, 245, 240);
            txtCol = sf::Color(100, 220, 255);
            typePrefix = "D-Pas#";
        } else if (es.type == 4) { // DUMMY
            fillCol = sf::Color(220, 175, 75, 80);
            outCol = sf::Color(255, 215, 0, 245);
            txtCol = sf::Color(255, 235, 120);
            typePrefix = "Dummy#";
        } else { // NORMAL
            fillCol = sf::Color(150, 150, 150, 75);
            outCol = sf::Color(210, 210, 210, 240);
            txtCol = sf::Color(230, 230, 230);
            typePrefix = "Norm#";
        }

        eBox.setFillColor(fillCol);
        eBox.setOutlineColor(outCol);
        eBox.setOutlineThickness(2.0f);
        _window.draw(eBox);

        if (_fontLoaded) {
            sf::Text eT(_font, typePrefix + std::to_string(i + 1), 13);
            eT.setFillColor(txtCol);
            eT.setPosition(eBox.getPosition() + sf::Vector2f(6.0f, 10.0f));
            _window.draw(eT);
        }
    }

    if (_fontLoaded) {
        sf::Text pT(_font, "P", 18);
        pT.setFillColor(sf::Color(0, 230, 118));
        pT.setPosition(pBox.getPosition() + sf::Vector2f(22.0f, 18.0f));
        _window.draw(pT);
    }

    // 8. Превью Box Fill
    if (_boxDragActive) {
        int x1 = std::min(_boxStartTile.x, _boxCurTile.x);
        int x2 = std::max(_boxStartTile.x, _boxCurTile.x);
        int y1 = std::min(_boxStartTile.y, _boxCurTile.y);
        int y2 = std::max(_boxStartTile.y, _boxCurTile.y);
        float bw = (x2 - x1 + 1) * _tileSize;
        float bh = (y2 - y1 + 1) * _tileSize;

        sf::RectangleShape boxRect({bw, bh});
        boxRect.setPosition({x1 * _tileSize, y1 * _tileSize});
        if (_boxIsErase) {
            boxRect.setFillColor(sf::Color(240, 40, 40, 85));
            boxRect.setOutlineColor(sf::Color(255, 60, 60, 255));
        } else {
            boxRect.setFillColor(sf::Color(76, 175, 80, 85));
            boxRect.setOutlineColor(sf::Color(100, 255, 120, 255));
        }
        boxRect.setOutlineThickness(2.0f);
        _window.draw(boxRect);
    }

    // 9. Превью Line Tool
    if (_lineDragActive) {
        auto tiles = getEditorLineTiles(_lineStartTile.x, _lineStartTile.y, _lineCurTile.x, _lineCurTile.y);
        for (const auto& pt : tiles) {
            sf::RectangleShape lineBlock({_tileSize, _tileSize});
            lineBlock.setPosition({pt.x * _tileSize, pt.y * _tileSize});
            lineBlock.setFillColor(sf::Color(33, 150, 243, 90));
            lineBlock.setOutlineColor(sf::Color(100, 200, 255, 230));
            lineBlock.setOutlineThickness(1.5f);
            _window.draw(lineBlock);
        }
    }

    // 10. Превью Selection
    if (_selectionActive) {
        int x1 = std::min(_selStart.x, _selEnd.x);
        int x2 = std::max(_selStart.x, _selEnd.x);
        int y1 = std::min(_selStart.y, _selEnd.y);
        int y2 = std::max(_selStart.y, _selEnd.y);
        float sw = (x2 - x1 + 1) * _tileSize;
        float sh = (y2 - y1 + 1) * _tileSize;

        sf::RectangleShape selRect({sw, sh});
        selRect.setPosition({x1 * _tileSize, y1 * _tileSize});
        selRect.setFillColor(sf::Color(255, 215, 0, 45));
        selRect.setOutlineColor(sf::Color(255, 215, 0, 240));
        selRect.setOutlineThickness(2.0f);
        _window.draw(selRect);
    }

    // 11. Превью Paste
    if (_pasteModeActive && _clipboard.hasData) {
        for (const auto& item : _clipboard.items) {
            int tx = _lastHoverTile.x + item.relX;
            int ty = _lastHoverTile.y + item.relY;
            sf::RectangleShape pTile({_tileSize, _tileSize});
            pTile.setPosition({tx * _tileSize, ty * _tileSize});
            if (item.tile == 1) pTile.setFillColor(sf::Color(76, 175, 80, 140));
            else if (item.tile == 2) pTile.setFillColor(sf::Color(90, 110, 140, 140));
            else if (item.doorTarget > 0) pTile.setFillColor(sf::Color(0, 229, 255, 140));
            else pTile.setFillColor(sf::Color(100, 100, 100, 60));
            pTile.setOutlineColor(sf::Color(255, 255, 255, 200));
            pTile.setOutlineThickness(1.0f);
            _window.draw(pTile);
        }
    }

    // 12. Линейка прыжка
    drawJumpArc();

    // 13. Подсветка кисти
    if (!_boxDragActive && !_lineDragActive && !_pasteModeActive &&
        _lastHoverTile.x >= 0 && _lastHoverTile.x < _width &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < _height) {
        int half = _brushSize / 2;
        float bW = static_cast<float>(_brushSize) * _tileSize;
        float bH = static_cast<float>(_brushSize) * _tileSize;
        float bX = static_cast<float>(_lastHoverTile.x - half) * _tileSize;
        float bY = static_cast<float>(_lastHoverTile.y - half) * _tileSize;

        sf::RectangleShape brushRect({bW, bH});
        brushRect.setPosition({bX, bY});
        brushRect.setOutlineThickness(2.0f);

        if (_currentTool == ToolType::PLATFORM) {
            brushRect.setFillColor(sf::Color(76, 175, 80, 80));
            brushRect.setOutlineColor(sf::Color(76, 175, 80, 240));
        } else if (_currentTool == ToolType::WALL) {
            brushRect.setFillColor(sf::Color(70, 85, 110, 80));
            brushRect.setOutlineColor(sf::Color(100, 120, 160, 240));
        } else if (_currentTool == ToolType::DOOR) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(0, 229, 255, 90));
            brushRect.setOutlineColor(sf::Color(0, 255, 255, 255));
        } else if (_currentTool == ToolType::TEST_SPAWN) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(255, 200, 0, 100));
            brushRect.setOutlineColor(sf::Color(255, 220, 0, 255));
        } else if (_currentTool == ToolType::PLAYER_SPAWN) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(0, 230, 118, 90));
            brushRect.setOutlineColor(sf::Color(0, 230, 118, 255));
        } else if (_currentTool == ToolType::ENEMY_SPAWN) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(160, 160, 160, 90));
            brushRect.setOutlineColor(sf::Color(210, 210, 210, 255));
        } else if (_currentTool == ToolType::ENEMY_TANK) {
            brushRect.setSize({_tileSize * 1.5f, _tileSize * 1.5f});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize - _tileSize * 0.25f, static_cast<float>(_lastHoverTile.y) * _tileSize - _tileSize * 0.5f});
            brushRect.setFillColor(sf::Color(255, 50, 50, 90));
            brushRect.setOutlineColor(sf::Color(255, 80, 80, 255));
        } else if (_currentTool == ToolType::ENEMY_DUELIST) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(170, 70, 230, 90));
            brushRect.setOutlineColor(sf::Color(200, 90, 255, 255));
        } else if (_currentTool == ToolType::ENEMY_DUELIST_PASSIVE) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(35, 150, 225, 90));
            brushRect.setOutlineColor(sf::Color(45, 185, 245, 255));
        } else if (_currentTool == ToolType::EDGE_PASSAGE) {
            brushRect.setFillColor(sf::Color(220, 50, 50, 60));
            brushRect.setOutlineColor(sf::Color(255, 60, 60, 220));
        } else if (_currentTool == ToolType::JUMP_PAD) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(255, 170, 0, 90));
            brushRect.setOutlineColor(sf::Color(255, 200, 50, 255));
        } else if (_currentTool == ToolType::SPIKES) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(230, 40, 40, 90));
            brushRect.setOutlineColor(sf::Color(255, 80, 80, 255));
        } else if (_currentTool == ToolType::TRIGGER_ZONE) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(160, 60, 255, 90));
            brushRect.setOutlineColor(sf::Color(190, 100, 255, 255));
        } else if (_currentTool == ToolType::FLOOD_FILL) {
            brushRect.setSize({_tileSize, _tileSize});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * _tileSize, static_cast<float>(_lastHoverTile.y) * _tileSize});
            brushRect.setFillColor(sf::Color(255, 180, 0, 90));
            brushRect.setOutlineColor(sf::Color(255, 200, 0, 255));
        }
        _window.draw(brushRect);
    }
}

void EditorApp::drawModal() {
    if (!_modal.active) return;

    float mx = _modal.screenPos.x;
    float my = _modal.screenPos.y;
    float mw = 280.0f;
    float mh = 220.0f;

    sf::RectangleShape bg({mw, mh});
    bg.setPosition({mx, my});
    bg.setFillColor(sf::Color(14, 18, 28, 245));
    bg.setOutlineColor(sf::Color(255, 215, 0, 240));
    bg.setOutlineThickness(2.0f);
    _window.draw(bg);

    // Заголовок модального окна
    sf::RectangleShape header({mw, 30.0f});
    header.setPosition({mx, my});
    header.setFillColor(sf::Color(24, 32, 48));
    _window.draw(header);

    if (_fontLoaded) {
        std::string titleStr = "[*] НАСТРОЙКИ ОБЪЕКТА";
        if (_modal.type == SettingsModal::TargetType::DOOR) titleStr = "[D] НАСТРОЙКА ДВЕРИ";
        else if (_modal.type == SettingsModal::TargetType::TEST_SPAWN) titleStr = "[▶] ТЕСТОВЫЙ ЧЕКПОИНТ";
        else if (_modal.type == SettingsModal::TargetType::ENEMY) titleStr = "[E] НАСТРОЙКА ВРАГА";
        else if (_modal.type == SettingsModal::TargetType::TRIGGER) titleStr = "[⚡] МЕХАНИКА / ТРИГГЕР";
        else if (_modal.type == SettingsModal::TargetType::TILE) titleStr = "[#] НАСТРОЙКА БЛОКА";

        sf::Text tText(_font, toSfString(titleStr), 13);
        tText.setFillColor(sf::Color(255, 215, 0));
        tText.setPosition({mx + 10.0f, my + 6.0f});
        _window.draw(tText);

        sf::Text xBtn(_font, "X", 14);
        xBtn.setFillColor(sf::Color(255, 80, 80));
        xBtn.setPosition({mx + mw - 20.0f, my + 5.0f});
        _window.draw(xBtn);

        // 1. ДВЕРЬ
        if (_modal.type == SettingsModal::TargetType::DOOR) {
            sf::Text sub(_font, toSfString("Выберите целевую комнату:"), 11);
            sub.setFillColor(sf::Color(160, 190, 220));
            sub.setPosition({mx + 16.0f, my + 34.0f});
            _window.draw(sub);

            int curTgt = (_modal.targetIndex >= 0 && _modal.targetIndex < static_cast<int>(_doors.size())) ? _doors[_modal.targetIndex].targetRoom : 1;

            for (int r = 1; r <= 10; ++r) {
                int col = (r - 1) % 5;
                int row = (r - 1) / 5;
                float bx = mx + 16.0f + col * 48.0f;
                float by = my + 52.0f + row * 34.0f;
                bool isCur = (curTgt == r);

                sf::RectangleShape rBox({42.0f, 28.0f});
                rBox.setPosition({bx, by});
                rBox.setFillColor(isCur ? sf::Color(0, 200, 255) : sf::Color(25, 36, 50));
                rBox.setOutlineColor(isCur ? sf::Color::White : sf::Color(60, 80, 110));
                rBox.setOutlineThickness(1.0f);
                _window.draw(rBox);

                sf::Text rT(_font, "R" + std::to_string(r), 11);
                rT.setFillColor(isCur ? sf::Color(10, 15, 25) : sf::Color(180, 210, 230));
                rT.setPosition({bx + 8.0f, by + 6.0f});
                _window.draw(rT);
            }

            sf::RectangleShape dBtn({mw - 32.0f, 28.0f});
            dBtn.setPosition({mx + 16.0f, my + 140.0f});
            dBtn.setFillColor(sf::Color(80, 25, 25));
            dBtn.setOutlineColor(sf::Color(255, 80, 80));
            dBtn.setOutlineThickness(1.0f);
            _window.draw(dBtn);

            sf::Text dTxt(_font, toSfString("Удалить дверь"), 12);
            dTxt.setFillColor(sf::Color(255, 120, 120));
            dTxt.setPosition({mx + 85.0f, my + 145.0f});
            _window.draw(dTxt);
        }

        // 2. ТЕСТОВЫЙ ЧЕКПОИНТ
        if (_modal.type == SettingsModal::TargetType::TEST_SPAWN) {
            sf::RectangleShape playBox({mw - 32.0f, 44.0f});
            playBox.setPosition({mx + 16.0f, my + 45.0f});
            playBox.setFillColor(sf::Color(20, 130, 60));
            playBox.setOutlineColor(sf::Color(50, 255, 120));
            playBox.setOutlineThickness(2.0f);
            _window.draw(playBox);

            sf::Text pTxt(_font, toSfString("▶ ЗАПУСК СИМУЛЯЦИИ"), 13);
            pTxt.setFillColor(sf::Color::White);
            pTxt.setStyle(sf::Text::Bold);
            pTxt.setPosition({mx + 38.0f, my + 56.0f});
            _window.draw(pTxt);

            sf::RectangleShape dBtn({mw - 32.0f, 28.0f});
            dBtn.setPosition({mx + 16.0f, my + 115.0f});
            dBtn.setFillColor(sf::Color(80, 25, 25));
            dBtn.setOutlineColor(sf::Color(255, 80, 80));
            dBtn.setOutlineThickness(1.0f);
            _window.draw(dBtn);

            sf::Text dTxt(_font, toSfString("Удалить тестовую точку"), 12);
            dTxt.setFillColor(sf::Color(255, 120, 120));
            dTxt.setPosition({mx + 55.0f, my + 120.0f});
            _window.draw(dTxt);
        }

        // 3. ВРАГ
        if (_modal.type == SettingsModal::TargetType::ENEMY) {
            int curType = (_modal.targetIndex >= 0 && _modal.targetIndex < static_cast<int>(_enemySpawns.size())) ? _enemySpawns[_modal.targetIndex].type : 0;
            const char* names[4] = {"Обычный (45 HP)", "Танк (110 HP)", "Дуэль-Агр (65 HP)", "Дуэль-Пасс (65 HP)"};

            for (int a = 0; a < 4; ++a) {
                float bx = mx + 16.0f + (a % 2) * 126.0f;
                float by = my + 38.0f + (a / 2) * 30.0f;
                bool isCur = (curType == a);

                sf::RectangleShape aBox({120.0f, 26.0f});
                aBox.setPosition({bx, by});
                aBox.setFillColor(isCur ? sf::Color(160, 50, 220) : sf::Color(25, 34, 48));
                aBox.setOutlineColor(isCur ? sf::Color(255, 215, 0) : sf::Color(60, 75, 100));
                aBox.setOutlineThickness(1.0f);
                _window.draw(aBox);

                sf::Text aTxt(_font, toSfString(names[a]), 9);
                aTxt.setFillColor(isCur ? sf::Color::White : sf::Color(180, 205, 230));
                aTxt.setPosition({bx + 6.0f, by + 6.0f});
                _window.draw(aTxt);
            }

            // Кнопка 5: Манекен (Dummy)
            bool isDummy = (curType == 4);
            sf::RectangleShape dummyBox({mw - 32.0f, 26.0f});
            dummyBox.setPosition({mx + 16.0f, my + 100.0f});
            dummyBox.setFillColor(isDummy ? sf::Color(215, 165, 35) : sf::Color(25, 34, 48));
            dummyBox.setOutlineColor(isDummy ? sf::Color(255, 240, 100) : sf::Color(60, 75, 100));
            dummyBox.setOutlineThickness(1.0f);
            _window.draw(dummyBox);

            sf::Text dummyTxt(_font, toSfString("Манекен / Dummy (Бессмертный тренажер)"), 10);
            dummyTxt.setFillColor(isDummy ? sf::Color(20, 15, 5) : sf::Color(255, 215, 80));
            dummyTxt.setPosition({mx + 22.0f, my + 105.0f});
            _window.draw(dummyTxt);

            sf::RectangleShape dBtn({mw - 32.0f, 26.0f});
            dBtn.setPosition({mx + 16.0f, my + 132.0f});
            dBtn.setFillColor(sf::Color(80, 25, 25));
            dBtn.setOutlineColor(sf::Color(255, 80, 80));
            dBtn.setOutlineThickness(1.0f);
            _window.draw(dBtn);

            sf::Text dTxt(_font, toSfString("Удалить врага"), 12);
            dTxt.setFillColor(sf::Color(255, 120, 120));
            dTxt.setPosition({mx + 85.0f, my + 136.0f});
            _window.draw(dTxt);
        }

        // 4. ТРИГГЕРЫ
        if (_modal.type == SettingsModal::TargetType::TRIGGER) {
            sf::RectangleShape b1({120.0f, 30.0f});
            b1.setPosition({mx + 16.0f, my + 42.0f});
            b1.setFillColor(sf::Color(255, 150, 0, 160));
            b1.setOutlineColor(sf::Color(255, 200, 50));
            b1.setOutlineThickness(1.0f);
            _window.draw(b1);
            sf::Text t1(_font, toSfString("Батут (Pad)"), 11);
            t1.setFillColor(sf::Color::White);
            t1.setPosition({mx + 30.0f, my + 50.0f});
            _window.draw(t1);

            sf::RectangleShape b2({120.0f, 30.0f});
            b2.setPosition({mx + 142.0f, my + 42.0f});
            b2.setFillColor(sf::Color(230, 40, 40, 160));
            b2.setOutlineColor(sf::Color(255, 80, 80));
            b2.setOutlineThickness(1.0f);
            _window.draw(b2);
            sf::Text t2(_font, toSfString("Шипы (Spikes)"), 11);
            t2.setFillColor(sf::Color::White);
            t2.setPosition({mx + 152.0f, my + 50.0f});
            _window.draw(t2);

            sf::RectangleShape dBtn({mw - 32.0f, 28.0f});
            dBtn.setPosition({mx + 16.0f, my + 120.0f});
            dBtn.setFillColor(sf::Color(80, 25, 25));
            dBtn.setOutlineColor(sf::Color(255, 80, 80));
            dBtn.setOutlineThickness(1.0f);
            _window.draw(dBtn);
            sf::Text dTxt(_font, toSfString("Удалить объект"), 12);
            dTxt.setFillColor(sf::Color(255, 120, 120));
            dTxt.setPosition({mx + 80.0f, my + 125.0f});
            _window.draw(dTxt);
        }

        // 5. ТАЙЛ
        if (_modal.type == SettingsModal::TargetType::TILE) {
            sf::RectangleShape bWall({mw - 32.0f, 28.0f});
            bWall.setPosition({mx + 16.0f, my + 42.0f});
            bWall.setFillColor(sf::Color(70, 85, 110));
            _window.draw(bWall);
            sf::Text wTxt(_font, toSfString("Сделать стеной (#)"), 11);
            wTxt.setFillColor(sf::Color::White);
            wTxt.setPosition({mx + 70.0f, my + 48.0f});
            _window.draw(wTxt);

            sf::RectangleShape bPlat({mw - 32.0f, 28.0f});
            bPlat.setPosition({mx + 16.0f, my + 78.0f});
            bPlat.setFillColor(sf::Color(76, 175, 80));
            _window.draw(bPlat);
            sf::Text pTxt(_font, toSfString("Сделать платформой (=)"), 11);
            pTxt.setFillColor(sf::Color::White);
            pTxt.setPosition({mx + 55.0f, my + 84.0f});
            _window.draw(pTxt);

            sf::RectangleShape bErase({mw - 32.0f, 28.0f});
            bErase.setPosition({mx + 16.0f, my + 114.0f});
            bErase.setFillColor(sf::Color(80, 25, 25));
            _window.draw(bErase);
            sf::Text eTxt(_font, toSfString("Стереть блок (Ластик)"), 11);
            eTxt.setFillColor(sf::Color(255, 120, 120));
            eTxt.setPosition({mx + 60.0f, my + 120.0f});
            _window.draw(eTxt);
        }
    }
}

void EditorApp::drawRoomBrowser() {
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);

    // Затемнение фонового мира
    sf::RectangleShape shade({winW, winH});
    shade.setFillColor(sf::Color(8, 12, 18, 175));
    _window.draw(shade);

    float panelW = std::min(winW - 60.0f, 960.0f);
    float panelH = winH - 160.0f;
    float panelX = (winW - panelW) / 2.0f;
    float panelY = 46.0f;

    // Главная панель браузера комнат (основной бг менюшки: sf::Color(18, 24, 34))
    sf::RectangleShape panel({panelW, panelH});
    panel.setPosition({panelX, panelY});
    panel.setFillColor(sf::Color(18, 24, 34, 250));
    panel.setOutlineColor(sf::Color(65, 85, 120, 240));
    panel.setOutlineThickness(2.0f);
    _window.draw(panel);

    // Заголовок панели
    sf::RectangleShape headerBox({panelW, 44.0f});
    headerBox.setPosition({panelX, panelY});
    headerBox.setFillColor(sf::Color(25, 33, 46, 255));
    _window.draw(headerBox);

    // Кнопка [ + Создать новую комнату ]
    sf::RectangleShape newBtn({210.0f, 32.0f});
    newBtn.setPosition({panelX + panelW - 225.0f, panelY + 6.0f});
    sf::Vector2i mPixel = sf::Mouse::getPosition(_window);
    sf::FloatRect newBtnRect({panelX + panelW - 225.0f, panelY + 6.0f}, {210.0f, 32.0f});
    bool hoverNew = newBtnRect.contains(sf::Vector2f(mPixel));
    newBtn.setFillColor(hoverNew ? sf::Color(35, 145, 80) : sf::Color(25, 110, 60));
    newBtn.setOutlineColor(hoverNew ? sf::Color(120, 255, 160) : sf::Color(60, 220, 110));
    newBtn.setOutlineThickness(1.5f);
    _window.draw(newBtn);

    if (_fontLoaded) {
        sf::Text titleTxt(_font, toSfString("📁 ВЫБОР И УПРАВЛЕНИЕ КОМНАТАМИ"), 14);
        titleTxt.setStyle(sf::Text::Bold);
        titleTxt.setFillColor(sf::Color(255, 215, 0));
        titleTxt.setPosition({panelX + 16.0f, panelY + 12.0f});
        _window.draw(titleTxt);

        std::string countStr = "Всего: " + std::to_string(_roomList.size()) + " комнат";
        sf::Text countTxt(_font, toSfString(countStr), 11);
        countTxt.setFillColor(sf::Color(140, 175, 210));
        countTxt.setPosition({panelX + 310.0f, panelY + 15.0f});
        _window.draw(countTxt);

        sf::Text btnTxt(_font, toSfString("+ Создать комнату"), 12);
        btnTxt.setStyle(sf::Text::Bold);
        btnTxt.setFillColor(sf::Color::White);
        btnTxt.setPosition({panelX + panelW - 195.0f, panelY + 12.0f});
        _window.draw(btnTxt);

        // Подсказка навигации
        sf::Text hintTxt(_font, toSfString("Навигация: Стрелки [↑][↓][←][→] | Выбор: [Enter] или [ЛКМ] | Создать: [N] | Закрыть: [Esc]"), 11);
        hintTxt.setFillColor(sf::Color(160, 185, 215));
        hintTxt.setPosition({panelX + 16.0f, panelY + 49.0f});
        _window.draw(hintTxt);
    }

    float contentX = panelX + 16.0f;
    float contentY = panelY + 70.0f;
    float contentW = panelW - 32.0f;
    float contentH = panelH - 78.0f;
    float colGap = 16.0f;
    float cardW = (contentW - colGap) / 2.0f;
    float cardH = 145.0f;
    float rowGap = 16.0f;

    int totalRows = (static_cast<int>(_roomList.size()) + 1) / 2;
    float totalContentHeight = totalRows * (cardH + rowGap);
    float maxScroll = std::max(0.0f, totalContentHeight - contentH);
    _roomBrowserScrollY = std::clamp(_roomBrowserScrollY, 0.0f, maxScroll);

    // Авто-скролл к выбранной стрелками карточке
    if (_roomBrowserSelected >= 0 && _roomBrowserSelected < static_cast<int>(_roomList.size())) {
        int selRow = _roomBrowserSelected / 2;
        float selTop = selRow * (cardH + rowGap);
        float selBottom = selTop + cardH;
        if (selTop < _roomBrowserScrollY) _roomBrowserScrollY = selTop;
        if (selBottom > _roomBrowserScrollY + contentH) _roomBrowserScrollY = selBottom - contentH;
    }

    // Отрисовка двух столбцов карточек
    for (size_t i = 0; i < _roomList.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        float cx = contentX + col * (cardW + colGap);
        float cy = contentY + row * (cardH + rowGap) - _roomBrowserScrollY;

        if (cy + cardH < contentY || cy > contentY + contentH) continue;

        sf::FloatRect cardRect({cx, cy}, {cardW, cardH});
        if (cardRect.contains(sf::Vector2f(mPixel))) {
            _roomBrowserSelected = static_cast<int>(i);
        }

        bool isSelected = (static_cast<int>(i) == _roomBrowserSelected);
        bool isCurrent = (_roomList[i].id == _currentRoomName);

        // Карточка комнаты:
        // "так же наведенная комната выделяется более светлым бг чем основной бг менюшки"
        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cx, cy});
        if (isSelected) {
            // Светлый фон при наведении (заметно светлее основного sf::Color(18, 24, 34))
            card.setFillColor(sf::Color(52, 75, 112, 255));
            card.setOutlineColor(sf::Color(255, 215, 0));
            card.setOutlineThickness(2.0f);
        } else {
            // Обычный фон карточки
            card.setFillColor(sf::Color(26, 34, 48, 240));
            card.setOutlineColor(sf::Color(50, 68, 95, 200));
            card.setOutlineThickness(1.0f);
        }
        _window.draw(card);

        // 1. Превью комнаты (Thumbnail)
        float prevX = cx + 8.0f;
        float prevY = cy + 8.0f;
        float prevW = cardW - 16.0f;
        float prevH = 94.0f;

        sf::RectangleShape prevBg({prevW, prevH});
        prevBg.setPosition({prevX, prevY});
        prevBg.setFillColor(sf::Color(12, 16, 24));
        prevBg.setOutlineColor(sf::Color(40, 52, 70));
        prevBg.setOutlineThickness(1.0f);
        _window.draw(prevBg);

        if (_roomList[i].thumbnail && _roomList[i].thumbnail->getSize().x > 0) {
            sf::Sprite spr(*_roomList[i].thumbnail);
            float scaleX = prevW / static_cast<float>(_roomList[i].thumbnail->getSize().x);
            float scaleY = prevH / static_cast<float>(_roomList[i].thumbnail->getSize().y);
            float s = std::min(scaleX, scaleY);
            spr.setScale({s, s});
            float sw = static_cast<float>(_roomList[i].thumbnail->getSize().x) * s;
            float sh = static_cast<float>(_roomList[i].thumbnail->getSize().y) * s;
            spr.setPosition({prevX + (prevW - sw) / 2.0f, prevY + (prevH - sh) / 2.0f});
            _window.draw(spr);
        } else if (_fontLoaded) {
            sf::Text noPrev(_font, toSfString("[ Превью карты .png ]"), 10);
            noPrev.setFillColor(sf::Color(100, 125, 155));
            noPrev.setPosition({prevX + prevW / 2.0f - 55.0f, prevY + prevH / 2.0f - 6.0f});
            _window.draw(noPrev);
        }

        // Маркер [ ТЕКУЩАЯ ] в правом верхнем углу превью
        if (isCurrent) {
            sf::RectangleShape curBadge({85.0f, 18.0f});
            curBadge.setPosition({prevX + prevW - 88.0f, prevY + 4.0f});
            curBadge.setFillColor(sf::Color(25, 135, 65, 230));
            curBadge.setOutlineColor(sf::Color(80, 255, 140));
            curBadge.setOutlineThickness(1.0f);
            _window.draw(curBadge);

            if (_fontLoaded) {
                sf::Text curTxt(_font, toSfString("ТЕКУЩАЯ"), 9);
                curTxt.setStyle(sf::Text::Bold);
                curTxt.setFillColor(sf::Color::White);
                curTxt.setPosition({prevX + prevW - 74.0f, prevY + 6.0f});
                _window.draw(curTxt);
            }
        }

        // 2. Название комнаты снизу под фото превью
        if (_fontLoaded) {
            sf::Text nameTxt(_font, toSfString(_roomList[i].title), 12);
            nameTxt.setStyle(sf::Text::Bold);
            nameTxt.setFillColor(isSelected ? sf::Color(255, 235, 90) : sf::Color(230, 238, 250));
            nameTxt.setPosition({cx + 10.0f, cy + 107.0f});
            _window.draw(nameTxt);

            std::string subInfo = _roomList[i].id;
            if (_roomList[i].width > 0) {
                subInfo += " [" + std::to_string(_roomList[i].width) + "x" + std::to_string(_roomList[i].height) + "]";
            }
            sf::Text subTxt(_font, toSfString(subInfo), 10);
            subTxt.setFillColor(isSelected ? sf::Color(210, 225, 255) : sf::Color(130, 150, 175));
            subTxt.setPosition({cx + 10.0f, cy + 125.0f});
            _window.draw(subTxt);
        }
    }

    // Модальное окно создания и наименования новой комнаты
    if (_isNamingRoom) {
        sf::RectangleShape modalShade({winW, winH});
        modalShade.setFillColor(sf::Color(0, 0, 0, 180));
        _window.draw(modalShade);

        float mw = 460.0f;
        float mh = 180.0f;
        float mx = (winW - mw) / 2.0f;
        float my = (winH - mh) / 2.0f;

        sf::RectangleShape mBox({mw, mh});
        mBox.setPosition({mx, my});
        mBox.setFillColor(sf::Color(20, 26, 38, 255));
        mBox.setOutlineColor(sf::Color(0, 215, 255));
        mBox.setOutlineThickness(2.5f);
        _window.draw(mBox);

        if (_fontLoaded) {
            sf::Text mTitle(_font, toSfString("[ СОЗДАНИЕ И ИМЕНОВАНИЕ НОВОЙ КОМНАТЫ ]"), 13);
            mTitle.setStyle(sf::Text::Bold);
            mTitle.setFillColor(sf::Color(255, 215, 0));
            mTitle.setPosition({mx + 18.0f, my + 14.0f});
            _window.draw(mTitle);

            sf::Text mPrompt(_font, toSfString("Введите имя комнаты (например: boss_arena, cave_1):"), 11);
            mPrompt.setFillColor(sf::Color(180, 205, 230));
            mPrompt.setPosition({mx + 18.0f, my + 42.0f});
            _window.draw(mPrompt);

            // Поле ввода текста
            sf::RectangleShape inputField({mw - 36.0f, 38.0f});
            inputField.setPosition({mx + 18.0f, my + 68.0f});
            inputField.setFillColor(sf::Color(12, 16, 24));
            inputField.setOutlineColor(sf::Color(0, 200, 255));
            inputField.setOutlineThickness(1.5f);
            _window.draw(inputField);

            std::string displayText = _newRoomNameInput + "_";
            sf::Text inTxt(_font, toSfString(displayText), 14);
            inTxt.setStyle(sf::Text::Bold);
            inTxt.setFillColor(sf::Color(100, 240, 255));
            inTxt.setPosition({mx + 28.0f, my + 77.0f});
            _window.draw(inTxt);

            sf::Text mHint(_font, toSfString("[ Enter ] — Создать и открыть  |  [ Esc ] — Отмена"), 11);
            mHint.setFillColor(sf::Color(140, 165, 195));
            mHint.setPosition({mx + 60.0f, my + 130.0f});
            _window.draw(mHint);
        }
    }
}

void EditorApp::drawSimulationHUD() {
    float winW = static_cast<float>(_window.getSize().x);

    // Верхняя стильная плашка симуляции
    sf::RectangleShape banner({winW, 36.0f});
    banner.setPosition({0.0f, 0.0f});
    banner.setFillColor(sf::Color(180, 25, 25, 230));
    banner.setOutlineColor(sf::Color(255, 80, 80));
    banner.setOutlineThickness(1.0f);
    _window.draw(banner);

    if (_fontLoaded) {
        std::string simStr = "<< РЕЖИМ СИМУЛЯЦИИ ИГРЫ >> Нажмите [Tab] или [Esc] для возврата в редактор";
        sf::Text txt(_font, toSfString(simStr), 15);
        txt.setFillColor(sf::Color::White);
        txt.setStyle(sf::Text::Bold);
        sf::FloatRect b = txt.getLocalBounds();
        txt.setPosition({(winW - b.size.x) / 2.0f, 8.0f});
        _window.draw(txt);
    }
}

void EditorApp::drawUI() {
    float winW = static_cast<float>(_window.getSize().x);
    float winH = static_cast<float>(_window.getSize().y);

    // 1. Верхняя панель
    sf::RectangleShape bar({winW, 118.0f});
    bar.setPosition({0.0f, 0.0f});
    bar.setFillColor(sf::Color(16, 20, 28, 245));
    bar.setOutlineColor(sf::Color(65, 80, 105, 200));
    bar.setOutlineThickness(1.0f);
    _window.draw(bar);

    if (_fontLoaded) {
        sf::Text title(_font, toSfString("[ STANDALONE РЕДАКТОР КАРТ ]  (Tab: Запуск симуляции | ПКМ: Настройки объекта | Space+Drag: Перемещение)"), 15);
        title.setFillColor(sf::Color(255, 215, 0));
        title.setPosition({20.0f, 6.0f});
        _window.draw(title);

        std::string roomInfo = "<< [PageDown] Комната " + std::to_string(_currentRoomIndex) + 
                               " / " + std::to_string(_totalRooms) + " [PageUp] >>    " +
                               "Размер: " + std::to_string(_width) + "x" + std::to_string(_height) + " (Alt+Стрелки)    " +
                               "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "% (Z / C / Wheel)";
        sf::Text roomText(_font, toSfString(roomInfo), 13);
        roomText.setFillColor(sf::Color(100, 220, 255));
        roomText.setPosition({20.0f, 30.0f});
        _window.draw(roomText);

        std::string hints = "Инструменты: [B] Кисть | [E] Ластик | [X] Box Fill | [L] Линия | [G] Ведро | [Q] Пипетка | [Tab] Симуляция";
        sf::Text hintText(_font, toSfString(hints), 12);
        hintText.setFillColor(sf::Color(160, 185, 215));
        hintText.setPosition({20.0f, 54.0f});
        _window.draw(hintText);

        std::string extraHints = "Горячие клавиши: Space+Drag: Панорама | ПКМ: Настройки блока | Ctrl+Z/Y: Undo/Redo (" + 
                                std::to_string(_undoStack.size()) + ") | Ctrl+C/V: Буфер | [J] Линейка (" + 
                                std::string(_showJumpRuler ? "ВКЛ" : "ВЫКЛ") + ") | [Ctrl+G] Сетка (" + std::string(_showGrid ? "ВКЛ" : "ВЫКЛ") + ")";
        sf::Text extraHintText(_font, toSfString(extraHints), 12);
        extraHintText.setFillColor(sf::Color(140, 165, 195));
        extraHintText.setPosition({20.0f, 78.0f});
        _window.draw(extraHintText);

        if (!_statusMessage.empty() && _statusTimer > 0.0f) {
            sf::Text statusText(_font, toSfString(_statusMessage), 15);
            statusText.setFillColor(sf::Color(100, 255, 120));
            sf::FloatRect bounds = statusText.getLocalBounds();

            float bgW = bounds.size.x + 36.0f;
            float bgH = bounds.size.y + 16.0f;
            float bgX = (winW - bgW) / 2.0f;
            float bgY = 130.0f;

            sf::RectangleShape msgBg({bgW, bgH});
            msgBg.setPosition({bgX, bgY});
            msgBg.setFillColor(sf::Color(15, 20, 28, 235));
            msgBg.setOutlineColor(sf::Color(100, 255, 120, 220));
            msgBg.setOutlineThickness(1.5f);

            statusText.setPosition({bgX + 18.0f, bgY + 6.0f});
            _window.draw(msgBg);
            _window.draw(statusText);
        }
    }

    // 2. Миникарта
    float mmW = 260.0f;
    float mmH = 140.0f;
    float mmX = winW - mmW - 20.0f;
    float mmY = winH - mmH - 20.0f;
    _minimapRect = sf::FloatRect({mmX, mmY}, {mmW, mmH});

    sf::RectangleShape mmBg({mmW, mmH});
    mmBg.setPosition({mmX, mmY});
    mmBg.setFillColor(sf::Color(12, 16, 24, 235));
    mmBg.setOutlineColor(sf::Color(70, 85, 115, 220));
    mmBg.setOutlineThickness(1.5f);
    _window.draw(mmBg);

    if (_fontLoaded) {
        sf::Text mmTitle(_font, toSfString("МИНИКАРТА (Клик для прыжка)"), 12);
        mmTitle.setFillColor(sf::Color(160, 185, 215));
        mmTitle.setPosition({mmX + 8.0f, mmY + 5.0f});
        _window.draw(mmTitle);
    }

    float mapW = _width * _tileSize;
    float mapH = _height * _tileSize;
    float innerW = mmW - 16.0f;
    float innerH = mmH - 30.0f;
    float scaleX = innerW / mapW;
    float scaleY = innerH / mapH;
    float scale = std::min(scaleX, scaleY);

    float offX = mmX + 8.0f;
    float offY = mmY + 24.0f;

    for (int y = 0; y < _height; ++y) {
        for (int x = 0; x < _width; ++x) {
            int tile = _map[y][x];
            if (tile == 0) continue;

            sf::RectangleShape dot({std::max(1.5f, _tileSize * scale), std::max(1.5f, _tileSize * scale)});
            dot.setPosition({offX + x * _tileSize * scale, offY + y * _tileSize * scale});
            if (tile == 1) dot.setFillColor(sf::Color(76, 175, 80));
            else if (tile == 2) dot.setFillColor(sf::Color(90, 110, 140));
            else if (tile == 3) dot.setFillColor(sf::Color(0, 229, 255));
            _window.draw(dot);
        }
    }

    sf::CircleShape pDot(3.0f);
    pDot.setFillColor(sf::Color(0, 230, 118));
    pDot.setPosition({offX + _playerSpawn.x * _tileSize * scale - 3.0f, offY + _playerSpawn.y * _tileSize * scale - 3.0f});
    _window.draw(pDot);

    if (_testSpawn) {
        sf::CircleShape sDot(4.0f);
        sDot.setFillColor(sf::Color(255, 220, 0));
        sDot.setPosition({offX + _testSpawn->x * _tileSize * scale - 4.0f, offY + _testSpawn->y * _tileSize * scale - 4.0f});
        _window.draw(sDot);
    }

    for (const auto& es : _enemySpawns) {
        sf::CircleShape eDot(es.type == 1 ? 4.0f : 3.0f);
        if (es.type == 1) eDot.setFillColor(sf::Color(255, 60, 60));
        else if (es.type == 2) eDot.setFillColor(sf::Color(190, 90, 255));
        else if (es.type == 3) eDot.setFillColor(sf::Color(45, 185, 245));
        else eDot.setFillColor(sf::Color(190, 190, 190));
        eDot.setPosition({offX + es.x * _tileSize * scale - 3.0f, offY + es.y * _tileSize * scale - 3.0f});
        _window.draw(eDot);
    }

    sf::Vector2f camCenter = _camera.getCenter();
    sf::Vector2f camSize = _camera.getSize();
    float camLeft = camCenter.x - camSize.x / 2.0f;
    float camTop = camCenter.y - camSize.y / 2.0f;

    sf::RectangleShape camFrame({camSize.x * scale, camSize.y * scale});
    camFrame.setPosition({offX + camLeft * scale, offY + camTop * scale});
    camFrame.setFillColor(sf::Color::Transparent);
    camFrame.setOutlineColor(sf::Color(255, 215, 0, 220));
    camFrame.setOutlineThickness(1.5f);
    _window.draw(camFrame);

    // 3. Нижняя панель с вкладками
    float mmW2 = std::min(240.0f, winW * 0.22f);
    float mmX2 = winW - mmW2 - 15.0f;
    float palX = 15.0f;
    float palH = 118.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX2 - palX - 12.0f;

    sf::RectangleShape palBg({palW, palH});
    palBg.setPosition({palX, palY});
    palBg.setFillColor(sf::Color(15, 20, 28, 245));
    palBg.setOutlineColor(sf::Color(65, 80, 110, 220));
    palBg.setOutlineThickness(1.5f);
    _window.draw(palBg);

    // 4. ВКЛАДКИ (TABS)
    float tabW = 150.0f;
    float tabH = 22.0f;
    float tabY = palY + 4.0f;

    // Вкладка 0: Блоки
    bool tab0Active = (_activeTab == PaletteTab::BLOCKS);
    sf::RectangleShape t0Box({tabW, tabH});
    t0Box.setPosition({palX + 10.0f, tabY});
    t0Box.setFillColor(tab0Active ? sf::Color(35, 60, 95) : sf::Color(22, 28, 38));
    t0Box.setOutlineColor(tab0Active ? sf::Color(255, 215, 0) : sf::Color(55, 70, 90));
    t0Box.setOutlineThickness(tab0Active ? 1.5f : 1.0f);
    _window.draw(t0Box);

    // Вкладка 1: Сущности
    bool tab1Active = (_activeTab == PaletteTab::ENTITIES);
    sf::RectangleShape t1Box({tabW, tabH});
    t1Box.setPosition({palX + 168.0f, tabY});
    t1Box.setFillColor(tab1Active ? sf::Color(35, 60, 95) : sf::Color(22, 28, 38));
    t1Box.setOutlineColor(tab1Active ? sf::Color(255, 215, 0) : sf::Color(55, 70, 90));
    t1Box.setOutlineThickness(tab1Active ? 1.5f : 1.0f);
    _window.draw(t1Box);

    // Вкладка 2: Механики
    bool tab2Active = (_activeTab == PaletteTab::MECHANICS);
    sf::RectangleShape t2Box({tabW + 20.0f, tabH});
    t2Box.setPosition({palX + 326.0f, tabY});
    t2Box.setFillColor(tab2Active ? sf::Color(35, 60, 95) : sf::Color(22, 28, 38));
    t2Box.setOutlineColor(tab2Active ? sf::Color(255, 215, 0) : sf::Color(55, 70, 90));
    t2Box.setOutlineThickness(tab2Active ? 1.5f : 1.0f);
    _window.draw(t2Box);

    // Вкладка 3: Комнаты
    bool tab3Active = (_activeTab == PaletteTab::ROOMS);
    sf::RectangleShape t3Box({130.0f, tabH});
    t3Box.setPosition({palX + 504.0f, tabY});
    t3Box.setFillColor(tab3Active ? sf::Color(35, 60, 95) : sf::Color(22, 28, 38));
    t3Box.setOutlineColor(tab3Active ? sf::Color(255, 215, 0) : sf::Color(55, 70, 90));
    t3Box.setOutlineThickness(tab3Active ? 1.5f : 1.0f);
    _window.draw(t3Box);

    // Кнопка [ ▶ СИМУЛЯЦИЯ ]
    sf::RectangleShape simBtn({125.0f, tabH});
    simBtn.setPosition({palX + 642.0f, tabY});
    simBtn.setFillColor(sf::Color(20, 130, 60));
    simBtn.setOutlineColor(sf::Color(60, 255, 120));
    simBtn.setOutlineThickness(1.5f);
    _window.draw(simBtn);

    // Кнопка [Клон в R...]
    sf::RectangleShape btnDup({95.0f, tabH});
    btnDup.setPosition({palX + 775.0f, tabY});
    btnDup.setFillColor(sf::Color(35, 50, 70));
    btnDup.setOutlineColor(sf::Color(0, 200, 255));
    btnDup.setOutlineThickness(1.0f);
    _window.draw(btnDup);

    // Кнопка [Очистить]
    sf::RectangleShape btnClr({75.0f, tabH});
    btnClr.setPosition({palX + 878.0f, tabY});
    btnClr.setFillColor(sf::Color(65, 25, 25));
    btnClr.setOutlineColor(sf::Color(255, 80, 80));
    btnClr.setOutlineThickness(1.0f);
    _window.draw(btnClr);

    // Кнопка [F5 - Обновить]
    float reloadX = std::max(palX + 960.0f, palX + palW - 110.0f);
    sf::RectangleShape btnReload({105.0f, tabH});
    btnReload.setPosition({reloadX, tabY});
    btnReload.setFillColor(sf::Color(25, 40, 60));
    btnReload.setOutlineColor(sf::Color(100, 220, 255));
    btnReload.setOutlineThickness(1.0f);
    _window.draw(btnReload);

    if (_fontLoaded) {
        sf::Text t0Txt(_font, toSfString("[#] БЛОКИ И СТЕНЫ"), 11);
        t0Txt.setFillColor(tab0Active ? sf::Color(255, 215, 0) : sf::Color(160, 185, 210));
        t0Txt.setPosition({palX + 22.0f, tabY + 4.0f});
        _window.draw(t0Txt);

        sf::Text t1Txt(_font, toSfString("[E] ВРАГИ И СПАВНЫ"), 11);
        t1Txt.setFillColor(tab1Active ? sf::Color(255, 215, 0) : sf::Color(160, 185, 210));
        t1Txt.setPosition({palX + 175.0f, tabY + 4.0f});
        _window.draw(t1Txt);

        sf::Text t2Txt(_font, toSfString("[⚡] ТРИГГЕРЫ И МЕХАНИКИ"), 11);
        t2Txt.setFillColor(tab2Active ? sf::Color(255, 215, 0) : sf::Color(160, 185, 210));
        t2Txt.setPosition({palX + 332.0f, tabY + 4.0f});
        _window.draw(t2Txt);

        sf::Text t3Txt(_font, toSfString("[📁] КОМНАТЫ"), 11);
        t3Txt.setFillColor(tab3Active ? sf::Color(255, 215, 0) : sf::Color(160, 185, 210));
        t3Txt.setPosition({palX + 514.0f, tabY + 4.0f});
        _window.draw(t3Txt);

        sf::Text sTxt(_font, toSfString("▶ СИМУЛЯЦИЯ"), 11);
        sTxt.setFillColor(sf::Color::White);
        sTxt.setStyle(sf::Text::Bold);
        sTxt.setPosition({palX + 650.0f, tabY + 4.0f});
        _window.draw(sTxt);

        sf::Text dupTxt(_font, toSfString("Клон в R" + std::to_string(_doorTargetRoom)), 11);
        dupTxt.setFillColor(sf::Color(150, 230, 255));
        dupTxt.setPosition({palX + 780.0f, tabY + 4.0f});
        _window.draw(dupTxt);

        sf::Text clrTxt(_font, toSfString("Очистить"), 11);
        clrTxt.setFillColor(sf::Color(255, 120, 120));
        clrTxt.setPosition({palX + 885.0f, tabY + 4.0f});
        _window.draw(clrTxt);

        sf::Text rTxt(_font, toSfString("[F5 Обновить]"), 11);
        rTxt.setFillColor(sf::Color(120, 230, 255));
        rTxt.setPosition({reloadX + 8.0f, tabY + 4.0f});
        _window.draw(rTxt);
    }

    // 5. Отрисовка карточек текущей вкладки
    float startX = palX + 10.0f;
    float cardY = palY + 30.0f;
    float cardW = 60.0f;
    float cardH = 78.0f;

    int slotIdx = 0;
    for (size_t i = 0; i < _paletteSprites.size(); ++i) {
        if (_paletteSprites[i].tab != _activeTab) continue;

        float cardX = startX + slotIdx * 65.0f;
        slotIdx++;
        if (cardX + cardW > palX + palW) break;

        bool isSelected = (_currentTool == _paletteSprites[i].tool);
        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cardX, cardY});
        card.setFillColor(isSelected ? sf::Color(35, 55, 80) : sf::Color(20, 26, 36));
        card.setOutlineColor(isSelected ? sf::Color(255, 215, 0) : sf::Color(60, 75, 100));
        card.setOutlineThickness(isSelected ? 2.5f : 1.0f);
        _window.draw(card);

        // Горячая клавиша инструмента
        if (_fontLoaded) {
            std::string hotkeyStr = "";
            if (_paletteSprites[i].tool == ToolType::PLATFORM) hotkeyStr = "[1/B]";
            else if (_paletteSprites[i].tool == ToolType::WALL) hotkeyStr = "[2]";
            else if (_paletteSprites[i].tool == ToolType::DOOR) hotkeyStr = "[3]";
            else if (_paletteSprites[i].tool == ToolType::PLAYER_SPAWN) hotkeyStr = "[4]";
            else if (_paletteSprites[i].tool == ToolType::ENEMY_SPAWN) hotkeyStr = "[5]";
            else if (_paletteSprites[i].tool == ToolType::ENEMY_TANK) hotkeyStr = "[6]";
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST) hotkeyStr = "[7]";
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST_PASSIVE) hotkeyStr = "[8]";
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUMMY) hotkeyStr = "[9]";
            else if (_paletteSprites[i].tool == ToolType::EDGE_PASSAGE) hotkeyStr = "[E]";
            else if (_paletteSprites[i].tool == ToolType::BOX_FILL) hotkeyStr = "[X]";
            else if (_paletteSprites[i].tool == ToolType::LINE) hotkeyStr = "[L]";
            else if (_paletteSprites[i].tool == ToolType::FLOOD_FILL) hotkeyStr = "[G]";
            else if (_paletteSprites[i].tool == ToolType::TEST_SPAWN) hotkeyStr = "[T]";

            if (!hotkeyStr.empty()) {
                sf::Text hkTxt(_font, hotkeyStr, 10);
                hkTxt.setFillColor(isSelected ? sf::Color(255, 235, 120) : sf::Color(140, 160, 180));
                hkTxt.setPosition({cardX + 4.0f, cardY + 3.0f});
                _window.draw(hkTxt);
            }
        }

        float iconX = cardX + (cardW - 32.0f) / 2.0f;
        float iconY = cardY + 17.0f;

        if (_paletteSprites[i].sprite) {
            _paletteSprites[i].sprite->setPosition({iconX, iconY});
            _window.draw(*_paletteSprites[i].sprite);
        } else {
            sf::RectangleShape colorBox({32.0f, 24.0f});
            colorBox.setPosition({iconX, iconY});
            if (_paletteSprites[i].tool == ToolType::PLATFORM) colorBox.setFillColor(sf::Color(76, 175, 80));
            else if (_paletteSprites[i].tool == ToolType::WALL) colorBox.setFillColor(sf::Color(90, 110, 140));
            else if (_paletteSprites[i].tool == ToolType::DOOR) colorBox.setFillColor(sf::Color(0, 229, 255));
            else if (_paletteSprites[i].tool == ToolType::PLAYER_SPAWN) colorBox.setFillColor(sf::Color(0, 230, 118));
            else if (_paletteSprites[i].tool == ToolType::ENEMY_SPAWN) colorBox.setFillColor(sf::Color(180, 180, 180));
            else if (_paletteSprites[i].tool == ToolType::ENEMY_TANK) colorBox.setFillColor(sf::Color(240, 50, 50));
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST) colorBox.setFillColor(sf::Color(170, 70, 230));
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST_PASSIVE) colorBox.setFillColor(sf::Color(45, 185, 245));
            else if (_paletteSprites[i].tool == ToolType::ENEMY_DUMMY) colorBox.setFillColor(sf::Color(220, 175, 75));
            else if (_paletteSprites[i].tool == ToolType::EDGE_PASSAGE) colorBox.setFillColor(sf::Color(60, 60, 60));
            else if (_paletteSprites[i].tool == ToolType::BOX_FILL) colorBox.setFillColor(sf::Color(100, 180, 255));
            else if (_paletteSprites[i].tool == ToolType::LINE) colorBox.setFillColor(sf::Color(80, 220, 160));
            else if (_paletteSprites[i].tool == ToolType::FLOOD_FILL) colorBox.setFillColor(sf::Color(255, 180, 0));
            else if (_paletteSprites[i].tool == ToolType::TEST_SPAWN) colorBox.setFillColor(sf::Color(255, 200, 0));
            else if (_paletteSprites[i].tool == ToolType::JUMP_PAD) colorBox.setFillColor(sf::Color(255, 150, 0));
            else if (_paletteSprites[i].tool == ToolType::SPIKES) colorBox.setFillColor(sf::Color(230, 40, 40));
            else if (_paletteSprites[i].tool == ToolType::TRIGGER_ZONE) colorBox.setFillColor(sf::Color(160, 60, 255));

            colorBox.setOutlineColor(sf::Color(200, 220, 240, 150));
            colorBox.setOutlineThickness(1.0f);
            _window.draw(colorBox);

            if (_fontLoaded) {
                std::string letter = "";
                if (_paletteSprites[i].tool == ToolType::PLAYER_SPAWN) letter = "P";
                else if (_paletteSprites[i].tool == ToolType::ENEMY_SPAWN) letter = "N";
                else if (_paletteSprites[i].tool == ToolType::ENEMY_TANK) letter = "T";
                else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST) letter = "DA";
                else if (_paletteSprites[i].tool == ToolType::ENEMY_DUELIST_PASSIVE) letter = "DP";
                else if (_paletteSprites[i].tool == ToolType::ENEMY_DUMMY) letter = "DM";
                else if (_paletteSprites[i].tool == ToolType::DOOR) letter = "D";
                else if (_paletteSprites[i].tool == ToolType::EDGE_PASSAGE) letter = "X";
                else if (_paletteSprites[i].tool == ToolType::TEST_SPAWN) letter = ">";
                else if (_paletteSprites[i].tool == ToolType::JUMP_PAD) letter = "^";
                else if (_paletteSprites[i].tool == ToolType::SPIKES) letter = "!";

                if (!letter.empty()) {
                    sf::Text lTxt(_font, letter, 12);
                    lTxt.setFillColor(sf::Color::White);
                    lTxt.setPosition({iconX + 6.0f, iconY + 3.0f});
                    _window.draw(lTxt);
                }
            }
        }

        if (_fontLoaded) {
            std::string label = _paletteSprites[i].name;
            if (label.size() > 9) label = label.substr(0, 8) + ".";
            sf::Text cardTxt(_font, toSfString(label), 10);
            cardTxt.setFillColor(isSelected ? sf::Color(255, 235, 120) : sf::Color(180, 195, 215));
            cardTxt.setPosition({cardX + 4.0f, cardY + 58.0f});
            _window.draw(cardTxt);
        }
    }

    // 6. Отрисовка модального окна настроек (если открыто)
    drawModal();

    // 7. Отрисовка галереи комнат (2 столбца превью + названия)
    if (_activeTab == PaletteTab::ROOMS) {
        drawRoomBrowser();
    }
}

void EditorApp::render() {
    _window.clear(sf::Color(18, 22, 30));

    if (_isSimulating) {
        // Отрисовка игрового мира в режиме симуляции
        _window.setView(_camera);

        if (_simArena) _simArena->draw(_window);
        for (auto& e : _simEnemies) {
            e.draw(_window, &_font);
        }
        if (_simCombat && _simPlayer) {
            _simCombat->drawWorld(_window, *_simPlayer);
        }
        if (_simPlayer) {
            _simPlayer->draw(_window);
        }

        // Отрисовка UI симуляции
        _window.setView(_window.getDefaultView());
        if (_simCombat && _simPlayer) {
            _simCombat->drawUI(_window, _simPlayer.get());
        }
        drawSimulationHUD();
    } else {
        // Отрисовка мира редактора
        _window.setView(_camera);
        drawWorld();

        // Отрисовка интерфейса редактора
        _window.setView(_window.getDefaultView());
        drawUI();
    }

    _window.display();
}

void EditorApp::run() {
    sf::Clock clock;
    while (_window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        handleEvents();
        update(dt);
        render();
    }
}
