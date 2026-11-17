#include "world/MapEditor.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <queue>

namespace {
// Вспомогательный алгоритм Брезенхема для рисования прямых линий тайлов
std::vector<sf::Vector2i> getLineTiles(int x0, int y0, int x1, int y1) {
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
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Стена";
        item.filename = "02_wall.png";
        item.tool = EditorTool::WALL;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Дверь";
        item.filename = "03_door.png";
        item.tool = EditorTool::DOOR;
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
    {
        IngameTileSpriteItem item;
        item.name = "Ластик";
        item.filename = "eraser.png";
        item.tool = EditorTool::ERASER;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Заливка Box";
        item.filename = "box_fill.png";
        item.tool = EditorTool::BOX_FILL;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Линия Line";
        item.filename = "line_tool.png";
        item.tool = EditorTool::LINE;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Ведро Fill";
        item.filename = "bucket_fill.png";
        item.tool = EditorTool::FLOOD_FILL;
        _paletteSprites.push_back(std::move(item));
    }
    {
        IngameTileSpriteItem item;
        item.name = "Пипетка";
        item.filename = "eyedropper.png";
        item.tool = EditorTool::EYEDROPPER;
        _paletteSprites.push_back(std::move(item));
    }

    // 2. Сканирование папки assets/tiles/
    std::string tilesDir = resolveAssetPath("assets/tiles");

    if (std::filesystem::exists(tilesDir)) {
        // Подгружаем текстуры для базовых
        for (auto& baseItem : _paletteSprites) {
            std::string p = tilesDir + "/" + baseItem.filename;
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

        // Пользовательские дополнительные тайлы
        for (const auto& entry : std::filesystem::directory_iterator(tilesDir)) {
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

void MapEditor::handlePlaytestInput(const std::optional<sf::Event>& event, sf::RenderWindow& window, sf::View& camera) {
    if (!event.has_value()) return;
    if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Tab || kp->code == sf::Keyboard::Key::P || kp->code == sf::Keyboard::Key::F6) {
            stopPlaytest(window, camera);
        }
    }
}

void MapEditor::drawPlaytestUI(sf::RenderWindow& window, const sf::Font* font) {
    if (!_isPlaytesting || !font) return;
    float winW = static_cast<float>(window.getSize().x);

    sf::RectangleShape banner({winW, 36.0f});
    banner.setPosition({0.0f, 0.0f});
    banner.setFillColor(sf::Color(180, 20, 20, 220));
    banner.setOutlineColor(sf::Color(255, 80, 80));
    banner.setOutlineThickness(1.0f);
    window.draw(banner);

    sf::Text txt(*font, toSfString("<< РЕЖИМ БЫСТРОГО ТЕСТИРОВАНИЯ (PLAYTEST) >>  Нажмите [Tab] или [F6] для возврата в редактор"), 15);
    txt.setFillColor(sf::Color::White);
    txt.setStyle(sf::Text::Bold);
    sf::FloatRect b = txt.getLocalBounds();
    txt.setPosition({(winW - b.size.x) / 2.0f, 8.0f});
    window.draw(txt);
}

void MapEditor::handleInput(const std::optional<sf::Event>& event, Arena& arena, 
                            Player& player, Enemy& enemy, sf::RenderWindow& window,
                            sf::View& camera) {
    if (!_active || !event.has_value()) return;

    if (_isPlaytesting) {
        handlePlaytestInput(event, window, camera);
        return;
    }

    if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
        bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
        bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
        bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);

        // Быстрый плейтест с точки курсора (Tab / P)
        if (kp->code == sf::Keyboard::Key::Tab || (kp->code == sf::Keyboard::Key::P && !ctrlHeld)) {
            startPlaytest(arena, player, window, camera);
            return;
        }

        // Undo / Redo
        if (ctrlHeld && kp->code == sf::Keyboard::Key::Z) {
            if (shiftHeld) redo(arena, player);
            else undo(arena, player);
            return;
        }
        if (ctrlHeld && kp->code == sf::Keyboard::Key::Y) {
            redo(arena, player);
            return;
        }

        // Копирование и Вставка
        if (ctrlHeld && kp->code == sf::Keyboard::Key::C) {
            copySelection(arena);
            return;
        }
        if (ctrlHeld && kp->code == sf::Keyboard::Key::V) {
            if (_clipboard.hasData) {
                _pasteModeActive = !_pasteModeActive;
                _statusMessage = _pasteModeActive ? "Режим вставки (Paste Mode)! Клик ЛКМ для размещения, Esc для отмены." : "Вставка отменена.";
                _statusTimer = 2.0f;
            } else {
                _statusMessage = "Буфер обмена пуст! Сначала выделите и нажмите Ctrl+C.";
                _statusTimer = 2.0f;
            }
            return;
        }
        if (kp->code == sf::Keyboard::Key::Escape) {
            if (_pasteModeActive) {
                _pasteModeActive = false;
                _statusMessage = "Режим вставки отменен.";
                _statusTimer = 1.5f;
                return;
            }
            if (_selectionActive) {
                _selectionActive = false;
                _statusMessage = "Выделение снято.";
                _statusTimer = 1.5f;
                return;
            }
        }

        // Линейка прыжка (J)
        if (kp->code == sf::Keyboard::Key::J) {
            toggleJumpRuler();
            _statusMessage = _showJumpRuler ? "Линейка прыжка (Jump Arc Ruler): ВКЛЮЧЕНА" : "Линейка прыжка: ВЫКЛЮЧЕНА";
            _statusTimer = 2.0f;
            return;
        }

        // Сетка (Ctrl+G)
        if (ctrlHeld && kp->code == sf::Keyboard::Key::G) {
            toggleGrid();
            _statusMessage = _showGrid ? "Сетка тайлов: ВКЛЮЧЕНА" : "Сетка тайлов: ВЫКЛЮЧЕНА";
            _statusTimer = 2.0f;
            return;
        }

        // Рамка 1920x1080 экрана (Ctrl+F)
        if (ctrlHeld && kp->code == sf::Keyboard::Key::F) {
            toggleScreenFrame();
            _statusMessage = _showScreenFrame ? "Рамка экрана (1920x1080 Viewfinder): ВКЛЮЧЕНА" : "Рамка экрана: ВЫКЛЮЧЕНА";
            _statusTimer = 2.0f;
            return;
        }

        // Пипетка (Q)
        if (kp->code == sf::Keyboard::Key::Q) {
            eyedropAt(_lastHoverTile.x, _lastHoverTile.y, arena);
            return;
        }

        // Заливка ведро (G без Ctrl)
        if (!ctrlHeld && kp->code == sf::Keyboard::Key::G) {
            _tool = EditorTool::FLOOD_FILL;
            _statusMessage = "Инструмент: Заливка замкнутой области (Bucket / Flood Fill)";
            _statusTimer = 2.0f;
            return;
        }

        // Линия Line Tool (L)
        if (kp->code == sf::Keyboard::Key::L) {
            _tool = EditorTool::LINE;
            _statusMessage = "Инструмент: Линия (Line Tool - протяжка прямой платформы)";
            _statusTimer = 2.0f;
            return;
        }

        // Прямоугольная заливка Box Fill (X)
        if (kp->code == sf::Keyboard::Key::X && !ctrlHeld) {
            _tool = EditorTool::BOX_FILL;
            _statusMessage = "Инструмент: Прямоугольная заливка (Box Fill)";
            _statusTimer = 2.0f;
            return;
        }

        // Дублирование комнаты (Ctrl+D)
        if (ctrlHeld && kp->code == sf::Keyboard::Key::D) {
            duplicateRoom(_doorTargetRoom, arena);
            return;
        }

        // Инструменты 1..9
        if (kp->code == sf::Keyboard::Key::Num1 || kp->code == sf::Keyboard::Key::Numpad1) {
            _tool = EditorTool::PLATFORM;
            _statusMessage = "Инструмент: Платформа (=)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num2 || kp->code == sf::Keyboard::Key::Numpad2) {
            _tool = EditorTool::WALL;
            _statusMessage = "Инструмент: Стена / Монолитный блок (#)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num3 || kp->code == sf::Keyboard::Key::Numpad3) {
            _tool = EditorTool::DOOR;
            _statusMessage = "Инструмент: Дверь / Переход между комнатами (D)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num4 || kp->code == sf::Keyboard::Key::Numpad4) {
            _tool = EditorTool::PLAYER_SPAWN;
            _statusMessage = "Инструмент: Спавн игрока (P)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num5 || kp->code == sf::Keyboard::Key::Numpad5) {
            _tool = EditorTool::ENEMY_SPAWN;
            _statusMessage = "Инструмент: Враг (Обычный / Normal)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num6 || kp->code == sf::Keyboard::Key::Numpad6) {
            _tool = EditorTool::ENEMY_TANK;
            _statusMessage = "Инструмент: Враг (Жирный / Tank - Крупный, много ХП)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num7 || kp->code == sf::Keyboard::Key::Numpad7) {
            _tool = EditorTool::ENEMY_DUELIST;
            _statusMessage = "Инструмент: Враг (Дуэлянт Агрессивный - Фиолетовый, прессинг, зацеп)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num8 || kp->code == sf::Keyboard::Key::Numpad8) {
            _tool = EditorTool::ENEMY_DUELIST_PASSIVE;
            _statusMessage = "Инструмент: Враг (Дуэлянт Пассивный - Голубой, спейсинг, невидимость)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num9 || kp->code == sf::Keyboard::Key::Numpad9) {
            _tool = EditorTool::ERASER;
            _statusMessage = "Инструмент: Ластик (стереть блок / врага)";
            _statusTimer = 2.0f;
        }

        // Кисть и настройка двери
        if (kp->code == sf::Keyboard::Key::B) {
            cycleBrushSize();
        } else if (kp->code == sf::Keyboard::Key::LBracket) {
            if (_tool == EditorTool::DOOR) {
                _doorTargetRoom = (_doorTargetRoom <= 1) ? arena.getTotalRooms() : (_doorTargetRoom - 1);
                _statusMessage = "Целевая комната двери: Комната " + std::to_string(_doorTargetRoom);
                _statusTimer = 2.0f;
            } else {
                cycleBrushSize();
            }
        } else if (kp->code == sf::Keyboard::Key::RBracket) {
            if (_tool == EditorTool::DOOR) {
                _doorTargetRoom = (_doorTargetRoom >= arena.getTotalRooms()) ? 1 : (_doorTargetRoom + 1);
                _statusMessage = "Целевая комната двери: Комната " + std::to_string(_doorTargetRoom);
                _statusTimer = 2.0f;
            } else {
                cycleBrushSize();
            }
        }

        // Горячая перезагрузка спрайтов
        if (kp->code == sf::Keyboard::Key::F5) {
            scanTileSprites();
            arena.reload();
            _statusMessage = "Спрайты и карта перезагружены!";
            _statusTimer = 2.0f;
        }

        // Переключение комнат
        if (kp->code == sf::Keyboard::Key::PageUp) {
            nextRoom(arena, player, enemy, camera);
        } else if (kp->code == sf::Keyboard::Key::PageDown) {
            prevRoom(arena, player, enemy, camera);
        }

        // Зум
        if (kp->code == sf::Keyboard::Key::Z && !ctrlHeld) {
            zoomIn(camera);
        } else if (kp->code == sf::Keyboard::Key::C && !ctrlHeld) {
            resetZoom(camera);
        }

        // Изменение размера холста (Alt + Стрелки)
        if (altHeld) {
            pushUndo(arena);
            if (kp->code == sf::Keyboard::Key::Right) {
                arena.resize(arena.getWidth() + 10, arena.getHeight());
                _statusMessage = "Размер комнаты расширен: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight());
                _statusTimer = 2.0f;
            } else if (kp->code == sf::Keyboard::Key::Left) {
                arena.resize(arena.getWidth() - 10, arena.getHeight());
                _statusMessage = "Размер комнаты сужен: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight());
                _statusTimer = 2.0f;
            } else if (kp->code == sf::Keyboard::Key::Down) {
                arena.resize(arena.getWidth(), arena.getHeight() + 5);
                _statusMessage = "Высота комнаты увеличена: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight());
                _statusTimer = 2.0f;
            } else if (kp->code == sf::Keyboard::Key::Up) {
                arena.resize(arena.getWidth(), arena.getHeight() - 5);
                _statusMessage = "Высота комнаты уменьшена: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight());
                _statusTimer = 2.0f;
            }
        }

        // Сохранение текущей комнаты
        if ((kp->code == sf::Keyboard::Key::S && ctrlHeld) || kp->code == sf::Keyboard::Key::F7) {
            saveCurrentRoom(arena);
        }
    }

    // Зум колесиком мыши (с зажатым Ctrl) или переключение инструментов
    if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
        bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
        if (ctrlHeld) {
            if (wheel->delta > 0) zoomIn(camera);
            else if (wheel->delta < 0) zoomOut(camera);
        }
    }

