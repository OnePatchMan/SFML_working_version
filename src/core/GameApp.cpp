#include "core/GameApp.h"
#include "entities/PlayerConfig.h"
#include "entities/EnemyConfig.h"
#include "audio/SoundManager.h"
#include <iostream>
#include <cstdlib>

namespace {
    std::filesystem::file_time_type getFileTimeSafe(const std::string& p) {
        try {
            std::string resolved = resolveAssetPath(p);
            if (std::filesystem::exists(resolved)) {
                return std::filesystem::last_write_time(resolved);
            }
        } catch (...) {}
        return {};
    }
}

GameApp::GameApp()
    : _arena(ARENA_WIDTH, ARENA_HEIGHT),
      _player(_arena.getSpawnPosition())
{
    Arena::setActive(&_arena);
    initWindow();
    initSubsystems();
}

void GameApp::initWindow() {
    // Отключение принудительного VSync драйвера Mesa (AMD/Intel) и Nvidia
#if defined(_WIN32)
    _putenv("vblank_mode=0");
    _putenv("__GL_SYNC_TO_VBLANK=0");
#else
    setenv("vblank_mode", "0", 1);
    setenv("__GL_SYNC_TO_VBLANK", "0", 1);
#endif

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    _window.create(desktop, "FightingComboGame", sf::State::Fullscreen);
    _window.setMouseCursorVisible(true);

#if !defined(_WIN32)
    // Автоматический перевод в честный Fullscreen 2 в Hyprland для сокрытия панелей (Quickshell, Waybar)
    if (std::getenv("HYPRLAND_INSTANCE_SIGNATURE") || std::getenv("WAYLAND_DISPLAY")) {
        std::system("hyprctl dispatch fullscreen 2 > /dev/null 2>&1 &");
    }
#endif

    _camera = sf::View(_player.position, sf::Vector2f(static_cast<float>(_window.getSize().x),
                                                      static_cast<float>(_window.getSize().y)));
}

void GameApp::initSubsystems() {
    SoundManager::get().init("assets/soundeff");

    _crosshair = std::make_unique<Crosshair>("assets/sprites/crosshair.png", _window);

    // Инициализация меню и настроек
    _menuSystem.init(_combat.getFont());
    _menuSystem.setFpsMode(static_cast<FpsLimitMode>(_fpsMode));
    _menuSystem.setShowFpsCounter(true);

    _menuSystem.setOnStartGame([this]() {
        _isTrainingMode = false;
        _menuSystem.setTrainingMode(false);
        respawnEnemies();
        _window.setMouseCursorVisible(false);
    });

    _menuSystem.setOnStartTraining([this]() {
        _isTrainingMode = true;
        _menuSystem.setTrainingMode(true);
        _arena.loadRoom(1);
        _player.reset(_arena.getSpawnPosition());
        respawnEnemies();
        _combat.reset();
        _camera.setCenter(_player.position);
        _window.setMouseCursorVisible(false);
    });

    _menuSystem.setOnResetGame([this]() {
        _isTrainingMode = false;
        _menuSystem.setTrainingMode(false);
        _arena.loadRoom(1);
        _player.reset(_arena.getSpawnPosition());
        respawnEnemies();
        _combat.reset();
        _camera.setCenter(_player.position);
    });

    _menuSystem.setOnExitGame([this]() {
        _window.close();
    });

    _menuSystem.setOnFpsModeChanged([this](FpsLimitMode m) {
        applyFpsMode(static_cast<FpsMode>(m));
    });

    applyFpsMode(FpsMode::UNCAPPED);
    respawnEnemies();

    _lastCombatTime = getFileTimeSafe("assets/config/combat_balance.kv");
    _lastPlayerTime = getFileTimeSafe("assets/config/player.kv");
    _lastEnemyTime = getFileTimeSafe("assets/config/enemy.kv");

    // Запускаем фоновую музыку главного меню при старте
    SoundManager::get().playMenuMusic();
}

