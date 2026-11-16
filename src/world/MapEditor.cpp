#include "world/MapEditor.h"
#include <iostream>
#include <cmath>
#include <algorithm>

MapEditor::MapEditor()
    : _active(false), _tool(EditorTool::PLATFORM), _brushSize(1), _viewScale(1.0f),
      _cameraPos(0.0f, 0.0f), _camSpeed(1200.0f), _isPanning(false)
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
    _active = !_active;
    if (_active) {
        window.setMouseCursorVisible(true);
        _baseCameraSize = sf::Vector2f(static_cast<float>(window.getSize().x),
                                       static_cast<float>(window.getSize().y));
        _viewScale = 1.0f;
        camera.setSize(_baseCameraSize);
        _cameraPos = player.position;
        camera.setCenter(_cameraPos);

        _statusMessage = "Режим редактора комнат [1..10]! WASD/MMB - Камера, Shift - Турбо, Z/X - Зум, PageUp/Down - Комнаты";
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
    saveCurrentRoom(arena);
    arena.nextRoom();
    player.reset(arena.getSpawnPosition());
    enemy.spawnPosition = arena.getEnemySpawnPosition();
    enemy.reset();
    _cameraPos = player.position;
    camera.setCenter(_cameraPos);

    _statusMessage = "Загружена Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                     " / " + std::to_string(arena.getTotalRooms()) + " (" + arena.getCurrentRoomPath() + ")";
    _statusTimer = 3.5f;
}

void MapEditor::prevRoom(Arena& arena, Player& player, Enemy& enemy, sf::View& camera) {
    saveCurrentRoom(arena);
    arena.prevRoom();
    player.reset(arena.getSpawnPosition());
    enemy.spawnPosition = arena.getEnemySpawnPosition();
    enemy.reset();
    _cameraPos = player.position;
    camera.setCenter(_cameraPos);

    _statusMessage = "Загружена Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                     " / " + std::to_string(arena.getTotalRooms()) + " (" + arena.getCurrentRoomPath() + ")";
    _statusTimer = 3.5f;
}

void MapEditor::handleInput(const std::optional<sf::Event>& event, Arena& arena, 
                            Player& player, Enemy& enemy, sf::RenderWindow& window,
                            sf::View& camera) {
    if (!_active || !event.has_value()) return;

    if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
        bool ctrlHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
        bool altHeld = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);

        // Инструменты
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
            _statusMessage = "Инструмент: Враг (Дуэлянт Агрессивный - Фиолетовый, прессинг, длинный зацеп)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::Num8 || kp->code == sf::Keyboard::Key::Numpad8) {
            _tool = EditorTool::ENEMY_DUELIST_PASSIVE;
            _statusMessage = "Инструмент: Враг (Дуэлянт Пассивный - Голубой, спейсинг, невидимость 1с/7с, паниш)";
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
        if (kp->code == sf::Keyboard::Key::Z) {
            zoomIn(camera);
        } else if (kp->code == sf::Keyboard::Key::X) {
            zoomOut(camera);
        } else if (kp->code == sf::Keyboard::Key::C && !ctrlHeld) {
            resetZoom(camera);
        }

        // Изменение размера холста (Alt + Стрелки)
        if (altHeld) {
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
        } else {
            if (wheel->delta > 0) {
                int t = static_cast<int>(_tool) - 1;
                if (t < 1) t = 5;
                _tool = static_cast<EditorTool>(t);
            } else if (wheel->delta < 0) {
                int t = static_cast<int>(_tool) + 1;
                if (t > 5) t = 1;
                _tool = static_cast<EditorTool>(t);
            }
        }
    }

    // Обработка нажатий мыши
    if (const auto* mbPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (mbPressed->button == sf::Mouse::Button::Middle) {
            _isPanning = true;
            _lastMousePixel = sf::Mouse::getPosition(window);
        } else if (mbPressed->button == sf::Mouse::Button::Left) {
            sf::Vector2i mPixel = sf::Mouse::getPosition(window);
            if (handlePaletteClick(mPixel, arena, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y))) return;
            if (handleMinimapClick(mPixel, arena, camera)) return;
        } else if (mbPressed->button == sf::Mouse::Button::Right) {
            if (_tool == EditorTool::DOOR) {
                sf::Vector2i mPixel = sf::Mouse::getPosition(window);
                sf::Vector2f mWorld = window.mapPixelToCoords(mPixel, camera);
                int tx = static_cast<int>(std::floor(mWorld.x / arena.getTileSize()));
                int ty = static_cast<int>(std::floor(mWorld.y / arena.getTileSize()));
                if (arena.isDoor(tx, ty)) {
                    int curTgt = arena.getDoorTargetRoom(tx, ty);
                    int nextTgt = (curTgt % arena.getTotalRooms()) + 1;
                    arena.setDoor(tx, ty, nextTgt);
                    _statusMessage = "Целевая комната двери изменена: Комната " + std::to_string(nextTgt);
                    _statusTimer = 2.0f;
                }
            }
        }
    }
    if (const auto* mbReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
        if (mbReleased->button == sf::Mouse::Button::Middle) {
            _isPanning = false;
        }
    }
}