    // Обработка нажатий мыши
    if (const auto* mbPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        bool shiftHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                         sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
        bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
        bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);

        if (mbPressed->button == sf::Mouse::Button::Middle) {
            if (altHeld || ctrlHeld) {
                eyedropAt(_lastHoverTile.x, _lastHoverTile.y, arena);
            } else {
                _isPanning = true;
                _lastMousePixel = sf::Mouse::getPosition(window);
            }
        } else if (mbPressed->button == sf::Mouse::Button::Left) {
            sf::Vector2i mPixel = sf::Mouse::getPosition(window);
            if (handlePaletteClick(mPixel, arena, player, enemy, camera, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y))) return;
            if (handleMinimapClick(mPixel, arena, camera)) return;

            // Если активен режим вставки
            if (_pasteModeActive) {
                pasteClipboard(_lastHoverTile.x, _lastHoverTile.y, arena, player);
                return;
            }

            // Пипетка на Alt+Click
            if (altHeld && !shiftHeld) {
                if (_tool == EditorTool::LINE) {
                    // Line tool start
                    _lineDragActive = true;
                    _lineStartTile = _lineCurTile = _lastHoverTile;
                } else {
                    eyedropAt(_lastHoverTile.x, _lastHoverTile.y, arena);
                }
                return;
            }

            // Выделение области на Ctrl+LMB Drag
            if (ctrlHeld) {
                _selectionActive = true;
                _selStart = _selEnd = _lastHoverTile;
                return;
            }

            // Прямоугольная заливка Box Fill на Shift+LMB или tool == BOX_FILL
            if (shiftHeld || _tool == EditorTool::BOX_FILL) {
                _boxDragActive = true;
                _boxIsErase = false;
                _boxStartTile = _boxCurTile = _lastHoverTile;
                return;
            }

            // Линия Line Tool
            if (_tool == EditorTool::LINE) {
                _lineDragActive = true;
                _lineStartTile = _lineCurTile = _lastHoverTile;
                return;
            }

            // Заливка Bucket
            if (_tool == EditorTool::FLOOD_FILL) {
                int fillT = 1;
                floodFill(_lastHoverTile.x, _lastHoverTile.y, arena, fillT);
                return;
            }

            // Пипетка
            if (_tool == EditorTool::EYEDROPPER) {
                eyedropAt(_lastHoverTile.x, _lastHoverTile.y, arena);
                return;
            }