void GameApp::applyFpsMode(FpsMode mode) {
    _fpsMode = mode;
    switch (mode) {
        case FpsMode::UNCAPPED:
            _window.setVerticalSyncEnabled(false);
            _window.setFramerateLimit(0);
            std::cout << "[FPS] Режим: БЕЗ ОГРАНИЧЕНИЙ (Uncapped / Max FPS)" << std::endl;
            break;
        case FpsMode::FPS_200:
            _window.setVerticalSyncEnabled(false);
            _window.setFramerateLimit(200);
            std::cout << "[FPS] Режим: 200 FPS" << std::endl;
            break;
        case FpsMode::FPS_240:
            _window.setVerticalSyncEnabled(false);
            _window.setFramerateLimit(240);
            std::cout << "[FPS] Режим: 240 FPS" << std::endl;
            break;
        case FpsMode::FPS_144:
            _window.setVerticalSyncEnabled(false);
            _window.setFramerateLimit(144);
            std::cout << "[FPS] Режим: 144 FPS" << std::endl;
            break;
        case FpsMode::FPS_60:
            _window.setVerticalSyncEnabled(false);
            _window.setFramerateLimit(60);
            std::cout << "[FPS] Режим: 60 FPS" << std::endl;
            break;
        case FpsMode::VSYNC:
            _window.setFramerateLimit(0);
            _window.setVerticalSyncEnabled(true);
            std::cout << "[FPS] Режим: VSync (Вертикальная синхронизация)" << std::endl;
            break;
    }
}

void GameApp::respawnEnemies() {
    _enemies.clear();
    if (_isTrainingMode) {
        // Спавн одного бессмертного тренировочного манекена прямо перед спавном игрока
        float dummyX = _arena.getSpawnPosition().x + 220.0f;
        float dummyY = _arena.getSpawnPosition().y;
        _enemies.emplace_back(sf::Vector2f(dummyX, dummyY), EnemyType::DUMMY);
        _enemies.back().setDummyBlockMode(_dummyBlockMode);
        _enemies.back().reset();
        std::cout << "[TRAINING MODE] Тренировочный манекен (Dummy) заспавнен на позиции ["
                  << dummyX << ", " << dummyY << "]" << std::endl;
        return;
    }
    const auto& spawns = _arena.getEnemySpawns();
    if (spawns.empty()) {
        _enemies.emplace_back(_arena.getEnemySpawnPosition(), EnemyType::NORMAL);
    } else {
        for (const auto& sp : spawns) {
            EnemyType t = (sp.type == 1) ? EnemyType::TANK : 
                          (sp.type == 2) ? EnemyType::DUELIST : 
                          (sp.type == 3) ? EnemyType::DUELIST_PASSIVE : 
                          (sp.type == 4) ? EnemyType::DUMMY :
                          (sp.type == 5 || sp.type == static_cast<int>(EnemyType::BOSS_1) || sp.enemyId == "boss_1") ? EnemyType::BOSS_1 : EnemyType::NORMAL;
            _enemies.emplace_back(sp.position, t);
            if (!sp.enemyId.empty()) {
                _enemies.back().setEnemyId(sp.enemyId);
            }
        }
    }
    for (auto& e : _enemies) {
        e.setSmartAI(_isSmartAIMode);
        e.reset();
    }
}

