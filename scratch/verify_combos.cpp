#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "combat/CombatSystem.h"
#include "graphics/Crosshair.h"
#include <cassert>
#include <iostream>

void tick(float dt, Arena& arena, Player& player, Enemy& enemy, CombatSystem& combat, const sf::Vector2f& mousePos) {
    player.update(dt, arena, mousePos);
    enemy.update(dt, arena);
    combat.update(dt, player, enemy);
}

void pressKey(CombatSystem& combat, Player& player, const sf::Vector2f& mousePos, sf::Keyboard::Key key) {
    sf::Event::KeyPressed kp;
    kp.code = key;
    sf::Event event(kp);
    player.handleInput(event);
    combat.handleInput(event, player, mousePos);
}

int main() {
    Arena arena(ARENA_WIDTH, ARENA_HEIGHT);
    Player player(arena.getSpawnPosition());
    Enemy enemy({player.position.x + 50.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)});
    CombatSystem combat;

    sf::Vector2f mousePos = {player.position.x + 100.0f, player.position.y};
    tick(0.01f, arena, player, enemy, combat, mousePos);

    // -------------------------------------------------------------
    // 1. Crosshair verification
    // -------------------------------------------------------------
    std::cout << "[TEST] 1. Testing Crosshair..." << std::endl;
    sf::RenderWindow testWindow;
    Crosshair crosshair("assets/sprites/crosshair.png", testWindow);
    std::cout << "Crosshair initialized." << std::endl;

    // -------------------------------------------------------------
    // 2. Air Suspension ONLY ON HIT verification
    // -------------------------------------------------------------
    std::cout << "[TEST] 2. Testing Air Suspension: Whiff vs Landed Hit..." << std::endl;
    // 2A: WHIFF in air (no enemy nearby)
    combat.reset();
    player.reset(arena.getSpawnPosition());
    player.position.y -= 250.0f; // in mid air
    enemy.position = {player.position.x + 500.0f, player.position.y}; // far away enemy
    tick(0.01f, arena, player, enemy, combat, mousePos);

    assert(!player.isOnGround());
    assert(!player.isAirSuspended());

    // Swing in the air without hitting anything
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.016f, arena, player, enemy, combat, mousePos);

    // Player MUST NOT be suspended on a whiff!
    std::cout << "Whiff swing: isAirSuspended = " << player.isAirSuspended() 
              << ", velocity.y = " << player.velocity.y << std::endl;
    assert(!player.isAirSuspended());
    assert(player.velocity.y > 0.0f); // Gravity pulls player down normally

    // 2B: HIT in air (enemy right in front of player)
    combat.reset();
    player.reset(arena.getSpawnPosition());
    player.position.y -= 250.0f; // in mid air
    enemy.position = {player.position.x + 35.0f, player.position.y}; // in front of player
    tick(0.01f, arena, player, enemy, combat, mousePos);

    // Swing and connect with enemy
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.016f, arena, player, enemy, combat, mousePos);

    // Player MUST be suspended on landed hit!
    std::cout << "Landed hit: isAirSuspended = " << player.isAirSuspended() 
              << ", velocity.y = " << player.velocity.y << std::endl;
    assert(player.isAirSuspended());
    assert(player.velocity.y == 0.0f);
    std::cout << "-> AIR SUSPENSION ONLY ON HIT CONFIRMED!" << std::endl;

    // -------------------------------------------------------------
    // 3. Testing 12d1 (Diagonal Launcher - moderate reachable distance)
    // -------------------------------------------------------------
    std::cout << "\n[TEST] 3. Testing Combo 1-2-d1 Diagonal Launcher..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    // Give player a tick to settle on ground
    tick(0.05f, arena, player, enemy, combat, mousePos);
    assert(player.isOnGround());

    enemy.reset();
    enemy.position = {player.position.x + 40.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
    enemy.update(0.001f, arena);

    // Step 1: Light (1)
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    for (int i = 0; i < 20; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 2: Heavy (2) - pushes enemy to dash distance
    pressKey(combat, player, mousePos, sf::Keyboard::Key::K);
    for (int i = 0; i < 30; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 3: Dash towards enemy
    player.setSimulatedMoveDir(1.0f);
    pressKey(combat, player, mousePos, sf::Keyboard::Key::LShift);
    player.setSimulatedMoveDir(0.0f);
    for (int i = 0; i < 8; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 4: d1 - diagonal launcher!
    float enemyX_before_d1 = enemy.position.x;
    float enemyY_before_d1 = enemy.position.y;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "12d1 triggered: combo=" << combat.getCurrentComboName() 
              << ", enemy velocity: vx=" << enemy.velocity.x << ", vy=" << enemy.velocity.y << std::endl;

    std::cout << "Enemy velocity after 12d1: vx=" << enemy.velocity.x << ", vy=" << enemy.velocity.y << std::endl;

    // Track trajectory over time
    float maxDeltaX_12d1 = 0.0f;
    float maxDeltaY_12d1 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        tick(0.016f, arena, player, enemy, combat, mousePos);
        float dx = enemy.position.x - enemyX_before_d1;
        float dy = enemyY_before_d1 - enemy.position.y; // positive upwards
        if (dx > maxDeltaX_12d1) maxDeltaX_12d1 = dx;
        if (dy > maxDeltaY_12d1) maxDeltaY_12d1 = dy;
    }

    std::cout << "12d1 Trajectory: traveled in X = " << maxDeltaX_12d1 
              << " px, traveled in Y = " << maxDeltaY_12d1 << " px" << std::endl;
    // Traveled in X must be reachable: between 70px and 140px (around dash distance ~112px)
    assert(maxDeltaX_12d1 >= 70.0f && maxDeltaX_12d1 <= 140.0f);
    assert(maxDeltaY_12d1 >= 70.0f); // Upwards flight
    std::cout << "-> 12d1 MODERATE REACHABLE DIAGONAL CONFIRMED!" << std::endl;

    // -------------------------------------------------------------
    // 4. Testing a1a2 (Diagonal Slam Down with Ground Bounce)
    // -------------------------------------------------------------
    std::cout << "\n[TEST] 4. Testing Combo a1-a2 Diagonal Slam Down & Bounce..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    player.position.y -= 250.0f; // mid air
    player.update(0.001f, arena, mousePos);
    enemy.reset();
    enemy.position = {player.position.x + 40.0f, player.position.y};

    // Step 1: Air light (a1)
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    for (int i = 0; i < 20; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 2: Air heavy slam (a2)
    float enemyX_before_a2 = enemy.position.x;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::K);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "Enemy velocity after a1a2 slam: vx=" << enemy.velocity.x << ", vy=" << enemy.velocity.y << std::endl;

    // Run until floor impact and bounce
    float maxDeltaX_a1a2 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        tick(0.016f, arena, player, enemy, combat, mousePos);
        float dx = enemy.position.x - enemyX_before_a2;
        if (dx > maxDeltaX_a1a2) maxDeltaX_a1a2 = dx;
    }

    std::cout << "a1a2 Trajectory: traveled in X = " << maxDeltaX_a1a2 << " px" << std::endl;
    // Traveled in X must be reachable: between 70px and 140px
    assert(maxDeltaX_a1a2 >= 70.0f && maxDeltaX_a1a2 <= 140.0f);
    std::cout << "-> a1a2 MODERATE REACHABLE SLAM & BOUNCE CONFIRMED!" << std::endl;

    // -------------------------------------------------------------
    // 5. Testing 112 (Diagonal Downward Throw from Ground)
    // -------------------------------------------------------------
    std::cout << "\n[TEST] 5. Testing Combo 1-1-2 Diagonal Downward Throw..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    tick(0.05f, arena, player, enemy, combat, mousePos);
    assert(player.isOnGround());

    enemy.reset();
    enemy.position = {player.position.x + 45.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
    enemy.update(0.001f, arena);

    // Step 1: Light (1)
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    for (int i = 0; i < 20; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 2: Light (1)
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    for (int i = 0; i < 20; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 3: Heavy (2)
    float enemyX_before_112 = enemy.position.x;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::K);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "Enemy velocity after 112 throw: vx=" << enemy.velocity.x << ", vy=" << enemy.velocity.y << std::endl;

    float maxDeltaX_112 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        tick(0.016f, arena, player, enemy, combat, mousePos);
        float dx = enemy.position.x - enemyX_before_112;
        if (dx > maxDeltaX_112) maxDeltaX_112 = dx;
    }

    std::cout << "112 Trajectory: traveled in X = " << maxDeltaX_112 << " px" << std::endl;
    assert(maxDeltaX_112 >= 50.0f && maxDeltaX_112 <= 150.0f);
    std::cout << "-> 112 CONTROLLED DOWNWARD THROW CONFIRMED!" << std::endl;

    // -------------------------------------------------------------
    // 6. Testing a2ad1 (Air Push to Dash -> Diagonal Launch)
    // -------------------------------------------------------------
    std::cout << "\n[TEST] 6. Testing Combo a2-ad1 Air Push -> Dash -> Diagonal Launch..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    player.position.y -= 250.0f; // mid air
    enemy.reset();
    enemy.position = {player.position.x + 40.0f, player.position.y};

    // Step 1: Air Heavy (a2)
    float enemyX_before_a2_push = enemy.position.x;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::K);
    for (int i = 0; i < 25; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);
    float a2_push_distance = enemy.position.x - enemyX_before_a2_push;
    std::cout << "a2 push distance = " << a2_push_distance << " px (Player dash distance is 112.5 px)" << std::endl;
    std::cout << "Player Y = " << player.position.y << ", Enemy Y = " << enemy.position.y 
              << ", Enemy vy = " << enemy.velocity.y << " (on dash level!)" << std::endl;
    assert(a2_push_distance >= 95.0f && a2_push_distance <= 125.0f);
    assert(enemy.velocity.y == 0.0f); // Hovering on dash level!

    // Step 2: Air Dash
    pressKey(combat, player, mousePos, sf::Keyboard::Key::LShift);
    for (int i = 0; i < 8; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Step 3: Air Dash Light (ad1)
    float enemyX_before_ad1 = enemy.position.x;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "a2ad1 triggered: combo=" << combat.getCurrentComboName() 
              << ", enemy velocity: vx=" << enemy.velocity.x << ", vy=" << enemy.velocity.y << std::endl;

    float maxDeltaX_a2ad1 = 0.0f;
    for (int i = 0; i < 60; ++i) {
        tick(0.016f, arena, player, enemy, combat, mousePos);
        float dx = enemy.position.x - enemyX_before_ad1;
        if (dx > maxDeltaX_a2ad1) maxDeltaX_a2ad1 = dx;
    }

    std::cout << "a2ad1 Trajectory: traveled in X = " << maxDeltaX_a2ad1 << " px" << std::endl;
    assert(maxDeltaX_a2ad1 >= 70.0f && maxDeltaX_a2ad1 <= 150.0f);
    std::cout << "-> a2ad1 AIR DIAGONAL LAUNCH CONFIRMED!" << std::endl;

    std::cout << "\n=======================================================" << std::endl;
    std::cout << "[ALL TESTS PASSED] AIR SUSPENSION ON HIT & MODERATE DIAGONAL FORCES VERIFIED!" << std::endl;
    std::cout << "=======================================================" << std::endl;
    return 0;
}