            // Обычное рисование
            pushUndo(arena);
            applyToolAt(_lastHoverTile.x, _lastHoverTile.y, arena, player, enemy);
        } else if (mbPressed->button == sf::Mouse::Button::Right) {
            // Shift + RMB: Box Erase
            if (shiftHeld) {
                _boxDragActive = true;
                _boxIsErase = true;
                _boxStartTile = _boxCurTile = _lastHoverTile;
                return;
            }

            // Проверка клика по существующему врагу: смена архетипа
            float ts = arena.getTileSize();
            bool enemyClicked = false;
            auto spawns = arena.getEnemySpawns();
            for (size_t i = 0; i < spawns.size(); ++i) {
                int stx = static_cast<int>(std::floor(spawns[i].position.x / ts));
                int sty = static_cast<int>(std::floor((spawns[i].position.y + 28.0f) / ts) - 1.0f);
                if (stx == _lastHoverTile.x && sty == _lastHoverTile.y) {
                    pushUndo(arena);
                    int nextType = (spawns[i].type + 1) % 4; // 0=Norm, 1=Tank, 2=Agr, 3=Pass
                    spawns[i].type = nextType;
                    arena.setEnemySpawns(spawns);
                    std::string tName = (nextType == 1) ? "Жирный (Tank)" :
                                        (nextType == 2) ? "Дуэлянт Агрессивный" :
                                        (nextType == 3) ? "Дуэлянт Пассивный" : "Обычный (Normal)";
                    _statusMessage = "Враг #" + std::to_string(i + 1) + " переключен: [" + tName + "]";
                    _statusTimer = 2.0f;
                    enemyClicked = true;
                    break;
                }
            }
            if (enemyClicked) return;

            // Проверка клика по двери: смена целевой комнаты
            if (arena.isDoor(_lastHoverTile.x, _lastHoverTile.y)) {
                pushUndo(arena);
                int curTgt = arena.getDoorTargetRoom(_lastHoverTile.x, _lastHoverTile.y);
                int nextTgt = (curTgt % arena.getTotalRooms()) + 1;
                arena.setDoor(_lastHoverTile.x, _lastHoverTile.y, nextTgt);
                _statusMessage = "Целевая комната двери изменена: Комната " + std::to_string(nextTgt);
                _statusTimer = 2.0f;
                return;
            }

            // Иначе - обычное стирание
            pushUndo(arena);
            eraseAt(_lastHoverTile.x, _lastHoverTile.y, arena);
        }
    }

    if (const auto* mbReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if (mbReleased->button == sf::Mouse::Button::Middle) {
            _isPanning = false;
        } else if (mbReleased->button == sf::Mouse::Button::Left || mbReleased->button == sf::Mouse::Button::Right) {
            // Завершение Box Fill
            if (_boxDragActive) {
                pushUndo(arena);
                int x1 = std::max(0, std::min(_boxStartTile.x, _boxCurTile.x));
                int x2 = std::min(arena.getWidth() - 1, std::max(_boxStartTile.x, _boxCurTile.x));
                int y1 = std::max(0, std::min(_boxStartTile.y, _boxCurTile.y));
                int y2 = std::min(arena.getHeight() - 1, std::max(_boxStartTile.y, _boxCurTile.y));

                int tileVal = (_tool == EditorTool::WALL ? 2 : 1);
                for (int y = y1; y <= y2; ++y) {
                    for (int x = x1; x <= x2; ++x) {
                        if (_boxIsErase) {
                            arena.setTile(x, y, 0);
                            arena.removeDoor(x, y);
                            arena.removeEnemySpawnAtTile(x, y);
                        } else {
                            arena.setTile(x, y, tileVal);
                        }
                    }
                }
                arena.markTilemapDirty();
                _statusMessage = _boxIsErase ? "Область Box стерта!" : "Область Box залита!";
                _statusTimer = 2.0f;
                _boxDragActive = false;
            }

            // Завершение Line Tool
            if (_lineDragActive) {
                pushUndo(arena);
                auto lineTiles = getLineTiles(_lineStartTile.x, _lineStartTile.y, _lineCurTile.x, _lineCurTile.y);
                int tileVal = (_tool == EditorTool::WALL ? 2 : 1);
                for (const auto& pt : lineTiles) {
                    if (pt.x >= 0 && pt.x < arena.getWidth() && pt.y >= 0 && pt.y < arena.getHeight()) {
                        arena.setTile(pt.x, pt.y, tileVal);
                    }
                }
                arena.markTilemapDirty();
                _statusMessage = "Линия размещена (" + std::to_string(lineTiles.size()) + " блоков)!";
                _statusTimer = 2.0f;
                _lineDragActive = false;
            }
        }
    }
}

bool MapEditor::handlePaletteClick(const sf::Vector2i& mousePixel, Arena& arena, Player& player, Enemy& enemy, sf::View& camera, float winW, float winH) {
    float mmW = std::min(240.0f, winW * 0.22f);
    float mmX = winW - mmW - 15.0f;
    float palX = 15.0f;
    float palH = 112.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX - palX - 12.0f;

    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    if (mPos.x < palX || mPos.x > palX + palW || mPos.y < palY || mPos.y > palY + palH) {
        return false;
    }

    // 1. Кнопка [ < ]
    sf::FloatRect prevBtn({palX + 328.0f, palY + 4.0f}, {20.0f, 20.0f});
    if (prevBtn.contains(mPos)) {
        _doorTargetRoom = (_doorTargetRoom <= 1) ? arena.getTotalRooms() : (_doorTargetRoom - 1);
        _tool = EditorTool::DOOR;
        _statusMessage = "Целевая комната для дверей: Комната " + std::to_string(_doorTargetRoom);
        _statusTimer = 2.0f;
        return true;
    }

    // 2. Кнопка [ > ]
    sf::FloatRect nextBtn({palX + 394.0f, palY + 4.0f}, {20.0f, 20.0f});
    if (nextBtn.contains(mPos)) {
        _doorTargetRoom = (_doorTargetRoom >= arena.getTotalRooms()) ? 1 : (_doorTargetRoom + 1);
        _tool = EditorTool::DOOR;
        _statusMessage = "Целевая комната для дверей: Комната " + std::to_string(_doorTargetRoom);
        _statusTimer = 2.0f;
        return true;
    }

    // 3. Быстрые цифровые кнопки комнат: 1..10
    for (int r = 1; r <= 10; ++r) {
        float rx = palX + 420.0f + (r - 1) * 25.0f;
        float ry = palY + 4.0f;
        sf::FloatRect rBtnRect({rx, ry}, {22.0f, 20.0f});
        if (rBtnRect.contains(mPos)) {
            _doorTargetRoom = r;
            _tool = EditorTool::DOOR;
            _statusMessage = "Выбран инструмент ДВЕРЬ -> Целевая комната: Комната " + std::to_string(r);
            _statusTimer = 2.5f;
            return true;
        }
    }

    // 4. Кнопка [Дублировать в R...]
    sf::FloatRect dupBtn({palX + 680.0f, palY + 4.0f}, {100.0f, 20.0f});
    if (dupBtn.contains(mPos)) {
        duplicateRoom(_doorTargetRoom, arena);
        return true;
    }

    // 5. Кнопка [Очистить]
    sf::FloatRect clrBtn({palX + 785.0f, palY + 4.0f}, {80.0f, 20.0f});
    if (clrBtn.contains(mPos)) {
        clearRoom(arena, player);
        return true;
    }

    // 6. Кнопка [F5 - Обновить спрайты]
    float reloadX = std::max(palX + 875.0f, palX + palW - 120.0f);
    sf::FloatRect reloadBtn({reloadX, palY + 4.0f}, {115.0f, 20.0f});
    if (reloadBtn.contains(mPos)) {
        scanTileSprites();
        _statusMessage = "Спрайты обновлены из assets/tiles/!";
        _statusTimer = 2.0f;
        return true;
    }

    // 7. Клик по карточкам спрайтов/инструментов
    float startX = palX + 10.0f;
    float cardY = palY + 28.0f;
    float cardW = 60.0f;
    float cardH = 76.0f;

    for (size_t i = 0; i < _paletteSprites.size(); ++i) {
        float cardX = startX + i * 65.0f;
        if (cardX + cardW > palX + palW) break;

        sf::FloatRect cardRect({cardX, cardY}, {cardW, cardH});
        if (cardRect.contains(mPos)) {
            _tool = _paletteSprites[i].tool;
            _statusMessage = "Выбран инструмент: " + _paletteSprites[i].name;
            _statusTimer = 2.0f;
            return true;
        }
    }
    return true;
}

bool MapEditor::handleMinimapClick(const sf::Vector2i& mousePixel, Arena& arena, sf::View& camera) {
    if (_minimapRect.contains({static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y)})) {
        float localX = mousePixel.x - _minimapRect.position.x;
        float localY = mousePixel.y - _minimapRect.position.y;

        float mapW = arena.getWidth() * arena.getTileSize();
        float mapH = arena.getHeight() * arena.getTileSize();
        float scaleX = _minimapRect.size.x / mapW;
        float scaleY = _minimapRect.size.y / mapH;
        float scale = std::min(scaleX, scaleY);

        _cameraPos = { localX / scale, localY / scale };
        camera.setCenter(_cameraPos);
        return true;
    }
    return false;
}