void GameApp::checkHotReload() {
    if (_kvWatchClock.getElapsedTime().asSeconds() >= 0.5f) {
        _kvWatchClock.restart();
        auto curC = getFileTimeSafe("assets/config/combat_balance.kv");
        if (curC != std::filesystem::file_time_type{} && curC > _lastCombatTime) {
            _lastCombatTime = curC;
            CombatConfig::reload();
            std::cout << "[HOT-RELOAD] Файл assets/config/combat_balance.kv был изменен на диске -> баланс комбо и оружия обновлен!" << std::endl;
        }
        auto curP = getFileTimeSafe("assets/config/player.kv");
        if (curP != std::filesystem::file_time_type{} && curP > _lastPlayerTime) {
            _lastPlayerTime = curP;
            PlayerConfig::reload();
            std::cout << "[HOT-RELOAD] Файл assets/config/player.kv был изменен на диске -> параметры игрока обновлены!" << std::endl;
        }
        auto curE = getFileTimeSafe("assets/config/enemy.kv");
        if (curE != std::filesystem::file_time_type{} && curE > _lastEnemyTime) {
            _lastEnemyTime = curE;
            EnemyConfig::reload();
            std::cout << "[HOT-RELOAD] Файл assets/config/enemy.kv был изменен на диске -> параметры врагов обновлены!" << std::endl;
        }
    }
}

