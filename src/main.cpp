#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/PlayerConfig.h"
#include "entities/Enemy.h"
#include "entities/EnemyConfig.h"
#include "combat/CombatSystem.h"
#include "graphics/Crosshair.h"
#include "audio/SoundManager.h"
#include "ui/MenuSystem.h"
#include <cstdlib>

int main() {
    // Отключение принудительного VSync драйвера Mesa (AMD/Intel) и Nvidia
#if defined(_WIN32)
    _putenv("vblank_mode=0");
    _putenv("__GL_SYNC_TO_VBLANK=0");
#else
    setenv("vblank_mode", "0", 1);
    setenv("__GL_SYNC_TO_VBLANK", "0", 1);
#endif

    SoundManager::get().init("assets/soundeff");

    Arena arena(ARENA_WIDTH, ARENA_HEIGHT);
    Player player(arena.getSpawnPosition());
    CombatSystem combat;

    bool isSmartAIMode = true;
    bool isTrainingMode = false;
    DummyBlockMode dummyBlockMode = DummyBlockMode::NONE;
    std::vector<Enemy> enemies;

    auto respawnEnemies = [&]() {
        enemies.clear();
        if (isTrainingMode) {
            // Спавн одного бессмертного тренировочного манекена прямо перед спавном игрока
            float dummyX = arena.getSpawnPosition().x + 220.0f;
            float dummyY = arena.getSpawnPosition().y;
            enemies.emplace_back(sf::Vector2f(dummyX, dummyY), EnemyType::DUMMY);
            enemies.back().setDummyBlockMode(dummyBlockMode);
            enemies.back().reset();
            std::cout << "[TRAINING MODE] Тренировочный манекен (Dummy) заспавнен на позиции ["
                      << dummyX << ", " << dummyY << "]" << std::endl;
            return;
        }
        const auto& spawns = arena.getEnemySpawns();
        if (spawns.empty()) {
            enemies.emplace_back(arena.getEnemySpawnPosition(), EnemyType::NORMAL);
        } else {
            for (const auto& sp : spawns) {
                EnemyType t = (sp.type == 1) ? EnemyType::TANK : 
                              (sp.type == 2) ? EnemyType::DUELIST : 
                              (sp.type == 3) ? EnemyType::DUELIST_PASSIVE : 
                              (sp.type == 4) ? EnemyType::DUMMY : EnemyType::NORMAL;
                enemies.emplace_back(sp.position, t);
            }
        }
        for (auto& e : enemies) {
            e.setSmartAI(isSmartAIMode);
            e.reset();
        }
    };
    respawnEnemies();

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "FightingComboGame", sf::State::Fullscreen);
    window.setMouseCursorVisible(false);

#if !defined(_WIN32)
    // Автоматический перевод в честный Fullscreen 2 в Hyprland для сокрытия панелей (Quickshell, Waybar)
    if (std::getenv("HYPRLAND_INSTANCE_SIGNATURE") || std::getenv("WAYLAND_DISPLAY")) {
        std::system("hyprctl dispatch fullscreen 2 > /dev/null 2>&1 &");
    }