bool MapEditor::handlePaletteClick(const sf::Vector2i& mousePixel, Arena& arena, float winW, float winH) {
    float mmW = std::min(240.0f, winW * 0.22f);
    float mmX = winW - mmW - 15.0f;
    float palX = 15.0f;
    float palH = 112.0f;
    float palY = winH - palH - 12.0f;
    float palW = mmX - palX - 12.0f;

    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    // Проверка попадания в область всей нижней панели
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

    // 4. Кнопка [F5 - Обновить спрайты]
    float reloadX = std::max(palX + 850.0f, palX + palW - 135.0f);
    sf::FloatRect reloadBtn({reloadX, palY + 4.0f}, {125.0f, 20.0f});
    if (reloadBtn.contains(mPos)) {
        scanTileSprites();
        _statusMessage = "Спрайты обновлены из assets/tiles/!";
        _statusTimer = 2.0f;
        return true;
    }

    // 5. Клик по карточкам спрайтов/инструментов
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
    if (!_active) return;

    if (_statusTimer > 0.0f) {
        _statusTimer -= dt;
        if (_statusTimer <= 0.0f) _statusMessage.clear();
    }

    // Не обрабатывать глобальные состояния клавиатуры/мыши, если окно не в фокусе.
    // sf::Mouse::isButtonPressed / sf::Keyboard::isKeyPressed — глобальные запросы ОС,
    // срабатывают даже когда пользователь кликает в другом окне.
    if (!window.hasFocus()) return;

    // Перетаскивание карты средней кнопкой мыши
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

    if (tx >= 0 && tx < arena.getWidth() && ty >= 0 && ty < arena.getHeight()) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            applyToolAt(tx, ty, arena, player, enemy);
        } else if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Right)) {
            if (_tool != EditorTool::DOOR) {
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
        _statusMessage = "Комната " + std::to_string(arena.getCurrentRoomIndex()) + " успешно сохранена в " + arena.getCurrentRoomPath() + "!";
        std::cout << "[MAP EDITOR] " << _statusMessage << std::endl;
    } else {
        _statusMessage = "Ошибка сохранения комнаты!";
        std::cerr << "[MAP EDITOR] " << _statusMessage << std::endl;
    }
    _statusTimer = 3.5f;
}

void MapEditor::drawWorld(sf::RenderWindow& window, const Arena& arena, const sf::Vector2f& mouseWorld,
                          const sf::Font* font) {
    if (!_active) return;

    float ts = arena.getTileSize();
    sf::View view = window.getView();
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();

    int startX = std::max(0, static_cast<int>((center.x - size.x / 2.0f) / ts) - 1);
    int endX = std::min(arena.getWidth(), static_cast<int>((center.x + size.x / 2.0f) / ts) + 2);
    int startY = std::max(0, static_cast<int>((center.y - size.y / 2.0f) / ts) - 1);
    int endY = std::min(arena.getHeight(), static_cast<int>((center.y + size.y / 2.0f) / ts) + 2);

    // Сетка тайлов
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

    // Маркер спавна игрока
    sf::Vector2f pSpawn = arena.getSpawnPosition();
    float pBoxX = std::round(pSpawn.x / ts) * ts;
    float pBoxY = (std::round((pSpawn.y + 30.0f) / ts) - 1.0f) * ts;
    sf::RectangleShape pBox({ts, ts});
    pBox.setPosition({pBoxX, pBoxY});
    pBox.setFillColor(sf::Color(0, 230, 118, 50));
    pBox.setOutlineColor(sf::Color(0, 230, 118, 220));
    pBox.setOutlineThickness(2.0f);
    window.draw(pBox);

    // Маркеры спавна врагов (разные цвета и размеры по архетипам: Normal, Tank, Duelist)
    const auto& eSpawns = arena.getEnemySpawns();
    for (size_t i = 0; i < eSpawns.size(); ++i) {
        sf::Vector2f eSpawn = eSpawns[i].position;
        int eType = eSpawns[i].type; // 0 = Normal, 1 = Tank, 2 = Duelist
        float eBoxX = std::round(eSpawn.x / ts) * ts;
        float eBoxY = (std::round((eSpawn.y + 28.0f) / ts) - 1.0f) * ts;

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

    if (font) {
        sf::Text pText(*font, "P", 18);
        pText.setFillColor(sf::Color(0, 230, 118));
        pText.setPosition({pBoxX + ts * 0.35f, pBoxY + ts * 0.25f});
        window.draw(pText);
    }

    // Подсветка курсора и кисти
    if (_lastHoverTile.x >= 0 && _lastHoverTile.x < arena.getWidth() &&
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
        }
        window.draw(brushRect);
    }
}