void GameApp::processEvents() {
    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f mouseScreenPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
    sf::Vector2f mouseWorld = _window.mapPixelToCoords(mousePixel, _camera);

    while (const std::optional<sf::Event> event = _window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            _window.close();
        }

        if (_menuSystem.isInMenu()) {
            _menuSystem.handleEvent(*event, mouseScreenPos);
            _window.setMouseCursorVisible(_menuSystem.isInMenu());
            continue;
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                _menuSystem.openPauseMenu();
                _window.setMouseCursorVisible(true);
                continue;
            }
            // F3 - переключение отображения FPS счетчика
            if (keyPressed->code == sf::Keyboard::Key::F3) {
                _menuSystem.setShowFpsCounter(!_menuSystem.getShowFpsCounter());
            }
            // F4 - переключение лимитера частоты кадров
            if (keyPressed->code == sf::Keyboard::Key::F4) {
                FpsMode nextMode = (_fpsMode == FpsMode::UNCAPPED) ? FpsMode::FPS_200 :
                                   (_fpsMode == FpsMode::FPS_200)   ? FpsMode::FPS_240 :
                                   (_fpsMode == FpsMode::FPS_240)   ? FpsMode::FPS_144 :
                                   (_fpsMode == FpsMode::FPS_144)   ? FpsMode::FPS_60 :
                                   (_fpsMode == FpsMode::FPS_60)    ? FpsMode::VSYNC :
                                                                      FpsMode::UNCAPPED;
                applyFpsMode(nextMode);
            }
            // Backspace - сброс позиций и состояний
            if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                _player.reset(_arena.getSpawnPosition());
                respawnEnemies();
                _combat.reset();
                std::cout << "[RESET] Арена, игрок и " << _enemies.size() << " врагов сброшены!" << std::endl;
            }
            // Tab - переключение режима противников (Манекен / Умный ИИ)
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
                _isSmartAIMode = !_isSmartAIMode;
                for (auto& e : _enemies) {
                    e.setType(_isSmartAIMode ? EnemyType::NORMAL : EnemyType::DUMMY);
                    e.setSmartAI(_isSmartAIMode);
                    e.reset();
                }
                std::cout << "[MODE] Режим всех врагов: " 
                          << (_isSmartAIMode ? "УМНЫЙ ИИ (Комбо-боец)" : "ТРЕНИРОВОЧНЫЙ МАНЕКЕН (Dummy)") 
                          << std::endl;
            }
            // Y - циклическое переключение архетипов всех врагов
            if (keyPressed->code == sf::Keyboard::Key::Y) {
                for (auto& e : _enemies) {
                    EnemyType nextT = (e.getType() == EnemyType::NORMAL) ? EnemyType::TANK :
                                      (e.getType() == EnemyType::TANK) ? EnemyType::DUELIST : 
                                      (e.getType() == EnemyType::DUELIST) ? EnemyType::DUELIST_PASSIVE : 
                                      (e.getType() == EnemyType::DUELIST_PASSIVE) ? EnemyType::DUMMY : EnemyType::NORMAL;
                    e.setType(nextT);
                    e.reset();
                }
                std::cout << "[ARCHETYPE] Враги переключены на новый тип!" << std::endl;
            }
            // F5 - горячая перезагрузка всех конфигураций
            if (keyPressed->code == sf::Keyboard::Key::F5) {
                CombatConfig::reload();
                PlayerConfig::reload();
                EnemyConfig::reload();
                SoundManager::get().reload();
                _arena.reload();
                respawnEnemies();
                std::cout << "[F5] Конфиги и комната " << _arena.getCurrentRoomIndex() << " перезагружены!" << std::endl;
            }
            // F6 - переключение режима блока манекена
            if (keyPressed->code == sf::Keyboard::Key::F6) {
                if (_isTrainingMode) {
                    _dummyBlockMode = (_dummyBlockMode == DummyBlockMode::NONE)      ? DummyBlockMode::HIGH :
                                      (_dummyBlockMode == DummyBlockMode::HIGH)      ? DummyBlockMode::LOW :
                                      (_dummyBlockMode == DummyBlockMode::LOW)       ? DummyBlockMode::AUTO :
                                      (_dummyBlockMode == DummyBlockMode::AUTO)      ? DummyBlockMode::AFTER_HIT :
                                                                                       DummyBlockMode::NONE;
                    for (auto& e : _enemies) {
                        if (e.getType() == EnemyType::DUMMY) {
                            e.setDummyBlockMode(_dummyBlockMode);
                        }
                    }
                    const char* bName = (_dummyBlockMode == DummyBlockMode::NONE)      ? "БЕЗ БЛОКА" :
                                        (_dummyBlockMode == DummyBlockMode::HIGH)      ? "ВЕРХНИЙ БЛОК (HIGH)" :
                                        (_dummyBlockMode == DummyBlockMode::LOW)       ? "НИЖНИЙ БЛОК (LOW)" :
                                        (_dummyBlockMode == DummyBlockMode::AUTO)      ? "АВТО-РЕАКЦИЯ" :
                                                                                         "КОНТРАТАКА ПОСЛЕ УДАРА (1.0с)";
                    std::cout << "[TRAINING] Режим блока манекена: " << bName << std::endl;
                }
            }
            // PageUp / PageDown - смена комнат
            if (keyPressed->code == sf::Keyboard::Key::PageUp) {
                _arena.nextRoom();
                _player.reset(_arena.getSpawnPosition());
                respawnEnemies();
                _combat.reset();
                _camera.setCenter(_player.position);
            } else if (keyPressed->code == sf::Keyboard::Key::PageDown) {
                _arena.prevRoom();
                _player.reset(_arena.getSpawnPosition());
                respawnEnemies();
                _combat.reset();
                _camera.setCenter(_player.position);
            }
            // T - атака/комбо врагов
            if (keyPressed->code == sf::Keyboard::Key::T) {
                for (auto& e : _enemies) {
                    if (e.isSmartAI) {
                        e.triggerComboSequence();
                    } else {
                        e.triggerAttack();
                    }
                }
                std::cout << "[AI COMBAT] Инициирована атака/комбо врагов!" << std::endl;
            }
        }

        _player.handleInput(event);
        _combat.handleInput(event, _player, mouseWorld);
    }
}

