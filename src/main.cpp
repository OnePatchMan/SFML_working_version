#include "core/Common.h"
#include "world/Arena.h"
#include "world/MapEditor.h"
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
    MapEditor mapEditor;

    bool isSmartAIMode = true;
    std::vector<Enemy> enemies;

    auto respawnEnemies = [&]() {
        enemies.clear();
        const auto& spawns = arena.getEnemySpawns();
        if (spawns.empty()) {
            enemies.emplace_back(arena.getEnemySpawnPosition(), EnemyType::NORMAL);
        } else {
            for (const auto& sp : spawns) {
                EnemyType t = (sp.type == 1) ? EnemyType::TANK : 
                              (sp.type == 2) ? EnemyType::DUELIST : 
                              (sp.type == 3) ? EnemyType::DUELIST_PASSIVE : EnemyType::NORMAL;
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
        window.setMouseCursorVisible(false);
    });
    menuSystem.setOnResetGame([&]() {
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

    while (window.isOpen()) {
        sf::Clock frameWorkClock;
        float dt = dtClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

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
                    if (mapEditor.isActive()) {
                        mapEditor.toggle(window, camera, player);
                    } else {
                        menuSystem.openPauseMenu();
                        window.setMouseCursorVisible(true);
                    }
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
                    // F6 - Переключение режима ручного редактора карты
                    if (keyPressed->code == sf::Keyboard::Key::F6) {
                        mapEditor.toggle(window, camera, player);
                        if (!mapEditor.isActive()) {
                            respawnEnemies();
                        }
                        std::cout << "[EDITOR] " 
                                  << (mapEditor.isActive() ? "Режим редактора ВКЛЮЧЕН" : "Режим редактора ВЫКЛЮЧЕН") 
                                  << std::endl;
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
                            e.setSmartAI(isSmartAIMode);
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
                                              (e.getType() == EnemyType::DUELIST) ? EnemyType::DUELIST_PASSIVE : EnemyType::NORMAL;
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
                    // Быстрое переключение комнат в бою (PageUp / PageDown)
                    if (!mapEditor.isActive()) {
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

                if (mapEditor.isActive()) {
                    if (!enemies.empty()) {
                        mapEditor.handleInput(event, arena, player, enemies.front(), window, camera);
                    }
                } else {
                    player.handleInput(event);
                    combat.handleInput(event, player, mouseWorld);
                }
            }

        // Обновление состояний
        if (!menuSystem.isInMenu()) {
            if (mapEditor.isActive()) {
                if (!enemies.empty()) {
                    mapEditor.update(dt, arena, player, enemies.front(), window, camera);
                }
            } else {
                arena.update(dt);
                player.update(dt, arena, mouseWorld);
                for (auto& e : enemies) {
                    e.update(dt, arena, player.position, player.isStunned());
                }
                combat.update(dt, player, enemies);

                // Проверка активации настраиваемых блоков дверей (тайл 3)
                static float doorCooldown = 0.0f;
                if (doorCooldown > 0.0f) doorCooldown -= dt;

                if (doorCooldown <= 0.0f) {
                    auto doorTarget = arena.getOverlappingDoorTargetRoom(player.position.x, player.position.y,
                                                                         Player::HALF_W, Player::TOP_OFFSET, Player::BOTTOM_OFFSET);
                    if (doorTarget.has_value()) {
                        int targetRoom = doorTarget.value();
                        if (targetRoom != arena.getCurrentRoomIndex()) {
                            arena.loadRoom(targetRoom);
                            player.position = arena.getSpawnPosition();
                            player.velocity = {0.0f, 0.0f};

                            respawnEnemies();
                            combat.reset();
                            doorCooldown = 0.6f; // Задержка перед следующим переходом, чтобы исключить зацикливание
                            std::cout << "[DOOR TRANSITION] Игрок перешел в комнату " << targetRoom << " через блок двери!" << std::endl;
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
        }

        // 1. Отрисовка игрового мира через камеру
        if (!menuSystem.isInMenu() || menuSystem.getAppState() == AppState::PAUSED) {
            window.clear(sf::Color(25, 28, 35));
            window.setView(camera);

            arena.draw(window);
            for (auto& e : enemies) {
                e.draw(window, combat.getFont());
            }
            combat.drawWorld(window, player);
            player.draw(window);

            if (mapEditor.isActive()) {
                mapEditor.drawWorld(window, arena, mouseWorld, combat.getFont());
            }
        } else {
            // В главном меню (и окнах настроек/гайда) мир игры не рендерится, заливка 100% сплошная
            window.clear(sf::Color(13, 17, 26));
        }

        // 2. Отрисовка интерфейса
        window.setView(window.getDefaultView());
        if (mapEditor.isActive()) {
            mapEditor.drawUI(window, arena, combat.getFont(), camera);
        } else if (menuSystem.isInMenu()) {
            menuSystem.draw(window, mouseScreenPos);
            window.setMouseCursorVisible(menuSystem.isInMenu());
        } else {
            combat.drawUI(window, &player);

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