void MapEditor::update(float dt, Arena& arena, Player& player, Enemy& enemy, 
                       sf::RenderWindow& window, sf::View& camera) {
    if (!_active || _isPlaytesting) return;

    if (_statusTimer > 0.0f) {
        _statusTimer -= dt;
        if (_statusTimer <= 0.0f) _statusMessage.clear();
    }

    if (!window.hasFocus()) return;

    // Перетаскивание камеры средней кнопкой мыши
    if (_isPanning) {
        sf::Vector2i curPixel = sf::Mouse::getPosition(window);
        sf::Vector2i delta = curPixel - _lastMousePixel;
        _cameraPos -= sf::Vector2f(delta.x * _viewScale, delta.y * _viewScale);
        camera.setCenter(_cameraPos);
        _lastMousePixel = curPixel;
    } else {
        // Перемещение камеры на WASD / Стрелки
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
            camera.setCenter(_cameraPos);
        } else {
            _cameraPos = camera.getCenter();
        }
    }

    // Обработка рисования мышью
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    if (mousePixel.y >= static_cast<int>(window.getSize().y) - 110 ||
        _minimapRect.contains({static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y)})) {
        return;
    }

    sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, camera);
    float ts = arena.getTileSize();
    int tx = static_cast<int>(std::floor(mouseWorld.x / ts));
    int ty = static_cast<int>(std::floor(mouseWorld.y / ts));
    _lastHoverTile = {tx, ty};

    // Обновление перетаскивания Box / Line / Selection
    if (_boxDragActive) {
        _boxCurTile = _lastHoverTile;
    }
    if (_lineDragActive) {
        _lineCurTile = _lastHoverTile;
    }
    if (_selectionActive) {
        _selEnd = _lastHoverTile;
    }

    // Непрерывное рисование зажатой ЛКМ/ПКМ (только для обычных кистей)
    if (!_boxDragActive && !_lineDragActive && !_selectionActive && !_pasteModeActive &&
        _tool != EditorTool::BOX_FILL && _tool != EditorTool::LINE && 
        _tool != EditorTool::FLOOD_FILL && _tool != EditorTool::EYEDROPPER) {
        if (tx >= 0 && tx < arena.getWidth() && ty >= 0 && ty < arena.getHeight()) {
            if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
                applyToolAt(tx, ty, arena, player, enemy);
            } else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
                eraseAt(tx, ty, arena);
            }
        }
    }
}

void MapEditor::applyToolAt(int tx, int ty, Arena& arena, Player& player, Enemy& enemy) {
    float ts = arena.getTileSize();

    if (_tool == EditorTool::PLAYER_SPAWN) {
        arena.setTile(tx, ty, 0);
        sf::Vector2f spawnPos = { static_cast<float>(tx * ts), static_cast<float>((ty + 1) * ts - 30.0f) };
        arena.setSpawnPosition(spawnPos);
        player.reset(spawnPos);
        _statusMessage = "Спавн игрока (P) установлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "]";
        _statusTimer = 2.0f;
        return;
    }
    if (_tool == EditorTool::ENEMY_SPAWN || _tool == EditorTool::ENEMY_TANK || 
        _tool == EditorTool::ENEMY_DUELIST || _tool == EditorTool::ENEMY_DUELIST_PASSIVE) {
        arena.setTile(tx, ty, 0);
        int enemyType = (_tool == EditorTool::ENEMY_TANK) ? 1 : 
                        (_tool == EditorTool::ENEMY_DUELIST) ? 2 : 
                        (_tool == EditorTool::ENEMY_DUELIST_PASSIVE) ? 3 : 0;
        arena.setEnemySpawnAt(tx, ty, enemyType);
        std::string tName = (enemyType == 1) ? "Жирный (Tank)" : 
                            (enemyType == 2) ? "Дуэлянт Агрессивный" : 
                            (enemyType == 3) ? "Дуэлянт Пассивный" : "Обычный (Normal)";
        _statusMessage = "Враг [" + tName + "] установлен на [" + std::to_string(tx) + ", " + std::to_string(ty) + "] (Всего: " + std::to_string(arena.getEnemySpawns().size()) + ")";
        _statusTimer = 2.0f;
        return;
    }
    if (_tool == EditorTool::DOOR) {
        arena.setDoor(tx, ty, _doorTargetRoom);
        _statusMessage = "Дверь перехода (D) установлена на [" + std::to_string(tx) + ", " + std::to_string(ty) + "] -> Цель: Комната " + std::to_string(_doorTargetRoom);
        _statusTimer = 2.0f;
        return;
    }

    int tileVal = (_tool == EditorTool::PLATFORM ? 1 : 2);
    int half = _brushSize / 2;
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            int cx = tx + dx;
            int cy = ty + dy;
            if (cx >= 0 && cx < arena.getWidth() && cy >= 0 && cy < arena.getHeight()) {
                arena.setTile(cx, cy, tileVal);
            }
        }
    }
}

void MapEditor::eraseAt(int tx, int ty, Arena& arena) {
    int half = _brushSize / 2;
    for (int dy = -half; dy <= half; ++dy) {
        for (int dx = -half; dx <= half; ++dx) {
            int cx = tx + dx;
            int cy = ty + dy;
            if (cx >= 0 && cx < arena.getWidth() && cy >= 0 && cy < arena.getHeight()) {
                arena.setTile(cx, cy, 0);
                arena.removeDoor(cx, cy);
                arena.removeEnemySpawnAtTile(cx, cy);
            }
        }
    }
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

void MapEditor::drawJumpArc(sf::RenderWindow& window, const Arena& arena, const sf::Font* font) {
    if (!_showJumpRuler) return;
    float ts = arena.getTileSize();
    sf::Vector2f startPos;
    if (_lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < arena.getHeight()) {
        startPos = { static_cast<float>(_lastHoverTile.x * ts + ts / 2.0f),
                     static_cast<float>(_lastHoverTile.y * ts) };
    } else {
        startPos = arena.getSpawnPosition();
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

            int tx = static_cast<int>(std::floor(px / ts));
            int ty = static_cast<int>(std::floor(py / ts));
            if (t > 0.08f && (arena.isSolid(tx, ty) || py > startPos.y + 350.0f)) {
                arc.append(sf::Vertex({curPt, sf::Color(255, 215, 0, 180)}));
                break;
            }

            sf::Color col = (t < 0.8f) ? sf::Color(0, 230, 255, 210) : sf::Color(255, 200, 50, 160);
            arc.append(sf::Vertex({curPt, col}));
        }
        window.draw(arc);

        sf::CircleShape apexCircle(4.0f);
        apexCircle.setFillColor(sf::Color(255, 215, 0));
        apexCircle.setOrigin({4.0f, 4.0f});
        apexCircle.setPosition(apexPt);
        window.draw(apexCircle);

        if (font && dir == 1) {
            float jumpHeight = std::abs(apexPt.y - startPos.y);
            sf::Text aTxt(*font, "Apex: -" + std::to_string(static_cast<int>(jumpHeight)) + "px", 11);
            aTxt.setFillColor(sf::Color(255, 235, 100));
            aTxt.setPosition({apexPt.x + 6.0f, apexPt.y - 14.0f});
            window.draw(aTxt);
        }
    }

    // Линейка горизонтального дэша (Dash: 720 px/s * 0.25s = 180 px)
    const float dashDist = 180.0f;
    for (int dir : {1, -1}) {
        sf::VertexArray dashLine(sf::PrimitiveType::Lines);
        dashLine.append(sf::Vertex({startPos, sf::Color(255, 80, 220, 220)}));
        dashLine.append(sf::Vertex({sf::Vector2f(startPos.x + dir * dashDist, startPos.y), sf::Color(255, 80, 220, 220)}));
        window.draw(dashLine);

        sf::CircleShape dashEnd(3.0f);
        dashEnd.setFillColor(sf::Color(255, 80, 220));
        dashEnd.setOrigin({3.0f, 3.0f});
        dashEnd.setPosition({startPos.x + dir * dashDist, startPos.y});
        window.draw(dashEnd);
    }
}