void GameApp::update(float dt, const sf::Vector2f& mouseWorld) {
    if (_menuSystem.isInMenu()) {
        if (_menuSystem.getAppState() == AppState::MAIN_MENU) {
            // В главном меню играет музыка меню (The_Final_Log.mp3)
            SoundManager::get().playMenuMusic();
        }
        return;
    }

    // Во время боссфайта играет Boss1.mp3, а при победе или исследовании локации — саундтрек игры
    bool bossActive = false;
    for (const auto& e : _enemies) {
        if (e.getType() == EnemyType::BOSS_1 && !e.isDead && e.hp > 0.0f) {
            bossActive = true;
            break;
        }
    }
    if (_arena.isBossFightActive()) {
        if (!bossActive) {
            // Босс побежден: убираем барьеры, снимаем фиксацию камеры, деактивируем триггер
            _arena.endBossFight();
        } else {
            SoundManager::get().playBossMusic("Boss1");
        }
    } else {
        if (bossActive) {
            SoundManager::get().playBossMusic("Boss1");
        } else {
            SoundManager::get().playGameMusic();
        }
    }

    _arena.update(dt);
    _arena.updateTriggers(dt, _player);
    _player.update(dt, _arena, mouseWorld);

    bool playerIsAttacking = _combat.isAttacking() || _combat.getCooldown() > 0.0f;
    for (auto& e : _enemies) {
        e.update(dt, _arena, _player.position, _player.isStunned(), playerIsAttacking, _player.isInvulnerable());
    }
    _combat.update(dt, _player, _enemies);

    // Проверка активации настраиваемых блоков дверей (тайл 3)
    static float doorCooldown = 0.0f;
    if (doorCooldown > 0.0f) doorCooldown -= dt;

    if (doorCooldown <= 0.0f) {
        auto doorOpt = _arena.getOverlappingDoor(_player.position.x, _player.position.y,
                                                Player::HALF_W, Player::TOP_OFFSET, Player::BOTTOM_OFFSET);
        if (doorOpt.has_value()) {
            const auto& door = doorOpt.value();
            std::string targetRoomName = !door.targetRoomName.empty() ? door.targetRoomName : (door.targetRoom > 0 ? ("room_" + std::to_string(door.targetRoom)) : "");
            bool isDiffRoom = !targetRoomName.empty() && targetRoomName != _arena.getCurrentRoomName();
            bool isSameRoom = (targetRoomName.empty() || targetRoomName == _arena.getCurrentRoomName()) &&
                              door.targetDoorX >= 0 && door.targetDoorY >= 0 &&
                              (door.targetDoorX != door.x || door.targetDoorY != door.y);
            if (isDiffRoom || isSameRoom) {
                if (isDiffRoom) {
                    _arena.loadRoomByName(targetRoomName);
                }
                if (door.targetDoorX >= 0 && door.targetDoorY >= 0) {
                    float ts = _arena.getTileSize();
                    float spawnX = (door.targetDoorX + 0.5f) * ts;
                    float spawnY = (door.targetDoorY + 1.0f) * ts - Player::BOTTOM_OFFSET;
                    if (door.targetDoorX <= 2) {
                        spawnX += 32.0f;
                    } else if (door.targetDoorX >= _arena.getWidth() - 3) {
                        spawnX -= 32.0f;
                    }
                    _player.position = { spawnX, spawnY };
                } else {
                    bool foundOpposite = false;
                    for (const auto& d : _arena.getDoors()) {
                        if (d.targetRoomName == _arena.getCurrentRoomName() || (!d.targetRoomName.empty() && d.targetRoomName == targetRoomName) || d.targetRoom == _arena.getCurrentRoomIndex()) {
                            float ts = _arena.getTileSize();
                            _player.position = { (d.x + 0.5f) * ts, (d.y + 1.0f) * ts - Player::BOTTOM_OFFSET };
                            foundOpposite = true;
                            break;
                        }
                    }
                    if (!foundOpposite) {
                        _player.position = _arena.getSpawnPosition();
                    }
                }
                _player.velocity = {0.0f, 0.0f};

                if (isDiffRoom) {
                    respawnEnemies();
                    _combat.reset();
                }
                doorCooldown = 1.0f;
                std::cout << "[DOOR TRANSITION] Переход в комнату " << (isDiffRoom ? targetRoomName : _arena.getCurrentRoomName()) << std::endl;
            }
        }
    }

    // Спавн динамических врагов из триггеров
    if (_arena.hasPendingEnemySpawns()) {
        auto pending = _arena.consumePendingEnemySpawns();
        for (const auto& sp : pending) {
            EnemyType t = (sp.type == 1) ? EnemyType::TANK : 
                          (sp.type == 2) ? EnemyType::DUELIST : 
                          (sp.type == 3) ? EnemyType::DUELIST_PASSIVE : 
                          (sp.type == 4) ? EnemyType::DUMMY :
                          (sp.type == 5 || sp.type == static_cast<int>(EnemyType::BOSS_1) || sp.enemyId == "boss_1") ? EnemyType::BOSS_1 : EnemyType::NORMAL;
            _enemies.emplace_back(sp.position, t);
            if (!sp.enemyId.empty()) {
                _enemies.back().setEnemyId(sp.enemyId);
            }
            _enemies.back().setSmartAI(_isSmartAIMode);
            _enemies.back().reset();
        }
    }

    // Динамический зум / размер камеры от триггеров
    if (_player.isDead) {
        _camera.setSize({1280.0f, 720.0f});
        _camera.setCenter(_player.position + _combat.getScreenShake());
    } else if (_arena.hasCustomCameraSize()) {
        _camera.setSize(_arena.getCameraSize());
    }

    if (!_player.isDead) {
        if (_arena.isCameraLocked()) {
            sf::Vector2f targetCam = _arena.getCameraLockPosition();
            int lockMode = _arena.getCameraLockMode();
            if (lockMode == 1) { // Фиксация на точке или блоке
                targetCam = _arena.getCameraLockPosition();
            } else if (lockMode == 2) { // Фиксация на определенном враге
                int enIdx = _arena.getCameraTargetEnemyIndex();
                const std::string& enTag = _arena.getCameraTargetEnemyTag();
                bool found = false;
                if (!enTag.empty()) {
                    for (const auto& e : _enemies) {
                        if (e.getEnemyId() == enTag && !e.isDead) {
                            targetCam = e.position;
                            found = true;
                            break;
                        }
                    }
                }
                if (!found && enIdx >= 0 && enIdx < static_cast<int>(_enemies.size()) && !_enemies[enIdx].isDead) {
                    targetCam = _enemies[enIdx].position;
                    found = true;
                }
                if (!found) targetCam = _arena.getCameraLockPosition();
            } else if (lockMode == 3 || _arena.isCameraLockOnBoss()) { // Боссфайт: средняя точка между игроком и боссом
                for (const auto& e : _enemies) {
                    if (e.getType() == EnemyType::BOSS_1 && !e.isDead) {
                        targetCam = (e.position + _player.position) * 0.5f;
                        break;
                    }
                }
            }
            _camera.setCenter(targetCam + _combat.getScreenShake());
        } else {
            _camera.setCenter(_player.position + _combat.getScreenShake());
        }
    }

    // Перезапуск при гибели (полный сброс арены, камеры и музыки!)
    static float playerDeathTimer = 2.0f;
    if (_player.isDead) {
        playerDeathTimer -= dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
            playerDeathTimer = 0.0f;
        }

        if (playerDeathTimer <= 0.0f) {
            _arena.loadRoom(1);
            _arena.resetCameraLock();
            _player.reset(_arena.getSpawnPosition());
            respawnEnemies();
            _combat.reset();
            _camera.setSize({1280.0f, 720.0f});
            _camera.setCenter(_player.position);
            SoundManager::get().playGameMusic();
            playerDeathTimer = 2.0f;
        }
    } else {
        playerDeathTimer = 2.0f;
    }
}

