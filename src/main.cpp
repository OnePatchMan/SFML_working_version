#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/PlayerConfig.h"
#include "entities/Enemy.h"
#include "entities/EnemyConfig.h"
#include "combat/CombatSystem.h"
#include "graphics/Crosshair.h"

int main() {
    Arena arena(ARENA_WIDTH, ARENA_HEIGHT);
    Player player(arena.getSpawnPosition());
    Enemy enemy({player.position.x + 350.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)});
    CombatSystem combat;

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "FightingComboGame", sf::Style::None);
    window.setMouseCursorVisible(false);
    window.setFramerateLimit(144);

    Crosshair crosshair("assets/sprites/crosshair.png", window);
    sf::View camera(player.position, sf::Vector2f(static_cast<float>(window.getSize().x), 
                                                 static_cast<float>(window.getSize().y)));
    sf::Clock dtClock;

    while (window.isOpen()) {
        float dt = dtClock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
        sf::Vector2f mouseWorld = window.mapPixelToCoords(mousePixel, camera);

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) {
                    window.close();
                }
                // R - сброс позиций и состояний
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    player.reset(arena.getSpawnPosition());
                    enemy.spawnPosition = {player.position.x + 350.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
                    enemy.reset();
                    combat.reset();
                    std::cout << "[RESET] Арена, игрок и манекен сброшены!" << std::endl;
                }
                // Tab - переключение режима противника (Манекен / Умный ИИ)
                if (keyPressed->code == sf::Keyboard::Key::Tab) {
                    enemy.setSmartAI(!enemy.isSmartAI);
                    std::cout << "[MODE] Режим противника: " 
                              << (enemy.isSmartAI ? "УМНЫЙ ИИ (Комбо-боец)" : "ТРЕНИРОВОЧНЫЙ МАНЕКЕН (Dummy)") 
                              << std::endl;
                }
                // F5 - горячая перезагрузка всех KV конфигураций (combat_balance.kv, player.kv, enemy.kv)
                if (keyPressed->code == sf::Keyboard::Key::F5) {
                    CombatConfig::reload();
                    PlayerConfig::reload();
                    EnemyConfig::reload();
                }
                // T - атака врага (или запуск комбо-связки умного ИИ)
                if (keyPressed->code == sf::Keyboard::Key::T) {
                    if (enemy.isSmartAI) {
                        enemy.triggerComboSequence();
                        std::cout << "[AI COMBAT] Умный ИИ инициирует комбо-связку (1f2d1 -> a1adw2 -> a1a1adw2)!" << std::endl;
                    } else {
                        enemy.triggerAttack();
                        std::cout << "[TRAIN] Манекен замахивается для удара!" << std::endl;
                    }
                }
            }

            player.handleInput(event);
            combat.handleInput(event, player, mouseWorld);
        }

        // Обновление физики и состояний сущностей
        player.update(dt, arena, mouseWorld);
        enemy.update(dt, arena, player.position);
        combat.update(dt, player, enemy);

        // Камера с динамической микро-тряской при ударах
        camera.setCenter(player.position + combat.getScreenShake());

        // 1. Отрисовка игрового мира через камеру
        window.clear(sf::Color(25, 28, 35));
        window.setView(camera);

        arena.draw(window);
        enemy.draw(window, combat.getFont());
        combat.drawWorld(window, player);
        player.draw(window);

        // 2. Отрисовка боевого интерфейса и прицела в координатах экрана
        window.setView(window.getDefaultView());
        combat.drawUI(window, &player);
        crosshair.Draw(window);

        window.display();
    }

    return 0;
}