void MapEditor::drawUI(sf::RenderWindow& window, const Arena& arena, const sf::Font* font,
                       const sf::View& camera) {
    if (!_active) return;

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
        sf::Text title(*font, toSfString("[ РЕДАКТОР ОГРОМНЫХ КАРТ И КОМНАТ ]  (F6 - Выход в бой | Ctrl+S или F7 - Сохранить)"), 17);
        title.setFillColor(sf::Color(255, 215, 0));
        title.setPosition({20.0f, 6.0f});
        window.draw(title);

        // Индикатор комнат и зума
        std::string roomInfo = "<< [PageDown] Комната " + std::to_string(arena.getCurrentRoomIndex()) + 
                               " / " + std::to_string(arena.getTotalRooms()) + 
                               " (" + arena.getCurrentRoomPath() + ") [PageUp] >>    " +
                               "Размер: " + std::to_string(arena.getWidth()) + "x" + std::to_string(arena.getHeight()) + " (Alt+Стрелки)    " +
                               "Зум: " + std::to_string(static_cast<int>(100.0f / _viewScale)) + "% (Z / X / Ctrl+Wheel)";
        sf::Text roomText(*font, toSfString(roomInfo), 14);
        roomText.setFillColor(sf::Color(100, 220, 255));
        roomText.setPosition({20.0f, 32.0f});
        window.draw(roomText);

        // Инструменты и кисть
        std::string toolsStr = "Инструменты: ";
        toolsStr += (_tool == EditorTool::PLATFORM ? ">> [1] Пол (=) <<  " : "[1] Пол  ");
        toolsStr += (_tool == EditorTool::WALL ? ">> [2] Стена (#) <<  " : "[2] Стена  ");
        toolsStr += (_tool == EditorTool::DOOR ? (">> [3] Дверь (D->R" + std::to_string(_doorTargetRoom) + ") <<  ") : "[3] Дверь  ");
        toolsStr += (_tool == EditorTool::PLAYER_SPAWN ? ">> [4] Игрок (P) <<  " : "[4] Игрок  ");
        toolsStr += (_tool == EditorTool::ENEMY_SPAWN ? ">> [5] Обычный (Norm) <<  " : "[5] Обычный  ");
        toolsStr += (_tool == EditorTool::ENEMY_TANK ? ">> [6] Жирный (Tank) <<  " : "[6] Жирный  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST ? ">> [7] Дуэлянт-Агр <<  " : "[7] Дуэль-Агр  ");
        toolsStr += (_tool == EditorTool::ENEMY_DUELIST_PASSIVE ? ">> [8] Дуэлянт-Пасс <<  " : "[8] Дуэль-Пасс  ");
        toolsStr += (_tool == EditorTool::ERASER ? ">> [9] Ластик <<  " : "[9] Ластик  ");
        toolsStr += "|  Кисть: [B] " + std::to_string(_brushSize) + "x" + std::to_string(_brushSize);

        sf::Text toolsText(*font, toSfString(toolsStr), 14);
        toolsText.setFillColor(sf::Color(220, 235, 255));
        toolsText.setPosition({20.0f, 58.0f});
        window.draw(toolsText);

        // Подсказки управления
        sf::Text hintText(*font, toSfString("Управление: [ЛКМ] - Рисовать | [ПКМ] - Стереть | [СКМ / Drag] - Перемещение камеры | [Shift + WASD] - Турбо-полет"), 13);
        hintText.setFillColor(sf::Color(160, 175, 195));
        hintText.setPosition({20.0f, 86.0f});
        window.draw(hintText);

        // Статусное всплывающее сообщение
        if (!_statusMessage.empty() && _statusTimer > 0.0f) {
            sf::Text statusText(*font, toSfString(_statusMessage), 16);
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

    // 2. Интерактивная миникарта огромной арены в нижнем правом углу
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

    // Отрисовка тайлов на миникарте
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

            if (tile == 1) dot.setFillColor(sf::Color(76, 175, 80));      // Платформа (зеленый)
            else if (tile == 2) dot.setFillColor(sf::Color(90, 110, 140)); // Стена (серо-синий)
            else if (tile == 3) dot.setFillColor(sf::Color(0, 229, 255));  // Дверь (бирюзовый)

            window.draw(dot);
        }
    }

    // Точки спавна на миникарте
    sf::Vector2f pSpawn = arena.getSpawnPosition();
    sf::CircleShape pDot(3.0f);
    pDot.setFillColor(sf::Color(0, 230, 118));
    pDot.setPosition({offX + pSpawn.x * scale - 3.0f, offY + pSpawn.y * scale - 3.0f});
    window.draw(pDot);

    for (const auto& sp : arena.getEnemySpawns()) {
        sf::CircleShape eDot(sp.type == 1 ? 4.0f : 3.0f);
        if (sp.type == 1) eDot.setFillColor(sf::Color(255, 60, 60)); // Жирный (красный)
        else if (sp.type == 2) eDot.setFillColor(sf::Color(190, 90, 255)); // Дуэлянт (фиолетовый)
        else eDot.setFillColor(sf::Color(190, 190, 190)); // Обычный (серый)
        eDot.setPosition({offX + sp.position.x * scale - 3.0f, offY + sp.position.y * scale - 3.0f});
        window.draw(eDot);
    }

    // Рамка области видимости камеры на миникарте
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

    // -----------------------------------------------------------------
    // 3. НИЖНЯЯ ПАНЕЛЬ: ПАЛИТРА ЦВЕТОВ, СПРАЙТОВ И НАСТРОЙКИ ДВЕРЕЙ
    // -----------------------------------------------------------------
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
        // Заголовок палитры
        sf::Text palTitle(*font, toSfString("ПАЛИТРА ЦВЕТОВ И СПРАЙТОВ"), 13);
        palTitle.setFillColor(sf::Color(255, 215, 0));
        palTitle.setPosition({palX + 10.0f, palY + 6.0f});
        window.draw(palTitle);

        // Секция настройки дверей:
        sf::Text doorLabel(*font, toSfString("ЦЕЛЬ ДВЕРЕЙ:"), 12);
        doorLabel.setFillColor(_tool == EditorTool::DOOR ? sf::Color(0, 255, 255) : sf::Color(140, 180, 200));
        doorLabel.setPosition({palX + 235.0f, palY + 6.0f});
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

        // 10 Быстрых цифровых кнопок комнат: 1..10
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

        // Подсказка для дверей
        sf::Text doorHint(*font, toSfString("(ПКМ по двери меняет цель)"), 11);
        doorHint.setFillColor(sf::Color(130, 150, 175));
        doorHint.setPosition({palX + 678.0f, palY + 6.0f});
        window.draw(doorHint);

        // Кнопка [F5 - Обновить]
        float reloadX = std::max(palX + 850.0f, palX + palW - 135.0f);
        sf::RectangleShape btnReload({125.0f, 20.0f});
        btnReload.setPosition({reloadX, palY + 4.0f});
        btnReload.setFillColor(sf::Color(25, 40, 60));
        btnReload.setOutlineColor(sf::Color(100, 220, 255));
        btnReload.setOutlineThickness(1.0f);
        window.draw(btnReload);
        sf::Text btnReloadT(*font, toSfString("[F5 - Обновить]"), 11);
        btnReloadT.setFillColor(sf::Color(120, 230, 255));
        btnReloadT.setPosition({reloadX + 12.0f, palY + 4.0f});
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

        // Бейдж горячей клавиши
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

            if (!hotkeyStr.empty()) {
                sf::Text hkTxt(*font, hotkeyStr, 10);
                hkTxt.setFillColor(isSelected ? sf::Color(255, 235, 120) : sf::Color(140, 160, 180));
                hkTxt.setPosition({cardX + 4.0f, cardY + 3.0f});
                window.draw(hkTxt);
            }
        }

        // Отрисовка превью / спрайта / цветового блока
        float iconX = cardX + (cardW - 32.0f) / 2.0f;
        float iconY = cardY + 17.0f;

        if (_paletteSprites[i].sprite) {
            _paletteSprites[i].sprite->setPosition({iconX, iconY});
            window.draw(*_paletteSprites[i].sprite);
        } else {
            // Если текстура не подгрузилась — рисуем стилизованный цветовой блок
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
                if (!letter.empty()) {
                    sf::Text lTxt(*font, letter, 13);
                    lTxt.setFillColor(sf::Color::White);
                    lTxt.setPosition({iconX + 10.0f, iconY + 3.0f});
                    window.draw(lTxt);
                }
            }
        }

        // Подпись инструмента внизу карточки
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