void MapEditor::drawWorld(sf::RenderWindow& window, const Arena& arena, const sf::Vector2f& mouseWorld,
                          const sf::Font* font) {
    if (!_active || _isPlaytesting) return;

    float ts = arena.getTileSize();
    sf::View view = window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / ts) - 1);
    int endX = std::min(arena.getWidth(), static_cast<int>((center.x + size.x / 2.0f) / ts) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / ts) - 1);
    int endY = std::min(arena.getHeight(), static_cast<int>((center.y + size.y / 2.0f) / ts) + 2);

    // 1. Сетка тайлов (Grid Overlay)
    if (_showGrid) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);
        for (int x = startX; x <= endX; ++x) {
            lines.append(sf::Vertex({sf::Vector2f(x * ts, startY * ts), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(x * ts, endY * ts), sf::Color(100, 120, 150, 40)}));
        }
        for (int y = startY; y <= endY; ++y) {
            lines.append(sf::Vertex({sf::Vector2f(startX * ts, y * ts), sf::Color(100, 120, 150, 40)}));
            lines.append(sf::Vertex({sf::Vector2f(endX * ts, y * ts), sf::Color(100, 120, 150, 40)}));
        }
        window.draw(lines);
    }

    // 2. Рамка экрана 1920x1080 (Viewfinder)
    if (_showScreenFrame) {
        sf::RectangleShape frame({1920.0f, 1080.0f});
        frame.setOrigin({960.0f, 540.0f});
        frame.setPosition(center);
        frame.setFillColor(sf::Color::Transparent);
        frame.setOutlineColor(sf::Color(255, 80, 220, 180));
        frame.setOutlineThickness(2.0f);
        window.draw(frame);

        if (font) {
            sf::Text fTxt(*font, "1920x1080 Viewport", 12);
            fTxt.setFillColor(sf::Color(255, 120, 230));
            fTxt.setPosition({center.x - 950.0f, center.y - 530.0f});
            window.draw(fTxt);
        }
    }

    // 3. Маркер спавна игрока
    sf::Vector2f pSpawn = arena.getSpawnPosition();
    float pBoxX = std::round(pSpawn.x / ts) * ts;
    float pBoxY = (std::round((pSpawn.y + 30.0f) / ts) - 1.0f) * ts;
    sf::RectangleShape pBox({ts, ts});
    pBox.setPosition({pBoxX, pBoxY});
    pBox.setFillColor(sf::Color(0, 230, 118, 50));
    pBox.setOutlineColor(sf::Color(0, 230, 118, 220));
    pBox.setOutlineThickness(2.0f);
    window.draw(pBox);

    if (font) {
        sf::Text pText(*font, "P", 18);
        pText.setFillColor(sf::Color(0, 230, 118));
        pText.setPosition({pBoxX + ts * 0.35f, pBoxY + ts * 0.25f});
        window.draw(pText);
    }

    // 4. Маркеры спавна врагов с инспектором
    const auto& eSpawns = arena.getEnemySpawns();
    int hoveredEnemyIdx = -1;
    for (size_t i = 0; i < eSpawns.size(); ++i) {
        sf::Vector2f eSpawn = eSpawns[i].position;
        int eType = eSpawns[i].type;
        float eBoxX = std::round(eSpawn.x / ts) * ts;
        float eBoxY = (std::round((eSpawn.y + 28.0f) / ts) - 1.0f) * ts;

        int stx = static_cast<int>(std::floor(eSpawn.x / ts));
        int sty = static_cast<int>(std::floor((eSpawn.y + 28.0f) / ts) - 1.0f);
        if (stx == _lastHoverTile.x && sty == _lastHoverTile.y) {
            hoveredEnemyIdx = static_cast<int>(i);
        }

        float boxW = (eType == 1) ? ts * 1.5f : ts;
        float boxH = (eType == 1) ? ts * 1.5f : ts;
        float drawX = (eType == 1) ? (eBoxX - ts * 0.25f) : eBoxX;
        float drawY = (eType == 1) ? (eBoxY - ts * 0.5f) : eBoxY;

        sf::RectangleShape eBox({boxW, boxH});
        eBox.setPosition({drawX, drawY});

        sf::Color fillCol, outCol, txtCol;
        std::string typePrefix;
        if (eType == 1) { // TANK
            fillCol = sf::Color(220, 40, 40, 75);
            outCol = sf::Color(255, 60, 60, 240);
            txtCol = sf::Color(255, 120, 120);
            typePrefix = "Tank#";
        } else if (eType == 2) { // DUELIST AGGRESSIVE
            fillCol = sf::Color(170, 70, 230, 75);
            outCol = sf::Color(190, 90, 255, 240);
            txtCol = sf::Color(220, 150, 255);
            typePrefix = "D-Agr#";
        } else if (eType == 3) { // DUELIST PASSIVE
            fillCol = sf::Color(35, 150, 225, 75);
            outCol = sf::Color(45, 185, 245, 240);
            txtCol = sf::Color(100, 220, 255);
            typePrefix = "D-Pas#";
        } else { // NORMAL
            fillCol = sf::Color(150, 150, 150, 75);
            outCol = sf::Color(210, 210, 210, 240);
            txtCol = sf::Color(230, 230, 230);
            typePrefix = "Norm#";
        }

        eBox.setFillColor(fillCol);
        eBox.setOutlineColor(outCol);
        eBox.setOutlineThickness(2.0f);
        window.draw(eBox);

        if (font) {
            sf::Text eText(*font, typePrefix + std::to_string(i + 1), 13);
            eText.setFillColor(txtCol);
            eText.setPosition({drawX + 4.0f, drawY + 4.0f});
            window.draw(eText);
        }
    }

    // 5. Инспектор объекта под курсором (Floating Inspector)
    if (font && hoveredEnemyIdx >= 0) {
        int eType = eSpawns[hoveredEnemyIdx].type;
        std::string eName = (eType == 1) ? "ТАНК (110 HP, Стан -45%)" :
                            (eType == 2) ? "ДУЭЛЯНТ АГР (65 HP, Прессинг, Зацеп 210px)" :
                            (eType == 3) ? "ДУЭЛЯНТ ПАСС (65 HP, Спейсинг, Инвиз 1с)" : "ОБЫЧНЫЙ (45 HP, Базовый)";
        std::string inspStr = "ВРАГ #" + std::to_string(hoveredEnemyIdx + 1) + "\n" + eName + "\n[ПКМ] - Сменить архетип | [Ластик] - Удалить";
        sf::Text inspTxt(*font, toSfString(inspStr), 12);
        inspTxt.setFillColor(sf::Color(255, 240, 150));

        float insX = _lastHoverTile.x * ts + ts + 8.0f;
        float insY = _lastHoverTile.y * ts - 10.0f;
        sf::RectangleShape inspBg({260.0f, 56.0f});
        inspBg.setPosition({insX, insY});
        inspBg.setFillColor(sf::Color(15, 20, 30, 235));
        inspBg.setOutlineColor(sf::Color(255, 215, 0, 220));
        inspBg.setOutlineThickness(1.5f);
        window.draw(inspBg);

        inspTxt.setPosition({insX + 8.0f, insY + 6.0f});
        window.draw(inspTxt);
    } else if (font && arena.isDoor(_lastHoverTile.x, _lastHoverTile.y)) {
        int target = arena.getDoorTargetRoom(_lastHoverTile.x, _lastHoverTile.y);
        std::string inspStr = "ДВЕРЬ ПЕРЕХОДА (Door Block)\nЦель: Комната " + std::to_string(target) + "\n[ПКМ] - След. цель (1..10) | [1..10] - Палитра";
        sf::Text inspTxt(*font, toSfString(inspStr), 12);
        inspTxt.setFillColor(sf::Color(0, 255, 255));

        float insX = _lastHoverTile.x * ts + ts + 8.0f;
        float insY = _lastHoverTile.y * ts - 10.0f;
        sf::RectangleShape inspBg({250.0f, 56.0f});
        inspBg.setPosition({insX, insY});
        inspBg.setFillColor(sf::Color(10, 25, 35, 235));
        inspBg.setOutlineColor(sf::Color(0, 229, 255, 220));
        inspBg.setOutlineThickness(1.5f);
        window.draw(inspBg);

        inspTxt.setPosition({insX + 8.0f, insY + 6.0f});
        window.draw(inspTxt);
    }

    // 6. Превью прямоугольной заливки Box Fill
    if (_boxDragActive) {
        int x1 = std::min(_boxStartTile.x, _boxCurTile.x);
        int x2 = std::max(_boxStartTile.x, _boxCurTile.x);
        int y1 = std::min(_boxStartTile.y, _boxCurTile.y);
        int y2 = std::max(_boxStartTile.y, _boxCurTile.y);
        float bw = (x2 - x1 + 1) * ts;
        float bh = (y2 - y1 + 1) * ts;

        sf::RectangleShape boxRect({bw, bh});
        boxRect.setPosition({x1 * ts, y1 * ts});
        if (_boxIsErase) {
            boxRect.setFillColor(sf::Color(240, 40, 40, 85));
            boxRect.setOutlineColor(sf::Color(255, 60, 60, 255));
        } else {
            boxRect.setFillColor(sf::Color(76, 175, 80, 85));
            boxRect.setOutlineColor(sf::Color(100, 255, 120, 255));
        }
        boxRect.setOutlineThickness(2.0f);
        window.draw(boxRect);

        if (font) {
            std::string dimStr = std::to_string(x2 - x1 + 1) + "x" + std::to_string(y2 - y1 + 1);
            sf::Text dimTxt(*font, dimStr, 13);
            dimTxt.setFillColor(sf::Color::White);
            dimTxt.setPosition({x1 * ts + 6.0f, y1 * ts + 4.0f});
            window.draw(dimTxt);
        }
    }

    // 7. Превью линии Line Tool
    if (_lineDragActive) {
        auto tiles = getLineTiles(_lineStartTile.x, _lineStartTile.y, _lineCurTile.x, _lineCurTile.y);
        for (const auto& pt : tiles) {
            sf::RectangleShape lineBlock({ts, ts});
            lineBlock.setPosition({pt.x * ts, pt.y * ts});
            lineBlock.setFillColor(sf::Color(33, 150, 243, 90));
            lineBlock.setOutlineColor(sf::Color(100, 200, 255, 230));
            lineBlock.setOutlineThickness(1.5f);
            window.draw(lineBlock);
        }
    }

    // 8. Превью выделения Selection (Ctrl+Drag)
    if (_selectionActive) {
        int x1 = std::min(_selStart.x, _selEnd.x);
        int x2 = std::max(_selStart.x, _selEnd.x);
        int y1 = std::min(_selStart.y, _selEnd.y);
        int y2 = std::max(_selStart.y, _selEnd.y);
        float sw = (x2 - x1 + 1) * ts;
        float sh = (y2 - y1 + 1) * ts;

        sf::RectangleShape selRect({sw, sh});
        selRect.setPosition({x1 * ts, y1 * ts});
        selRect.setFillColor(sf::Color(255, 215, 0, 45));
        selRect.setOutlineColor(sf::Color(255, 215, 0, 240));
        selRect.setOutlineThickness(2.0f);
        window.draw(selRect);
    }

    // 9. Превью вставки Paste Mode
    if (_pasteModeActive && _clipboard.hasData) {
        for (const auto& item : _clipboard.items) {
            int tx = _lastHoverTile.x + item.relX;
            int ty = _lastHoverTile.y + item.relY;
            sf::RectangleShape pTile({ts, ts});
            pTile.setPosition({tx * ts, ty * ts});
            if (item.tile == 1) pTile.setFillColor(sf::Color(76, 175, 80, 140));
            else if (item.tile == 2) pTile.setFillColor(sf::Color(90, 110, 140, 140));
            else if (item.doorTarget > 0) pTile.setFillColor(sf::Color(0, 229, 255, 140));
            else pTile.setFillColor(sf::Color(100, 100, 100, 60));
            pTile.setOutlineColor(sf::Color(255, 255, 255, 200));
            pTile.setOutlineThickness(1.0f);
            window.draw(pTile);
        }
    }

    // 10. Линейка прыжка (Jump Arc Ruler)
    drawJumpArc(window, arena, font);

    // 11. Подсветка курсора и кисти
    if (!_boxDragActive && !_lineDragActive && !_pasteModeActive &&
        _lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
        _lastHoverTile.y >= 0 && _lastHoverTile.y < arena.getHeight()) {
        int half = _brushSize / 2;
        float bW = static_cast<float>(_brushSize) * ts;
        float bH = static_cast<float>(_brushSize) * ts;
        float bX = static_cast<float>(_lastHoverTile.x - half) * ts;
        float bY = static_cast<float>(_lastHoverTile.y - half) * ts;

        sf::RectangleShape brushRect({bW, bH});
        brushRect.setPosition({bX, bY});
        brushRect.setOutlineThickness(2.0f);

        if (_tool == EditorTool::PLATFORM) {
            brushRect.setFillColor(sf::Color(76, 175, 80, 80));
            brushRect.setOutlineColor(sf::Color(76, 175, 80, 240));
        } else if (_tool == EditorTool::WALL) {
            brushRect.setFillColor(sf::Color(33, 150, 243, 80));
            brushRect.setOutlineColor(sf::Color(33, 150, 243, 240));
        } else if (_tool == EditorTool::DOOR) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(0, 229, 255, 90));
            brushRect.setOutlineColor(sf::Color(0, 229, 255, 255));
        } else if (_tool == EditorTool::PLAYER_SPAWN) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(0, 230, 118, 90));
            brushRect.setOutlineColor(sf::Color(0, 230, 118, 255));
        } else if (_tool == EditorTool::ENEMY_SPAWN) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(160, 160, 160, 90));
            brushRect.setOutlineColor(sf::Color(210, 210, 210, 255));
        } else if (_tool == EditorTool::ENEMY_TANK) {
            brushRect.setSize({ts * 1.5f, ts * 1.5f});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts - ts * 0.25f, static_cast<float>(_lastHoverTile.y) * ts - ts * 0.5f});
            brushRect.setFillColor(sf::Color(255, 50, 50, 90));
            brushRect.setOutlineColor(sf::Color(255, 80, 80, 255));
        } else if (_tool == EditorTool::ENEMY_DUELIST) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(170, 70, 230, 90));
            brushRect.setOutlineColor(sf::Color(200, 90, 255, 255));
        } else if (_tool == EditorTool::ENEMY_DUELIST_PASSIVE) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(35, 150, 225, 90));
            brushRect.setOutlineColor(sf::Color(45, 185, 245, 255));
        } else if (_tool == EditorTool::ERASER) {
            brushRect.setSize({bW, bH});
            brushRect.setPosition({bX, bY});
            brushRect.setFillColor(sf::Color(180, 30, 30, 60));
            brushRect.setOutlineColor(sf::Color(255, 50, 50, 220));
        } else if (_tool == EditorTool::FLOOD_FILL) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(255, 180, 0, 90));
            brushRect.setOutlineColor(sf::Color(255, 200, 0, 255));
        } else if (_tool == EditorTool::EYEDROPPER) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(200, 200, 255, 90));
            brushRect.setOutlineColor(sf::Color(255, 255, 255, 255));
        } else if (_tool == EditorTool::BOX_FILL || _tool == EditorTool::LINE) {
            brushRect.setSize({ts, ts});
            brushRect.setPosition({static_cast<float>(_lastHoverTile.x) * ts, static_cast<float>(_lastHoverTile.y) * ts});
            brushRect.setFillColor(sf::Color(100, 220, 255, 80));
            brushRect.setOutlineColor(sf::Color(100, 220, 255, 240));
        }
        window.draw(brushRect);
    }
}

