#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "combat/CombatSystem.h"
#include <cassert>
#include <iostream>

void tick(float dt, Arena& arena, Player& player, Enemy& enemy, CombatSystem& combat, const sf::Vector2f& mousePos) {
    player.update(dt, arena, mousePos);
    enemy.update(dt, arena, player.position);
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
    Enemy enemy({player.position.x + 350.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)});
    CombatSystem combat;

    sf::Vector2f mousePos = {player.position.x + 100.0f, player.position.y};
    tick(0.05f, arena, player, enemy, combat, mousePos);

    // -------------------------------------------------------------
    // 1. VERIFY FACING: NO MOUSE FACING, REMEMBERS LAST FACING
    // -------------------------------------------------------------
    std::cout << "[TEST 1] Testing Character Facing (No mouse turning, remembers direction)..." << std::endl;
    // Initial: facing right
    assert(player.isFacingRight());

    // Mouse is placed to the LEFT of player
    sf::Vector2f mouseLeft = {player.position.x - 300.0f, player.position.y};
    tick(0.05f, arena, player, enemy, combat, mouseLeft);

    // Character MUST NOT turn to mouse! Still facing right!
    std::cout << "Player facing after mouse to left: isFacingRight = " << player.isFacingRight() << std::endl;
    assert(player.isFacingRight());
    std::cout << "-> MOUSE FACING REMOVED SUCCESSFULLY!" << std::endl;

    // -------------------------------------------------------------
    // 2. VERIFY SPOT DODGE (ON PLACE WITHOUT MOVEMENT)
    // -------------------------------------------------------------
    std::cout << "\n[TEST 2] Testing Spot Dodge on Shift without directional keys..." << std::endl;
    float posX_before_dodge = player.position.x;
    pressKey(combat, player, mousePos, sf::Keyboard::Key::LShift);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "Spot Dodge active: isSpotDodging=" << player.isSpotDodging() 
              << ", isInvulnerable=" << player.isInvulnerable() 
              << ", isPerfectDodgeActive=" << player.isPerfectDodgeActive() 
              << ", deltaX=" << (player.position.x - posX_before_dodge) << std::endl;
    assert(player.isSpotDodging());
    assert(player.isInvulnerable());
    assert(player.isPerfectDodgeActive());
    assert(player.position.x == posX_before_dodge); // Stayed strictly on place!
    std::cout << "-> SPOT DODGE CONFIRMED (on place & invincible)!" << std::endl;

    // -------------------------------------------------------------
    // 3. VERIFY PERFECT DODGE & COUNTER-ATTACK
    // -------------------------------------------------------------
    std::cout << "\n[TEST 3] Testing Perfect Dodge and Counter-Strike..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    enemy.reset();
    tick(0.05f, arena, player, enemy, combat, mousePos);

    // Place enemy right in front of player
    enemy.position = {player.position.x + 40.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
    enemy.isFacingLeft = true;

    // Enemy triggers attack (0.38s windup)
    enemy.triggerAttack();

    // Player waits for windup telegraph
    for (int i = 0; i < 35; ++i) {
        tick(0.01f, arena, player, enemy, combat, mousePos);
    }
    assert(enemy.isWindingUp());

    // Right before strike lands, player spot dodges!
    pressKey(combat, player, mousePos, sf::Keyboard::Key::LShift);
    assert(player.isPerfectDodgeActive());

    // Attack becomes active and intersects player in perfect dodge window!
    for (int i = 0; i < 5; ++i) {
        tick(0.01f, arena, player, enemy, combat, mousePos);
        if (player.isPerfectDodgeEmpowered()) break;
    }

    std::cout << "After Perfect Dodge: player empowered = " << player.isPerfectDodgeEmpowered() 
              << ", enemy hurtTimer = " << enemy.hurtTimer << std::endl;
    assert(player.isPerfectDodgeEmpowered());
    assert(enemy.hurtTimer > 0.0f); // Enemy was staggered!
    std::cout << "-> PERFECT DODGE TRIGGERED & ENEMY STAGGERED!" << std::endl;

    // Now Player counter-attacks with Light attack
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.02f, arena, player, enemy, combat, mousePos);
    std::cout << "Counter Attack name: " << combat.getCurrentComboName() 
              << ", enemy hp = " << enemy.hp << std::endl;
    assert(combat.getCurrentComboName().find("COUNTER") != std::string::npos);
    assert(enemy.hp < 100.0f);
    std::cout << "-> PERFECT COUNTER-STRIKE EXECUTED SUCCESSFULLY!" << std::endl;

    // -------------------------------------------------------------
    // 4. VERIFY TIGHT COMBO TIMING
    // -------------------------------------------------------------
    std::cout << "\n[TEST 4] Testing Tight Combo Timing Window..." << std::endl;
    combat.reset();
    player.reset(arena.getSpawnPosition());
    enemy.reset();
    tick(0.05f, arena, player, enemy, combat, mousePos);

    // Hit 1: Light
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.02f, arena, player, enemy, combat, mousePos);
    assert(combat.getComboHistory().size() == 1);

    // Wait 0.45 seconds (exceeding the new tight 0.42s window)
    for (int i = 0; i < 45; ++i) tick(0.01f, arena, player, enemy, combat, mousePos);

    // Next hit should reset combo chain because player was too slow!
    pressKey(combat, player, mousePos, sf::Keyboard::Key::J);
    tick(0.02f, arena, player, enemy, combat, mousePos);

    std::cout << "History after slow press: size=" << combat.getComboHistory().size() << " (was reset!)" << std::endl;
    assert(combat.getComboHistory().size() == 1); // Reset to 1 instead of chaining to 2!
    std::cout << "-> TIGHT COMBO TIMING CONFIRMED!" << std::endl;

    std::cout << "\n=======================================================" << std::endl;
    std::cout << "[ALL TESTS PASSED] FACING, SPOT DODGE, PERFECT DODGE & TIGHT TIMINGS VERIFIED!" << std::endl;
    std::cout << "=======================================================" << std::endl;
    return 0;
}
