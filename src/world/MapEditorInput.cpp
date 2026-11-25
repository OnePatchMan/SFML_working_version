#include "world/MapEditor.h"
#include "world/MapEditorInternal.h"
#include <iostream>
#include <cmath>
#include <algorithm>

using namespace map_editor_internal;

void MapEditor::handlePlaytestInput(const std::optional<sf::Event>& event, sf::RenderWindow& window, sf::View& camera) {
    if (!event.has_value()) return;
    if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
        if (kp->code == sf::Keyboard::Key::Tab || kp->code == sf::Keyboard::Key::P || kp->code == sf::Keyboard::Key::F6) {
            stopPlaytest(window, camera);
        }
    }
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
        } else if (kp->code == sf::Keyboard::Key::Num0 || kp->code == sf::Keyboard::Key::Numpad0) {
            _tool = EditorTool::LAMP;
            _statusMessage = "Инструмент: Люминесцентная лампа (L - освещение лаборатории)";
            _statusTimer = 2.0f;
        } else if (kp->code == sf::Keyboard::Key::B && !kp->shift) {
            _tool = EditorTool::ENEMY_BOSS;
            _statusMessage = "Инструмент: Босс 1 (Бледный Мутант, 4x3 тайла, Горилла без шеи и головы)";
            _statusTimer = 2.0f;
        }

        // Кисть и настройка двери
        if (kp->code == sf::Keyboard::Key::B && kp->shift) {
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
            if (handleLeftToolbarClick(mPixel, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y))) return;
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
                    int nextType = 0;
                    if (spawns[i].type == 0) nextType = 1;
                    else if (spawns[i].type == 1) nextType = 2;
                    else if (spawns[i].type == 2) nextType = 3;
                    else if (spawns[i].type == 3) nextType = 5;
                    else nextType = 0;
                    spawns[i].type = nextType;
                    arena.setEnemySpawns(spawns);
                    std::string tName = (nextType == 1) ? "Жирный (Tank)" :
                                        (nextType == 2) ? "Дуэлянт Агрессивный" :
                                        (nextType == 3) ? "Дуэлянт Пассивный" :
                                        (nextType == 5) ? "Босс 1 (Бледный Мутант)" : "Обычный (Normal)";
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
    float mmW = 260.0f;
    float mmX = winW - mmW - 20.0f;
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
            _currentTileCode = _paletteSprites[i].tileCode;
            if (_currentTileCode == 0) {
                if (_tool == EditorTool::PLATFORM) _currentTileCode = 1;
                else if (_tool == EditorTool::WALL) _currentTileCode = 2;
                else if (_tool == EditorTool::LAMP) _currentTileCode = 4;
            }
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
    if (mousePixel.y >= static_cast<int>(window.getSize().y) - 124 ||
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
        _tool == EditorTool::ENEMY_DUELIST || _tool == EditorTool::ENEMY_DUELIST_PASSIVE ||
        _tool == EditorTool::ENEMY_BOSS) {
        arena.setTile(tx, ty, 0);
        int enemyType = (_tool == EditorTool::ENEMY_TANK) ? 1 : 
                        (_tool == EditorTool::ENEMY_DUELIST) ? 2 : 
                        (_tool == EditorTool::ENEMY_DUELIST_PASSIVE) ? 3 :
                        (_tool == EditorTool::ENEMY_BOSS) ? 5 : 0;
        arena.setEnemySpawnAt(tx, ty, enemyType);
        std::string tName = (enemyType == 1) ? "Жирный (Tank)" : 
                            (enemyType == 2) ? "Дуэлянт Агрессивный" : 
                            (enemyType == 3) ? "Дуэлянт Пассивный" :
                            (enemyType == 5) ? "Босс 1 (Бледный Мутант)" : "Обычный (Normal)";
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
    if (_tool == EditorTool::LAMP) {
        arena.setTile(tx, ty, 4);
        arena.scanLamps();
        _statusMessage = "Люминесцентная лампа установлена на [" + std::to_string(tx) + ", " + std::to_string(ty) + "]";
        _statusTimer = 2.0f;
        return;
    }

    int tileVal = (_tool == EditorTool::PLATFORM ? 1 : (_currentTileCode > 0 ? _currentTileCode : 2));
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


bool MapEditor::handleLeftToolbarClick(const sf::Vector2i& mousePixel, float winW, float winH) {
    float startY = 126.0f;
    float btnH = 24.0f;
    float btnGap = 3.0f;
    float totalH = static_cast<float>(s_ingameLeftButtons.size()) * (btnH + btnGap) + 8.0f;

    sf::Vector2f mPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
    if (mPos.x < 6.0f || mPos.x > 60.0f || mPos.y < startY || mPos.y > startY + totalH) {
        return false;
    }

    for (size_t i = 0; i < s_ingameLeftButtons.size(); ++i) {
        float by = startY + 4.0f + i * (btnH + btnGap);
        sf::FloatRect btnRect({10.0f, by}, {46.0f, btnH});
        if (btnRect.contains(mPos)) {
            _tool = s_ingameLeftButtons[i].tool;
            if (_tool == EditorTool::PLATFORM) _currentTileCode = 1;
            else if (_tool == EditorTool::WALL) _currentTileCode = 2;
            else if (_tool == EditorTool::LAMP) _currentTileCode = 4;
            else if (_tool == EditorTool::DOOR) _currentTileCode = 3;
            _statusMessage = "Инструмент: " + s_ingameLeftButtons[i].label;
            _statusTimer = 2.0f;
            return true;
        }
    }
    return false;
}