void MapEditor::drawUI(sf::RenderWindow& window, const Arena& arena, const sf::Font* font,
                       const sf::View& camera) {
    if (!_active || _isPlaytesting) return;

    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    // 1. Верхняя панель инструментов
    sf::RectangleShape bar({winW, 118.0f});
    bar.setPosition({0.0f, 0.0f});
    bar.setFillColor(sf::Color(16, 20, 28, 245));
    bar.setOutlineColor(sf::Color(65, 80, 105, 200));
    bar.setOutlineThickness(1.0f);
    window.draw(bar);

    if (font) {
        // Заголовок
        sf::Text title(*font, toSfString("[ РЕДАКТОР КАРТ ]  (Tab/P - Тест с курсора | Ctrl+Z/Y - Undo/Redo | Ctrl+S / F7 - Сохранить)"), 16);
        title.setFillColor(sf::Color(255, 215, 0));
        title.setPosition({20.0f, 6.0f});
        window.draw(title);

        // Индикатор комнат и зума
        std::string roomInfo = "<< [PageDown] Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                               " / " + std::to_string(arena.getTotalRooms()) + 
                               " (" + arena.getCurrentRoomPath() + ") [PageUp] >>    " +
                               "Размер: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight()) + " (Alt+Стрелки)    " +
                               "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "% (Z / C / Wheel)";
        sf::Text roomText(*font, toSfString(roomInfo), 13);
        roomText.setFillColor(sf::Color(100, 220, 255));
        roomText.setPosition({20.0f, 30.0f});
        window.draw(roomText);

        // Инструменты и кисть
        std::string toolsStr = "Инструменты: ";
        toolsStr += (_tool == EditorTool::PLATFORM ? ">> [1] Пол <<  " : "[1] Пол  ");
        toolsStr += (_tool == EditorTool::WALL ? ">> [2] Стена <<  " : "[2] Стена  ");
        toolsStr += (_tool == EditorTool::DOOR ? (">> [3] Дверь(R" + std::to_string(_doorTargetRoom) + ") <<  ") : "[3] Дверь  ");
        toolsStr += (_tool == EditorTool::PLAYER_SPAWN ? ">> [4] Игрок <<  " : "[4] Игрок  ");
        toolsStr += (_tool == EditorTool::ENEMY_SPAWN ? ">> [5] Обычн <<  " : "[5] Обычн  ");
        toolsStr += (_tool == EditorTool::ENEMY_TANK ? ">> [6] Танк <<  " : "[6] Танк  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST ? ">> [7] Дуэль-Агр <<  " : "[7] Дуэль-Агр  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST_PASSIVE ? ">> [8] Дуэль-Пасс <<  " : "[8] Дуэль-Пасс  ");
        toolsStr += (_tool == EditorTool::ERASER ? ">> [9] Ластик <<  " : "[9] Ластик  ");
        toolsStr += (_tool == EditorTool::BOX_FILL ? ">> [X] Box <<  " : "[X] Box  ");
        toolsStr += (_tool == EditorTool::LINE ? ">> [L] Линия <<  " : "[L] Линия  ");
        toolsStr += (_tool == EditorTool::FLOOD_FILL ? ">> [G] Ведро <<  " : "[G] Ведро  ");
        toolsStr += (_tool == EditorTool::EYEDROPPER ? ">> [Q] Пипетка <<  " : "[Q] Пипетка  ");
        toolsStr += "| Кисть: [B] " + std::to_string(_brushSize) + "x" + std::to_string(_brushSize);

        sf::Text toolsText(*font, toSfString(toolsStr), 13);
        toolsText.setFillColor(sf::Color(220, 235, 255));
        toolsText.setPosition({20.0f, 54.0f});
        window.draw(toolsText);

        // Подсказки управления
        std::string hints = "Shift+Drag: Box Fill | Alt+Drag: Линия | Ctrl+Z/Y: Отмена/Повтор (" + 
                            std::to_string(_undoStack.size()) + ") | Ctrl+C/V: Копия/Вставка | J: Дуга прыжка (" +
                            (_showJumpRuler ? "ВКЛ" : "ВЫКЛ") + ") | Ctrl+G: Сетка (" + (_showGrid ? "ВКЛ" : "ВЫКЛ") + ")";
        sf::Text hintText(*font, toSfString(hints), 12);
        hintText.setFillColor(sf::Color(160, 185, 215));
        hintText.setPosition({20.0f, 82.0f});
        window.draw(hintText);

        // Статусное всплывающее сообщение
        if (!_statusMessage.empty() && _statusTimer > 0.0f) {
            sf::Text statusText(*font, toSfString(_statusMessage), 15);
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

            window.draw(msgBg);
            window.draw(statusText);
        }
    }

    // 2. Интерактивная миникарта
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
    window.draw(mmBg);

    if (font) {
        sf::Text mmTitle(*font, toSfString("МИНИКАРТА (Клик для прыжка)"), 12);
        mmTitle.setFillColor(sf::Color(160, 185, 215));
        mmTitle.setPosition({mmX + 8.0f, mmY + 5.0f});
        window.draw(mmTitle);
    }

    float mapW = arena.getWidth() * arena.getTileSize();
    float mapH = arena.getHeight() * arena.getTileSize();
    float innerW = mmW - 16.0f;
    float innerH = mmH - 30.0f;
    float scaleX = innerW / mapW;
    float scaleY = innerH / mapH;
    float scale = std::min(scaleX, scaleY);

    float offX = mmX + 8.0f;
    float offY = mmY + 24.0f;

    for (int y = 0; y < arena.getHeight(); ++y) {
        for (int x = 0; x < arena.getWidth(); ++x) {
            int tile = arena.getTile(x, y);
            if (tile == 0) continue;

            sf::RectangleShape dot({std::max(1.5f, arena.getTileSize() * scale), 
                                    std::max(1.5f, arena.getTileSize() * scale)});
            dot.setPosition({offX + x * arena.getTileSize() * scale, 
                             offY + y * arena.getTileSize() * scale});

            if (tile == 1) dot.setFillColor(sf::Color(76, 175, 80));
            else if (tile == 2) dot.setFillColor(sf::Color(90, 110, 140));
            else if (tile == 3) dot.setFillColor(sf::Color(0, 229, 255));

            window.draw(dot);
        }
    }

    sf::Vector2f pSpawn = arena.getSpawnPosition();
    sf::CircleShape pDot(3.0f);
    pDot.setFillColor(sf::Color(0, 230, 118));
    pDot.setPosition({offX + pSpawn.x * scale - 3.0f, offY + pSpawn.y * scale - 3.0f});
    window.draw(pDot);

    for (const auto& sp : arena.getEnemySpawns()) {
        sf::CircleShape eDot(sp.type == 1 ? 4.0f : 3.0f);
        if (sp.type == 1) eDot.setFillColor(sf::Color(255, 60, 60));
        else if (sp.type == 2) eDot.setFillColor(sf::Color(190, 90, 255));
        else if (sp.type == 3) eDot.setFillColor(sf::Color(45, 185, 245));
        else eDot.setFillColor(sf::Color(190, 190, 190));
        eDot.setPosition({offX + sp.position.x * scale - 3.0f, offY + sp.position.y * scale - 3.0f});
        window.draw(eDot);
    }

    sf::Vector2f camCenter = camera.getCenter();
    sf::Vector2f camSize = camera.getSize();
    float camLeft = camCenter.x - camSize.x / 2.0f;
    float camTop = camCenter.y - camSize.y / 2.0f;

    sf::RectangleShape camFrame({camSize.x * scale, camSize.y * scale});
    camFrame.setPosition({offX + camLeft * scale, offY + camTop * scale});
    camFrame.setFillColor(sf::Color::Transparent);
    camFrame.setOutlineColor(sf::Color(255, 215, 0, 220));
    camFrame.setOutlineThickness(1.5f);
    window.draw(camFrame);

    // 3. Нижняя панель: палитра и галерея комнат
    float palX = 15.0f;
    float palH = 112.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX - palX - 12.0f;

    sf::RectangleShape palBg({palW, palH});
    palBg.setPosition({palX, palY});
    palBg.setFillColor(sf::Color(15, 20, 28, 245));
    palBg.setOutlineColor(sf::Color(65, 80, 110, 220));
    palBg.setOutlineThickness(1.5f);
    window.draw(palBg);

    if (font) {
        sf::Text palTitle(*font, toSfString("ПАЛИТРА И ГАЛЕРЕЯ КОМНАТ"), 13);
        palTitle.setFillColor(sf::Color(255, 215, 0));
        palTitle.setPosition({palX + 10.0f, palY + 6.0f});
        window.draw(palTitle);

        sf::Text doorLabel(*font, toSfString("ЦЕЛЬ ДВЕРИ:"), 12);
        doorLabel.setFillColor(_tool == EditorTool::DOOR ? sf::Color(0, 255, 255) : sf::Color(140, 180, 200));
        doorLabel.setPosition({palX + 220.0f, palY + 6.0f});
        window.draw(doorLabel);

        // Кнопка [ < ]
        sf::RectangleShape btnPrev({20.0f, 20.0f});
        btnPrev.setPosition({palX + 328.0f, palY + 4.0f});
        btnPrev.setFillColor(sf::Color(30, 45, 65));
        btnPrev.setOutlineColor(sf::Color(0, 200, 255));
        btnPrev.setOutlineThickness(1.0f);
        window.draw(btnPrev);
        sf::Text btnPrevT(*font, "<", 12);
        btnPrevT.setFillColor(sf::Color::White);
        btnPrevT.setPosition({palX + 334.0f, palY + 4.0f});
        window.draw(btnPrevT);

        // Бейдж текущей целевой комнаты
        sf::RectangleShape badgeCur({40.0f, 20.0f});
        badgeCur.setPosition({palX + 351.0f, palY + 4.0f});
        badgeCur.setFillColor(sf::Color(0, 100, 140));
        badgeCur.setOutlineColor(sf::Color(0, 255, 255));
        badgeCur.setOutlineThickness(1.0f);
        window.draw(badgeCur);
        sf::Text badgeCurT(*font, "R" + std::to_string(_doorTargetRoom), 12);
        badgeCurT.setFillColor(sf::Color(0, 255, 255));
        badgeCurT.setPosition({palX + 357.0f, palY + 4.0f});
        window.draw(badgeCurT);

        // Кнопка [ > ]
        sf::RectangleShape btnNext({20.0f, 20.0f});
        btnNext.setPosition({palX + 394.0f, palY + 4.0f});
        btnNext.setFillColor(sf::Color(30, 45, 65));
        btnNext.setOutlineColor(sf::Color(0, 200, 255));
        btnNext.setOutlineThickness(1.0f);
        window.draw(btnNext);
        sf::Text btnNextT(*font, ">", 12);
        btnNextT.setFillColor(sf::Color::White);
        btnNextT.setPosition({palX + 400.0f, palY + 4.0f});
        window.draw(btnNextT);

        // 10 Цифровых кнопок комнат
        for (int r = 1; r <= 10; ++r) {
            float rx = palX + 420.0f + (r - 1) * 25.0f;
            float ry = palY + 4.0f;
            bool isCurrentTarget = (_doorTargetRoom == r);

            sf::RectangleShape rBtn({22.0f, 20.0f});
            rBtn.setPosition({rx, ry});
            rBtn.setFillColor(isCurrentTarget ? sf::Color(0, 200, 255) : sf::Color(25, 36, 50));
            rBtn.setOutlineColor(isCurrentTarget ? sf::Color(255, 255, 255) : sf::Color(60, 80, 105));
            rBtn.setOutlineThickness(1.0f);
            window.draw(rBtn);

            sf::Text rTxt(*font, std::to_string(r), 11);
            rTxt.setFillColor(isCurrentTarget ? sf::Color(10, 15, 25) : sf::Color(180, 200, 220));
            float textOffset = (r >= 10) ? 4.0f : 7.0f;
            rTxt.setPosition({rx + textOffset, ry + 3.0f});
            window.draw(rTxt);
        }

        // Кнопка [Клон в R...]
        sf::RectangleShape btnDup({100.0f, 20.0f});
        btnDup.setPosition({palX + 680.0f, palY + 4.0f});
        btnDup.setFillColor(sf::Color(35, 50, 70));
        btnDup.setOutlineColor(sf::Color(0, 200, 255));
        btnDup.setOutlineThickness(1.0f);
        window.draw(btnDup);
        sf::Text btnDupT(*font, toSfString("Клон в R" + std::to_string(_doorTargetRoom)), 11);
        btnDupT.setFillColor(sf::Color(150, 230, 255));
        btnDupT.setPosition({palX + 685.0f, palY + 3.0f});
        window.draw(btnDupT);

        // Кнопка [Очистить]
        sf::RectangleShape btnClr({80.0f, 20.0f});
        btnClr.setPosition({palX + 785.0f, palY + 4.0f});
        btnClr.setFillColor(sf::Color(65, 25, 25));
        btnClr.setOutlineColor(sf::Color(255, 80, 80));
        btnClr.setOutlineThickness(1.0f);
        window.draw(btnClr);
        sf::Text btnClrT(*font, toSfString("Очистить"), 11);
        btnClrT.setFillColor(sf::Color(255, 120, 120));
        btnClrT.setPosition({palX + 795.0f, palY + 3.0f});
        window.draw(btnClrT);

        // Кнопка [F5 - Обновить]
        float reloadX = std::max(palX + 875.0f, palX + palW - 120.0f);
        sf::RectangleShape btnReload({115.0f, 20.0f});
        btnReload.setPosition({reloadX, palY + 4.0f});
        btnReload.setFillColor(sf::Color(25, 40, 60));
        btnReload.setOutlineColor(sf::Color(100, 220, 255));
        btnReload.setOutlineThickness(1.0f);
        window.draw(btnReload);
        sf::Text btnReloadT(*font, toSfString("[F5 - Обновить]"), 11);
        btnReloadT.setFillColor(sf::Color(120, 230, 255));
        btnReloadT.setPosition({reloadX + 8.0f, palY + 4.0f});
        window.draw(btnReloadT);
    }

    // Отрисовка карточек палитры
    float startX = palX + 10.0f;
    float cardY = palY + 28.0f;
    float cardW = 60.0f;
    float cardH = 76.0f;

    for (size_t i = 0; i < _paletteSprites.size(); ++i) {
        float cardX = startX + i * 65.0f;
        if (cardX + cardW > palX + palW) break;

        bool isSelected = (_tool == _paletteSprites[i].tool);
        sf::RectangleShape card({cardW, cardH});
        card.setPosition({cardX, cardY});
        card.setFillColor(isSelected ? sf::Color(35, 55, 80) : sf::Color(20, 26, 36));
        card.setOutlineColor(isSelected ? sf::Color(255, 215, 0) : sf::Color(60, 75, 100));
        card.setOutlineThickness(isSelected ? 2.5f : 1.0f);
        window.draw(card);

        if (font) {
            std::string hotkeyStr = "";
            if (_paletteSprites[i].tool == EditorTool::PLATFORM) hotkeyStr = "[1]";
            else if (_paletteSprites[i].tool == EditorTool::WALL && _paletteSprites[i].filename.find("02_wall") != std::string::npos) hotkeyStr = "[2]";
            else if (_paletteSprites[i].tool == EditorTool::DOOR) hotkeyStr = "[3]";
            else if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) hotkeyStr = "[4]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) hotkeyStr = "[5]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) hotkeyStr = "[6]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) hotkeyStr = "[7]";
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) hotkeyStr = "[8]";
            else if (_paletteSprites[i].tool == EditorTool::ERASER) hotkeyStr = "[9]";
            else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) hotkeyStr = "[X]";
            else if (_paletteSprites[i].tool == EditorTool::LINE) hotkeyStr = "[L]";
            else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) hotkeyStr = "[G]";
            else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) hotkeyStr = "[Q]";

            if (!hotkeyStr.empty()) {
                sf::Text hkTxt(*font, hotkeyStr, 10);
                hkTxt.setFillColor(isSelected ? sf::Color(255, 235, 120) : sf::Color(140, 160, 180));
                hkTxt.setPosition({cardX + 4.0f, cardY + 3.0f});
                window.draw(hkTxt);
            }
        }

        float iconX = cardX + (cardW - 32.0f) / 2.0f;
        float iconY = cardY + 17.0f;

        if (_paletteSprites[i].sprite) {
            _paletteSprites[i].sprite->setPosition({iconX, iconY});
            window.draw(*_paletteSprites[i].sprite);
        } else {
            sf::RectangleShape colorBox({32.0f, 24.0f});
            colorBox.setPosition({iconX, iconY});
            if (_paletteSprites[i].tool == EditorTool::PLATFORM) colorBox.setFillColor(sf::Color(76, 175, 80));
            else if (_paletteSprites[i].tool == EditorTool::WALL) colorBox.setFillColor(sf::Color(90, 110, 140));
            else if (_paletteSprites[i].tool == EditorTool::DOOR) colorBox.setFillColor(sf::Color(0, 229, 255));
            else if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) colorBox.setFillColor(sf::Color(0, 230, 118));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) colorBox.setFillColor(sf::Color(180, 180, 180));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) colorBox.setFillColor(sf::Color(240, 50, 50));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) colorBox.setFillColor(sf::Color(170, 70, 230));
            else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) colorBox.setFillColor(sf::Color(45, 185, 245));
            else if (_paletteSprites[i].tool == EditorTool::ERASER) colorBox.setFillColor(sf::Color(60, 60, 60));
            else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) colorBox.setFillColor(sf::Color(100, 180, 255));
            else if (_paletteSprites[i].tool == EditorTool::LINE) colorBox.setFillColor(sf::Color(80, 220, 160));
            else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) colorBox.setFillColor(sf::Color(255, 180, 0));
            else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) colorBox.setFillColor(sf::Color(200, 100, 255));

            colorBox.setOutlineColor(sf::Color(200, 220, 240, 150));
            colorBox.setOutlineThickness(1.0f);
            window.draw(colorBox);

            if (font) {
                std::string letter = "";
                if (_paletteSprites[i].tool == EditorTool::PLAYER_SPAWN) letter = "P";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_SPAWN) letter = "N";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_TANK) letter = "T";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST) letter = "DA";
                else if (_paletteSprites[i].tool == EditorTool::ENEMY_DUELIST_PASSIVE) letter = "DP";
                else if (_paletteSprites[i].tool == EditorTool::DOOR) letter = "D";
                else if (_paletteSprites[i].tool == EditorTool::ERASER) letter = "X";
                else if (_paletteSprites[i].tool == EditorTool::BOX_FILL) letter = "[]";
                else if (_paletteSprites[i].tool == EditorTool::LINE) letter = "/";
                else if (_paletteSprites[i].tool == EditorTool::FLOOD_FILL) letter = "F";
                else if (_paletteSprites[i].tool == EditorTool::EYEDROPPER) letter = "Q";

                if (!letter.empty()) {
                    sf::Text lTxt(*font, letter, 12);
                    lTxt.setFillColor(sf::Color::White);
                    lTxt.setPosition({iconX + 6.0f, iconY + 3.0f});
                    window.draw(lTxt);
                }
            }
        }

        if (font) {
            std::string label = _paletteSprites[i].name;
            if (label.size() > 9) label = label.substr(0, 8) + ".";
            sf::Text cardTxt(*font, toSfString(label), 10);
            cardTxt.setFillColor(isSelected ? sf::Color(255, 235, 120) : sf::Color(180, 195, 215));
            cardTxt.setPosition({cardX + 4.0f, cardY + 56.0f});
            window.draw(cardTxt);
        }
    }
}