#endif

    // Управление режимами ограничения частоты кадров (FPS)
    enum class FpsMode {
        UNCAPPED,
        FPS_200,
        FPS_240,
        FPS_144,
        FPS_60,
        VSYNC
    };
    FpsMode fpsMode = FpsMode::UNCAPPED;
    auto applyFpsMode = [&](FpsMode mode) {
        fpsMode = mode;
        switch (mode) {
            case FpsMode::UNCAPPED:
                window.setVerticalSyncEnabled(false);
                window.setFramerateLimit(0);
                std::cout << "[FPS] Режим: БЕЗ ОГРАНИЧЕНИЙ (Uncapped / Max FPS)" << std::endl;
                break;
            case FpsMode::FPS_200:
                window.setVerticalSyncEnabled(false);
                window.setFramerateLimit(200);
                std::cout << "[FPS] Режим: 200 FPS" << std::endl;
                break;
            case FpsMode::FPS_240:
                window.setVerticalSyncEnabled(false);
                window.setFramerateLimit(240);
                std::cout << "[FPS] Режим: 240 FPS" << std::endl;
                break;
            case FpsMode::FPS_144:
                window.setVerticalSyncEnabled(false);
                window.setFramerateLimit(144);
                std::cout << "[FPS] Режим: 144 FPS" << std::endl;
                break;
            case FpsMode::FPS_60:
                window.setVerticalSyncEnabled(false);
                window.setFramerateLimit(60);
                std::cout << "[FPS] Режим: 60 FPS" << std::endl;
                break;
            case FpsMode::VSYNC:
                window.setFramerateLimit(0);
                window.setVerticalSyncEnabled(true);
                std::cout << "[FPS] Режим: VSync (Вертикальная синхронизация)" << std::endl;
                break;
        }
    };
    applyFpsMode(FpsMode::UNCAPPED);

    Crosshair crosshair("assets/sprites/crosshair.png", window);
    sf::View camera(player.position, sf::Vector2f(static_cast<float>(window.getSize().x), 
                                                 static_cast<float>(window.getSize().y)));
    sf::Clock dtClock;

    // Инициализация системы главного меню и настроек
    MenuSystem menuSystem;
    menuSystem.init(combat.getFont());
    menuSystem.setFpsMode(static_cast<FpsLimitMode>(fpsMode));
    menuSystem.setShowFpsCounter(true);

    menuSystem.setOnStartGame([&]() {
        isTrainingMode = false;
        menuSystem.setTrainingMode(false);
        respawnEnemies();
        window.setMouseCursorVisible(false);
    });
    menuSystem.setOnStartTraining([&]() {
        isTrainingMode = true;
        menuSystem.setTrainingMode(true);
        arena.loadRoom(1);
        player.reset(arena.getSpawnPosition());
        respawnEnemies();
        combat.reset();
        camera.setCenter(player.position);
        window.setMouseCursorVisible(false);
    });
    menuSystem.setOnResetGame([&]() {
        isTrainingMode = false;
        menuSystem.setTrainingMode(false);
        arena.loadRoom(1);
        player.reset(arena.getSpawnPosition());
        respawnEnemies();
        combat.reset();
        camera.setCenter(player.position);
    });
    menuSystem.setOnExitGame([&]() {
        window.close();
    });
    menuSystem.setOnFpsModeChanged([&](FpsLimitMode m) {
        applyFpsMode(static_cast<FpsMode>(m));
    });

    // На старте игра открывается в Главном Меню со свободным курсором мыши
    window.setMouseCursorVisible(true);

    // FPS и тайминги производительности (активен по умолчанию, переключение по F3)
    sf::Clock fpsClock;
    int fpsFrameCounter = 0;
    float currentFps = 144.0f;
    float frameTimeMs = 6.9f;
    float accumulatedCpuMs = 0.0f;
    float accumulatedSwapMs = 0.0f;
    float avgCpuMs = 0.4f;
    float avgSwapMs = 6.5f;

    // Мониторинг изменений файлов KV на диске для автоматической горячей перезагрузки (Hot-Reload)
    sf::Clock kvWatchClock;
    auto getFileTimeSafe = [](const std::string& p) -> std::filesystem::file_time_type {
        try {
            std::string resolved = resolveAssetPath(p);
            if (std::filesystem::exists(resolved)) {
                return std::filesystem::last_write_time(resolved);
            }
        } catch (...) {}
        return {};
    };
    auto lastCombatTime = getFileTimeSafe("assets/config/combat_balance.kv");
    auto lastPlayerTime = getFileTimeSafe("assets/config/player.kv");
    auto lastEnemyTime = getFileTimeSafe("assets/config/enemy.kv");

    while (window.isOpen()) {
        sf::Clock frameWorkClock;
        float dt = dtClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        // Автоматическая горячая перезагрузка конфигураций при сохранении KV-файлов (без необходимости F5)
        if (kvWatchClock.getElapsedTime().asSeconds() >= 0.5f) {
            kvWatchClock.restart();
            auto curC = getFileTimeSafe("assets/config/combat_balance.kv");
            if (curC != std::filesystem::file_time_type{} && curC > lastCombatTime) {
                lastCombatTime = curC;
                CombatConfig::reload();
                std::cout << "[HOT-RELOAD] Файл assets/config/combat_balance.kv был изменен на диске -> баланс комбо и оружия обновлен!" << std::endl;
            }
            auto curP = getFileTimeSafe("assets/config/player.kv");
            if (curP != std::filesystem::file_time_type{} && curP > lastPlayerTime) {
                lastPlayerTime = curP;
                PlayerConfig::reload();
                std::cout << "[HOT-RELOAD] Файл assets/config/player.kv был изменен на диске -> параметры игрока обновлены!" << std::endl;
            }
            auto curE = getFileTimeSafe("assets/config/enemy.kv");
            if (curE != std::filesystem::file_time_type{} && curE > lastEnemyTime) {
                lastEnemyTime = curE;
                EnemyConfig::reload();
                std::cout << "[HOT-RELOAD] Файл assets/config/enemy.kv был изменен на диске -> параметры врагов обновлены!" << std::endl;
            }
        }

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, camera);
        sf::Vector2f mouseScreenPos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
        sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            if (menuSystem.isInMenu()) {
                menuSystem.handleEvent(*event, mouseScreenPos);
                window.setMouseCursorVisible(menuSystem.isInMenu());
                continue;
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    menuSystem.openPauseMenu();
                    window.setMouseCursorVisible(true);
                    continue;
                }
                // F3 - переключение отображения FPS счетчика
                if (keyPressed->code == sf::Keyboard::Key::F3) {
                    menuSystem.setShowFpsCounter(!menuSystem.getShowFpsCounter());
                }
                    // F4 - переключение лимитера частоты кадров (Uncapped -> 200 -> 240 -> 144 -> 60 -> VSync)
                    if (keyPressed->code == sf::Keyboard::Key::F4) {
                        FpsMode nextMode = (fpsMode == FpsMode::UNCAPPED) ? FpsMode::FPS_200 :
                                           (fpsMode == FpsMode::FPS_200)   ? FpsMode::FPS_240 :
                                           (fpsMode == FpsMode::FPS_240)   ? FpsMode::FPS_144 :
                                           (fpsMode == FpsMode::FPS_144)   ? FpsMode::FPS_60 :
                                           (fpsMode == FpsMode::FPS_60)    ? FpsMode::VSYNC :
                                                                             FpsMode::UNCAPPED;
                        applyFpsMode(nextMode);
                    }
                    // Backspace - сброс позиций и состояний (R используется для броска/подбора булавы)
                    if (keyPressed->code == sf::Keyboard::Key::Backspace) {
                        player.reset(arena.getSpawnPosition());
                        respawnEnemies();
                        combat.reset();
                        std::cout << "[RESET] Арена, игрок и " << enemies.size() << " врагов сброшены!" << std::endl;
                    }
                    // Tab - переключение режима противников (Манекен / Умный ИИ)
                    if (keyPressed->code == sf::Keyboard::Key::Tab) {
                        isSmartAIMode = !isSmartAIMode;
                        for (auto& e : enemies) {
                            e.setType(isSmartAIMode ? EnemyType::NORMAL : EnemyType::DUMMY);
                            e.setSmartAI(isSmartAIMode);
                            e.reset();
                        }
                        std::cout << "[MODE] Режим всех врагов: " 
                                  << (isSmartAIMode ? "УМНЫЙ ИИ (Комбо-боец)" : "ТРЕНИРОВОЧНЫЙ МАНЕКЕН (Dummy)") 
                                  << std::endl;
                    }
                    // Y - циклическое переключение архетипов всех врагов (Normal -> Tank -> Duelist -> Duelist Passive)
                    if (keyPressed->code == sf::Keyboard::Key::Y) {
                        for (auto& e : enemies) {
                            EnemyType nextT = (e.getType() == EnemyType::NORMAL) ? EnemyType::TANK :
                                              (e.getType() == EnemyType::TANK) ? EnemyType::DUELIST : 
                                              (e.getType() == EnemyType::DUELIST) ? EnemyType::DUELIST_PASSIVE : 
                                              (e.getType() == EnemyType::DUELIST_PASSIVE) ? EnemyType::DUMMY : EnemyType::NORMAL;
                            e.setType(nextT);
                            e.reset();
                        }
                        std::cout << "[ARCHETYPE] Враги переключены на новый тип!" << std::endl;
                    }
                    // F5 - горячая перезагрузка всех KV конфигураций, звуков и текущей комнаты
                    if (keyPressed->code == sf::Keyboard::Key::F5) {
                        CombatConfig::reload();
                        PlayerConfig::reload();
                        EnemyConfig::reload();
                        SoundManager::get().reload();
                        arena.reload();
                        respawnEnemies();
                        std::cout << "[F5] Конфиги и комната " << arena.getCurrentRoomIndex() << " перезагружены, "
                                  << enemies.size() << " врагов заспавнены!" << std::endl;
                    }
                    // F6 - переключение режима блока манекена в тренировочном режиме
                    if (keyPressed->code == sf::Keyboard::Key::F6) {
                        if (isTrainingMode) {
                            dummyBlockMode = (dummyBlockMode == DummyBlockMode::NONE)      ? DummyBlockMode::HIGH :
                                             (dummyBlockMode == DummyBlockMode::HIGH)      ? DummyBlockMode::LOW :
                                             (dummyBlockMode == DummyBlockMode::LOW)       ? DummyBlockMode::AUTO :
                                             (dummyBlockMode == DummyBlockMode::AUTO)      ? DummyBlockMode::AFTER_HIT :
                                                                                             DummyBlockMode::NONE;
                            for (auto& e : enemies) {
                                if (e.getType() == EnemyType::DUMMY) {
                                    e.setDummyBlockMode(dummyBlockMode);
                                }
                            }
                            const char* bName = (dummyBlockMode == DummyBlockMode::NONE)      ? "БЕЗ БЛОКА" :
                                                (dummyBlockMode == DummyBlockMode::HIGH)      ? "ВЕРХНИЙ БЛОК (HIGH)" :
                                                (dummyBlockMode == DummyBlockMode::LOW)       ? "НИЖНИЙ БЛОК (LOW)" :
                                                (dummyBlockMode == DummyBlockMode::AUTO)      ? "АВТО-РЕАКЦИЯ" :
                                                                                                "БЛОК ПОСЛЕ УДАРА / В ОКНО (1.0с)";
                            std::cout << "[TRAINING] Режим блока манекена: " << bName << std::endl;
                        }
                    }
                    // Быстрое переключение комнат в бою (PageUp / PageDown)
                    if (keyPressed->code == sf::Keyboard::Key::PageUp) {
                        arena.nextRoom();
                        player.reset(arena.getSpawnPosition());
                        respawnEnemies();
                        combat.reset();
                        camera.setCenter(player.position);
                    } else if (keyPressed->code == sf::Keyboard::Key::PageDown) {
                        arena.prevRoom();
                        player.reset(arena.getSpawnPosition());
                        respawnEnemies();
                        combat.reset();
                        camera.setCenter(player.position);
                    }
                    // T - атака врагов (или запуск комбо-связки умного ИИ)
                    if (keyPressed->code == sf::Keyboard::Key::T) {
                        for (auto& e : enemies) {
                            if (e.isSmartAI) {
                                e.triggerComboSequence();
                            } else {
                                e.triggerAttack();
                            }
                        }
                        std::cout << "[AI COMBAT] Инициирована атака/комбо врагов!" << std::endl;
                    }
                }

                player.handleInput(event);
                combat.handleInput(event, player, mouseWorld);
            }

        // Обновление состояний
        if (!menuSystem.isInMenu()) {
            arena.update(dt);
            arena.updateTriggers(dt, player);
            player.update(dt, arena, mouseWorld);
            bool playerIsAttacking = combat.isAttacking() || combat.getCooldown() > 0.0f;
            for (auto& e : enemies) {
                e.update(dt, arena, player.position, player.isStunned(), playerIsAttacking);
            }
            combat.update(dt, player, enemies);

            // Проверка активации настраиваемых блоков дверей (тайл 3)
            static float doorCooldown = 0.0f;
            if (doorCooldown > 0.0f) doorCooldown -= dt;

            if (doorCooldown <= 0.0f) {
                auto doorOpt = arena.getOverlappingDoor(player.position.x, player.position.y,
                                                        Player::HALF_W, Player::TOP_OFFSET, Player::BOTTOM_OFFSET);
                if (doorOpt.has_value()) {
                    const auto& door = doorOpt.value();
                    std::string targetRoomName = !door.targetRoomName.empty() ? door.targetRoomName : (door.targetRoom > 0 ? ("room_" + std::to_string(door.targetRoom)) : "");
                    bool isDiffRoom = !targetRoomName.empty() && targetRoomName != arena.getCurrentRoomName();
                    bool isSameRoom = (targetRoomName.empty() || targetRoomName == arena.getCurrentRoomName()) &&
                                      door.targetDoorX >= 0 && door.targetDoorY >= 0 &&
                                      (door.targetDoorX != door.x || door.targetDoorY != door.y);
                    if (isDiffRoom || isSameRoom) {
                        if (isDiffRoom) {
                            arena.loadRoomByName(targetRoomName);
                        }
                        if (door.targetDoorX >= 0 && door.targetDoorY >= 0) {
                            // Перемещение ровно к связанной двери в целевой комнате
                            float ts = arena.getTileSize();
                            float spawnX = (door.targetDoorX + 0.5f) * ts;
                            float spawnY = (door.targetDoorY + 1.0f) * ts - Player::BOTTOM_OFFSET;
                            if (door.targetDoorX <= 2) {
                                spawnX += 32.0f;
                            } else if (door.targetDoorX >= arena.getWidth() - 3) {
                                spawnX -= 32.0f;
                            }
                            player.position = { spawnX, spawnY };
                        } else {
                            // Если точные координаты не заданы, ищем обратную дверь или спавн
                            bool foundOpposite = false;
                            for (const auto& d : arena.getDoors()) {
                                if (d.targetRoomName == arena.getCurrentRoomName() || (!d.targetRoomName.empty() && d.targetRoomName == targetRoomName) || d.targetRoom == arena.getCurrentRoomIndex()) {
                                    float ts = arena.getTileSize();
                                    player.position = { (d.x + 0.5f) * ts, (d.y + 1.0f) * ts - Player::BOTTOM_OFFSET };
                                    foundOpposite = true;
                                    break;
                                }
                            }
                            if (!foundOpposite) {
                                player.position = arena.getSpawnPosition();
                            }
                        }
                        player.velocity = {0.0f, 0.0f};

                        if (isDiffRoom) {
                            respawnEnemies();
                            combat.reset();
                        }
                        doorCooldown = 1.0f; // Задержка перед следующим переходом, чтобы исключить зацикливание
                        std::cout << "[DOOR TRANSITION] Игрок перешел в комнату " << (isDiffRoom ? targetRoomName : arena.getCurrentRoomName()) << " через связанную дверь!" << std::endl;
                    }
                }
            }

            // Камера с динамической микро-тряской при ударах
            camera.setCenter(player.position + combat.getScreenShake());

            // Логика гибели персонажа: перезапуск всей игры с первой комнаты
            static float playerDeathTimer = 2.0f;
            if (player.isDead) {
                playerDeathTimer -= dt;
                // Клавиша Space или Enter позволяет мгновенно перезапуститься без ожидания
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Enter)) {
                    playerDeathTimer = 0.0f;
                }

                if (playerDeathTimer <= 0.0f) {
                    // Перезапуск всей игры с самого начала (Комната 1, полное HP, респавн врагов)
                    arena.loadRoom(1);
                    player.reset(arena.getSpawnPosition());
                    respawnEnemies();
                    combat.reset();
                    camera.setCenter(player.position);
                    playerDeathTimer = 2.0f;
                    std::cout << "[GAME OVER] Персонаж погиб! Перезапуск с самого начала (Комната 1)..." << std::endl;
                }
            } else {
                playerDeathTimer = 2.0f;
            }
        }

        // 1. Отрисовка игрового мира через камеру
        if (!menuSystem.isInMenu() || menuSystem.getAppState() == AppState::PAUSED) {
            window.clear(sf::Color::Black);
            window.setView(camera);

            arena.draw(window);
            for (auto& e : enemies) {
                e.draw(window, combat.getFont());
            }
            combat.drawWorld(window, player);
            player.draw(window);

            // Динамическое лабораторное освещение (флуоресцентные лампы, полумрак, персональная аура)
            arena.renderLighting(window, camera, player.position, player.hasWeapon());

            // Полное скрытие всех областей за пределами локации (100% черный цвет)
            arena.drawBlackoutMask(window);
        } else {
            // В главном меню (и окнах настроек/гайда) мир игры не рендерится, глубокая обсидианово-черная заливка
            window.clear(sf::Color(8, 9, 12));
        }

        // 2. Отрисовка интерфейса
        window.setView(window.getDefaultView());
        if (menuSystem.isInMenu()) {
            menuSystem.draw(window, mouseScreenPos);
            window.setMouseCursorVisible(menuSystem.isInMenu());
        } else {
            combat.drawUI(window, &player);
            arena.drawNotificationUI(window, combat.getFont());

            // Экран гибели персонажа (Game Over)
            if (player.isDead) {
                sf::RectangleShape deathOverlay(winSize);
                deathOverlay.setFillColor(sf::Color(15, 3, 3, 195));
                window.draw(deathOverlay);

                if (combat.getFont()) {
                    sf::Text deathTitle(*combat.getFont(), toSfString("ВЫ ПОГИБЛИ"), 56);
                    deathTitle.setFillColor(sf::Color(235, 45, 45));
                    deathTitle.setOutlineThickness(2.5f);
                    deathTitle.setOutlineColor(sf::Color(10, 0, 0, 240));
                    sf::FloatRect titleBounds = deathTitle.getLocalBounds();
                    deathTitle.setOrigin({titleBounds.position.x + titleBounds.size.x * 0.5f, 
                                          titleBounds.position.y + titleBounds.size.y * 0.5f});
                    deathTitle.setPosition(sf::Vector2f(winSize.x * 0.5f, winSize.y * 0.44f));
                    window.draw(deathTitle);

                    sf::Text deathSub(*combat.getFont(), toSfString("Возрождение в комнате 1... [Пробел / Enter для ускорения]"), 18);
                    deathSub.setFillColor(sf::Color(225, 210, 210, 220));
                    sf::FloatRect subBounds = deathSub.getLocalBounds();
                    deathSub.setOrigin({subBounds.position.x + subBounds.size.x * 0.5f, 
                                        subBounds.position.y + subBounds.size.y * 0.5f});
                    deathSub.setPosition(sf::Vector2f(winSize.x * 0.5f, winSize.y * 0.54f));
                    window.draw(deathSub);
                }
            } else {
                crosshair.Draw(window);
            }

            // Информационная панель режима тренировки (вверху по центру экрана, темный обсидиан)
            if (isTrainingMode && combat.getFont()) {
                const sf::Font& font = *combat.getFont();
                float barW = 640.0f;
                float barH = 30.0f;
                float barX = winSize.x * 0.5f - barW * 0.5f;
                float barY = 12.0f;

                sf::RectangleShape banner({barW, barH});
                banner.setPosition({barX, barY});
                banner.setFillColor(sf::Color(11, 13, 17, 245));
                banner.setOutlineThickness(1.5f);
                banner.setOutlineColor(sf::Color(65, 72, 85, 230));
                window.draw(banner);

                std::string blockStr = (dummyBlockMode == DummyBlockMode::NONE)      ? "БЕЗ БЛОКА" :
                                       (dummyBlockMode == DummyBlockMode::HIGH)      ? "ВЕРХНИЙ БЛОК" :
                                       (dummyBlockMode == DummyBlockMode::LOW)       ? "НИЖНИЙ БЛОК" :
                                       (dummyBlockMode == DummyBlockMode::AUTO)      ? "АВТО-РЕАКЦИЯ" :
                                                                                       "ПОСЛЕ УДАРА (1с)";
                std::string infoStr = "РЕЖИМ ТРЕНИРОВКИ  |  Манекен: Бессмертный  |  [F6] Блок: " + blockStr + "  |  [Backspace] Сброс";
                sf::Text bannerText(font, toSfString(infoStr), 13);
                bannerText.setFillColor(sf::Color(215, 255, 230));
                sf::FloatRect tb = bannerText.getLocalBounds();
                bannerText.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
                bannerText.setPosition(sf::Vector2f(winSize.x * 0.5f, barY + barH * 0.5f));
                window.draw(bannerText);
            }
        }

        // 3. Отрисовка счетчика FPS (в правом верхнем углу)
        if (menuSystem.getShowFpsCounter() && combat.getFont()) {
            const sf::Font& font = *combat.getFont();
            bool isVsyncWaiting = (avgSwapMs > 2.0f && avgSwapMs > avgCpuMs * 2.0f);
            int detectedHz = static_cast<int>(std::round(currentFps));

            char fpsBuf[128];
            if (isVsyncWaiting) {
                std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d | CPU: %.1f ms (Экран ~%dHz)", 
                              detectedHz, avgCpuMs, detectedHz);
            } else {
                const char* modeTag = (fpsMode == FpsMode::UNCAPPED) ? "MAX" :
                                      (fpsMode == FpsMode::FPS_200)   ? "200" :
                                      (fpsMode == FpsMode::FPS_240)   ? "240" :
                                      (fpsMode == FpsMode::FPS_144)   ? "144" :
                                      (fpsMode == FpsMode::FPS_60)    ? "60" : "VSYNC";
                std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d (CPU: %.1f ms) [%s]", 
                              detectedHz, avgCpuMs, modeTag);
            }

            float badgeW = isVsyncWaiting ? 245.0f : 185.0f;
            float badgeH = 26.0f;
            float badgeX = winSize.x - badgeW - 20.0f;
            float badgeY = 12.0f;

            sf::RectangleShape badge({badgeW, badgeH});
            badge.setPosition({badgeX, badgeY});
            badge.setFillColor(sf::Color(15, 18, 25, 215));
            badge.setOutlineThickness(1.5f);

            sf::Color fpsColor = (currentFps >= 55.0f) ? sf::Color(80, 240, 110) :
                                 (currentFps >= 30.0f) ? sf::Color(255, 215, 60) :
                                                         sf::Color(255, 80, 80);
            badge.setOutlineColor(sf::Color(fpsColor.r, fpsColor.g, fpsColor.b, 160));
            window.draw(badge);

            sf::Text fpsText(font, toSfString(fpsBuf), 12);
            fpsText.setFillColor(fpsColor);
            fpsText.setPosition({badgeX + 8.0f, badgeY + 4.0f});
            window.draw(fpsText);
        }

        float cpuWorkMs = frameWorkClock.getElapsedTime().asSeconds() * 1000.0f;
        accumulatedCpuMs += cpuWorkMs;

        sf::Clock swapClock;
        window.display();
        float swapMs = swapClock.getElapsedTime().asSeconds() * 1000.0f;
        accumulatedSwapMs += swapMs;

        fpsFrameCounter++;
        if (fpsClock.getElapsedTime().asSeconds() >= 0.25f) {
            float elapsed = fpsClock.restart().asSeconds();
            if (elapsed > 0.0f && fpsFrameCounter > 0) {
                currentFps = static_cast<float>(fpsFrameCounter) / elapsed;
                frameTimeMs = (elapsed / static_cast<float>(fpsFrameCounter)) * 1000.0f;
                avgCpuMs = accumulatedCpuMs / static_cast<float>(fpsFrameCounter);
                avgSwapMs = accumulatedSwapMs / static_cast<float>(fpsFrameCounter);
            }
            fpsFrameCounter = 0;
            accumulatedCpuMs = 0.0f;
            accumulatedSwapMs = 0.0f;
        }
    }

    return 0;
}
