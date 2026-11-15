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

int main() {
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
                              (sp.type == 2) ? EnemyType::DUELIST : EnemyType::NORMAL;
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
    sf::RenderWindow window(desktop, "FightingComboGame", sf::Style::None);
    window.setMouseCursorVisible(false);

    // Управление режимами ограничения частоты кадров (FPS)
    enum class FpsMode {
        UNCAPPED,
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
    bool isPaused = false;

    // FPS счетчик (активен по умолчанию, переключение по F3)
    sf::Clock fpsClock;
    int fpsFrameCounter = 0;
    float currentFps = 144.0f;
    float frameTimeMs = 6.9f;
    bool showFpsCounter = true;

    while (window.isOpen()) {
        float dt = dtClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        fpsFrameCounter++;
        if (fpsClock.getElapsedTime().asSeconds() >= 0.25f) {
            float elapsed = fpsClock.restart().asSeconds();
            if (elapsed > 0.0f && fpsFrameCounter > 0) {
                currentFps = static_cast<float>(fpsFrameCounter) / elapsed;
                frameTimeMs = (elapsed / static_cast<float>(fpsFrameCounter)) * 1000.0f;
            }
            fpsFrameCounter = 0;
        }

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, camera);
        sf::Vector2f winSize(static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
        float cx = winSize.x * 0.5f;
        float cy = winSize.y * 0.5f;

        sf::FloatRect resumeRect({cx - 160.0f, cy - 20.0f}, {320.0f, 50.0f});
        sf::FloatRect exitRect({cx - 160.0f, cy + 45.0f}, {320.0f, 50.0f});

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* mbPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mbPressed->button == sf::Mouse::Button::Left && isPaused) {
                    sf::Vector2f mousePos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));
                    if (resumeRect.contains(mousePos)) {
                        isPaused = false;
                        window.setMouseCursorVisible(false);
                    } else if (exitRect.contains(mousePos)) {
                        window.close();
                    }
                }
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    if (mapEditor.isActive()) {
                        mapEditor.toggle(window, camera, player);
                    } else {
                        isPaused = !isPaused;
                        window.setMouseCursorVisible(isPaused);
                    }
                }
                if (isPaused) {
                    if (keyPressed->code == sf::Keyboard::Key::Enter || keyPressed->code == sf::Keyboard::Key::Space) {
                        isPaused = false;
                        window.setMouseCursorVisible(false);
                    } else if (keyPressed->code == sf::Keyboard::Key::Q) {
                        window.close();
                    }
                } else {
                    // F3 - переключение отображения FPS счетчика
                    if (keyPressed->code == sf::Keyboard::Key::F3) {
                        showFpsCounter = !showFpsCounter;
                    }
                    // F4 - переключение лимитера частоты кадров (Uncapped -> 240 -> 144 -> 60 -> VSync)
                    if (keyPressed->code == sf::Keyboard::Key::F4) {
                        FpsMode nextMode = (fpsMode == FpsMode::UNCAPPED) ? FpsMode::FPS_240 :
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
                    // Y - циклическое переключение архетипов всех врагов (Normal -> Tank -> Duelist)
                    if (keyPressed->code == sf::Keyboard::Key::Y) {
                        for (auto& e : enemies) {
                            EnemyType nextT = (e.getType() == EnemyType::NORMAL) ? EnemyType::TANK :
                                              (e.getType() == EnemyType::TANK) ? EnemyType::DUELIST : EnemyType::NORMAL;
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
            }

            if (!isPaused) {
                if (mapEditor.isActive()) {
                    if (!enemies.empty()) {
                        mapEditor.handleInput(event, arena, player, enemies.front(), window, camera);
                    }
                } else {
                    player.handleInput(event);
                    combat.handleInput(event, player, mouseWorld);
                }
            }
        }

        // Обновление состояний
        if (!isPaused) {
            if (mapEditor.isActive()) {
                if (!enemies.empty()) {
                    mapEditor.update(dt, arena, player, enemies.front(), window, camera);
                }
            } else {
                arena.update(dt);
                player.update(dt, arena, mouseWorld);
                for (auto& e : enemies) {
                    e.update(dt, arena, player.position);
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

        // 2. Отрисовка интерфейса
        window.setView(window.getDefaultView());
        if (mapEditor.isActive()) {
            mapEditor.drawUI(window, arena, combat.getFont(), camera);
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
                    deathTitle.setPosition({winSize.x * 0.5f, winSize.y * 0.44f});
                    window.draw(deathTitle);

                    sf::Text deathSub(*combat.getFont(), toSfString("Возрождение в комнате 1... [Пробел / Enter для ускорения]"), 18);
                    deathSub.setFillColor(sf::Color(225, 210, 210, 220));
                    sf::FloatRect subBounds = deathSub.getLocalBounds();
                    deathSub.setOrigin({subBounds.position.x + subBounds.size.x * 0.5f, 
                                        subBounds.position.y + subBounds.size.y * 0.5f});
                    deathSub.setPosition({winSize.x * 0.5f, winSize.y * 0.54f});
                    window.draw(deathSub);
                }
            } else if (isPaused) {
                // Окно и кнопки Паузы (Pause Menu)
                sf::RectangleShape pauseOverlay(winSize);
                pauseOverlay.setFillColor(sf::Color(10, 14, 22, 215));
                window.draw(pauseOverlay);

                float panelW = 440.0f;
                float panelH = 290.0f;
                sf::RectangleShape panel({panelW, panelH});
                panel.setOrigin({panelW * 0.5f, panelH * 0.5f});
                panel.setPosition({cx, cy});
                panel.setFillColor(sf::Color(20, 25, 36, 248));
                panel.setOutlineThickness(3.0f);
                panel.setOutlineColor(sf::Color(75, 120, 190, 235));
                window.draw(panel);

                if (combat.getFont()) {
                    const sf::Font& font = *combat.getFont();

                    // Заголовок
                    sf::Text title(font, toSfString("ПАУЗА"), 42);
                    title.setFillColor(sf::Color(240, 245, 255));
                    title.setStyle(sf::Text::Bold);
                    title.setOutlineThickness(2.0f);
                    title.setOutlineColor(sf::Color(10, 15, 30, 250));
                    sf::FloatRect tb = title.getLocalBounds();
                    title.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
                    title.setPosition({cx, cy - 88.0f});
                    window.draw(title);

                    sf::Vector2f mousePos(static_cast<float>(mousePixel.x), static_cast<float>(mousePixel.y));

                    // Кнопка ПРОДОЛЖИТЬ
                    bool hoverResume = resumeRect.contains(mousePos);
                    sf::RectangleShape btnResume(resumeRect.size);
                    btnResume.setPosition(resumeRect.position);
                    btnResume.setFillColor(hoverResume ? sf::Color(45, 95, 175) : sf::Color(30, 48, 75));
                    btnResume.setOutlineThickness(hoverResume ? 2.5f : 1.5f);
                    btnResume.setOutlineColor(hoverResume ? sf::Color(130, 190, 255) : sf::Color(65, 95, 135));
                    window.draw(btnResume);

                    sf::Text txtResume(font, toSfString("ПРОДОЛЖИТЬ [ESC]"), 20);
                    txtResume.setFillColor(hoverResume ? sf::Color(255, 255, 255) : sf::Color(220, 230, 245));
                    sf::FloatRect rb = txtResume.getLocalBounds();
                    txtResume.setOrigin({rb.position.x + rb.size.x * 0.5f, rb.position.y + rb.size.y * 0.5f});
                    txtResume.setPosition({resumeRect.position.x + resumeRect.size.x * 0.5f, 
                                           resumeRect.position.y + resumeRect.size.y * 0.5f});
                    window.draw(txtResume);

                    // Кнопка ВЫХОД
                    bool hoverExit = exitRect.contains(mousePos);
                    sf::RectangleShape btnExit(exitRect.size);
                    btnExit.setPosition(exitRect.position);
                    btnExit.setFillColor(hoverExit ? sf::Color(165, 45, 55) : sf::Color(65, 28, 35));
                    btnExit.setOutlineThickness(hoverExit ? 2.5f : 1.5f);
                    btnExit.setOutlineColor(hoverExit ? sf::Color(255, 110, 120) : sf::Color(125, 55, 65));
                    window.draw(btnExit);

                    sf::Text txtExit(font, toSfString("ВЫЙТИ ИЗ ИГРЫ [Q]"), 20);
                    txtExit.setFillColor(hoverExit ? sf::Color(255, 255, 255) : sf::Color(240, 210, 215));
                    sf::FloatRect eb = txtExit.getLocalBounds();
                    txtExit.setOrigin({eb.position.x + eb.size.x * 0.5f, eb.position.y + eb.size.y * 0.5f});
                    txtExit.setPosition({exitRect.position.x + exitRect.size.x * 0.5f, 
                                         exitRect.position.y + exitRect.size.y * 0.5f});
                    window.draw(txtExit);
                }
            } else {
                crosshair.Draw(window);
            }
        }

        // 3. Отрисовка счетчика FPS (в правом верхнем углу)
        if (showFpsCounter && combat.getFont()) {
            const sf::Font& font = *combat.getFont();
            const char* modeTag = (fpsMode == FpsMode::UNCAPPED) ? "MAX" :
                                  (fpsMode == FpsMode::FPS_240)   ? "240" :
                                  (fpsMode == FpsMode::FPS_144)   ? "144" :
                                  (fpsMode == FpsMode::FPS_60)    ? "60" : "VSYNC";

            char fpsBuf[64];
            std::snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %d (%.1f ms) [%s]", 
                          static_cast<int>(std::round(currentFps)), frameTimeMs, modeTag);

            float badgeW = 180.0f;
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

        window.display();
    }

    return 0;
}