void GameApp::render() {
    sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
    sf::Vector2f winSize(static_cast<float>(_window.getSize().x), static_cast<float>(_window.getSize().y));
    sf::Vector2f mouseScreenPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

    // 1. Отрисовка игрового мира через камеру
    if (!_menuSystem.isInMenu() || _menuSystem.getAppState() == AppState::PAUSED) {
        _window.clear(sf::Color::Black);
        _window.setView(_camera);

        _arena.draw(_window);
        for (auto& e : _enemies) {
            e.draw(_window, _combat.getFont());
        }
        _combat.drawWorld(_window, _player);
        _player.draw(_window);

        _arena.renderLighting(_window, _camera, _player.position, _player.hasWeapon());
        _arena.drawBlackoutMask(_window);
    }

    // 2. Отрисовка UI / HUD в координатах экрана
    _window.setView(_window.getDefaultView());

    if (_menuSystem.isInMenu()) {
        _menuSystem.draw(_window, mouseScreenPos);
    } else {
        // Отрисовка грандиозного ХП-бара босса по центру экрана
        for (const auto& e : _enemies) {
            if (e.getType() == EnemyType::BOSS_1 && !e.isDead && e.hp > 0.0f) {
                _combat.drawBossHealthBar(_window, &e);
                break;
            }
        }

        _combat.drawUI(_window, &_player);

        if (_player.isDead && _combat.getFont()) {
            const sf::Font& font = *_combat.getFont();
            sf::RectangleShape deathBg({winSize.x, winSize.y});
            deathBg.setFillColor(sf::Color(10, 0, 0, 160));
            _window.draw(deathBg);

            sf::Text deathText(font, toSfString("ВЫ ПОГИБЛИ"), 54);
            deathText.setStyle(sf::Text::Bold);
            deathText.setFillColor(sf::Color(255, 60, 60));
            sf::FloatRect tb = deathText.getLocalBounds();
            deathText.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
            deathText.setPosition(sf::Vector2f(winSize.x * 0.5f, winSize.y * 0.45f));
            _window.draw(deathText);

            sf::Text deathSub(font, toSfString("[SPACE] или [ENTER] — Возродиться"), 20);
            deathSub.setFillColor(sf::Color(220, 220, 220));
            sf::FloatRect sb = deathSub.getLocalBounds();
            deathSub.setOrigin({sb.position.x + sb.size.x * 0.5f, sb.position.y + sb.size.y * 0.5f});
            deathSub.setPosition(sf::Vector2f(winSize.x * 0.5f, winSize.y * 0.54f));
            _window.draw(deathSub);
        } else if (_crosshair) {
            _crosshair->Draw(_window);
        }

        // Информационная панель режима тренировки
        if (_isTrainingMode && _combat.getFont()) {
            const sf::Font& font = *_combat.getFont();
            float barW = 640.0f;
            float barH = 30.0f;
            float barX = winSize.x * 0.5f - barW * 0.5f;
            float barY = 12.0f;

            sf::RectangleShape banner({barW, barH});
            banner.setPosition({barX, barY});
            banner.setFillColor(sf::Color(18, 18, 22, 245));
            banner.setOutlineThickness(1.5f);
            banner.setOutlineColor(sf::Color(55, 55, 62, 230));
            _window.draw(banner);

            std::string blockStr = (_dummyBlockMode == DummyBlockMode::NONE)      ? "БЕЗ БЛОКА" :
                                   (_dummyBlockMode == DummyBlockMode::HIGH)      ? "ВЕРХНИЙ БЛОК" :
                                   (_dummyBlockMode == DummyBlockMode::LOW)       ? "НИЖНИЙ БЛОК" :
                                   (_dummyBlockMode == DummyBlockMode::AUTO)      ? "АВТО-РЕАКЦИЯ" :
                                                                                   "КОНТРАТАКА (1с)";
            std::string infoStr = "РЕЖИМ ТРЕНИРОВКИ  |  Манекен: Бессмертный  |  [F6] Блок: " + blockStr + "  |  [Backspace] Сброс";
            sf::Text bannerText(font, toSfString(infoStr), 13);
            bannerText.setFillColor(sf::Color::White);
            sf::FloatRect tb = bannerText.getLocalBounds();
            bannerText.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
            bannerText.setPosition(sf::Vector2f(winSize.x * 0.5f, barY + barH * 0.5f));
            _window.draw(bannerText);
        }
    }

    // 3. Отрисовка счетчика FPS
    if (_menuSystem.getShowFpsCounter() && _combat.getFont()) {
        const sf::Font& font = *_combat.getFont();
        bool isVsyncWaiting = (_avgSwapMs > 2.0f && _avgSwapMs > _avgCpuMs * 2.0f);
        int detectedHz = static_cast<int>(std::round(_currentFps));

        char fpsBuf[128];
        if (isVsyncWaiting) {
            std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d | CPU: %.1f ms (Экран ~%dHz)", 
                          detectedHz, _avgCpuMs, detectedHz);
        } else {
            const char* modeTag = (_fpsMode == FpsMode::UNCAPPED) ? "MAX" :
                                  (_fpsMode == FpsMode::FPS_200)   ? "200" :
                                  (_fpsMode == FpsMode::FPS_240)   ? "240" :
                                  (_fpsMode == FpsMode::FPS_144)   ? "144" :
                                  (_fpsMode == FpsMode::FPS_60)    ? "60" : "VSYNC";
            std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d (CPU: %.1f ms) [%s]", 
                          detectedHz, _avgCpuMs, modeTag);
        }

        float badgeW = isVsyncWaiting ? 245.0f : 185.0f;
        float badgeH = 26.0f;
        float badgeX = winSize.x - badgeW - 20.0f;
        float badgeY = 12.0f;

        sf::RectangleShape badge({badgeW, badgeH});
        badge.setPosition({badgeX, badgeY});
        badge.setFillColor(sf::Color(15, 18, 25, 215));
        badge.setOutlineThickness(1.5f);

        sf::Color fpsColor = (_currentFps >= 55.0f) ? sf::Color(80, 240, 110) :
                             (_currentFps >= 30.0f) ? sf::Color(255, 215, 60) :
                                                     sf::Color(255, 80, 80);
        badge.setOutlineColor(sf::Color(fpsColor.r, fpsColor.g, fpsColor.b, 160));
        _window.draw(badge);

        sf::Text fpsText(font, toSfString(fpsBuf), 12);
        fpsText.setFillColor(fpsColor);
        fpsText.setPosition({badgeX + 8.0f, badgeY + 4.0f});
        _window.draw(fpsText);
    }
}

int GameApp::run() {
    sf::Clock dtClock;

    while (_window.isOpen()) {
        sf::Clock frameWorkClock;
        float dt = dtClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        checkHotReload();
        processEvents();

        sf::Vector2i mousePixel = sf::Mouse::getPosition(_window);
        sf::Vector2f mouseWorld = _window.mapPixelToCoords(mousePixel, _camera);

        update(dt, mouseWorld);

        float cpuWorkMs = frameWorkClock.getElapsedTime().asSeconds() * 1000.0f;
        _accumulatedCpuMs += cpuWorkMs;

        render();

        sf::Clock swapClock;
        _window.display();
        float swapMs = swapClock.getElapsedTime().asSeconds() * 1000.0f;
        _accumulatedSwapMs += swapMs;

        _fpsFrameCounter++;
        if (_fpsClock.getElapsedTime().asSeconds() >= 0.25f) {
            float elapsed = _fpsClock.restart().asSeconds();
            if (elapsed > 0.0f && _fpsFrameCounter > 0) {
                _currentFps = static_cast<float>(_fpsFrameCounter) / elapsed;
                _avgCpuMs = _accumulatedCpuMs / static_cast<float>(_fpsFrameCounter);
                _avgSwapMs = _accumulatedSwapMs / static_cast<float>(_fpsFrameCounter);
            }
            _fpsFrameCounter = 0;
            _accumulatedCpuMs = 0.0f;
            _accumulatedSwapMs = 0.0f;
        }
    }

    return 0;
}

