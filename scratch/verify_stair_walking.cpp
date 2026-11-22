
#include <iostream>
#include <cassert>
#include <cmath>
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"

int main() {
    Arena arena;
    arena.loadRoom(1);

    // tileSize = 64
    // Upper floor row 5: floor at y = 5 * 64 = 320. Enemy center y = 320 - 28 = 292
    // Lower floor row 7: floor at y = 7 * 64 = 448. Player center y = 448 - 28 = 420
    // Stairs are at col 8-9 (x = 512 to 640)
    sf::Vector2f playerPos(750.0f, 420.0f);
    Enemy enemy(sf::Vector2f(350.0f, 292.0f), EnemyType::NORMAL);
    enemy.forceDetectPlayer(playerPos);

    std::cout << "Testing enemy walking DOWN stairs in room_1..." << std::endl;
    int jumpCount = 0;
    for (int frame = 0; frame < 240; ++frame) {
        enemy.update(0.016f, arena, playerPos);
        if (enemy.velocity.y < -300.0f) {
            jumpCount++;
        }
    }
    std::cout << "Down - Enemy final pos: x=" << enemy.position.x << ", y=" << enemy.position.y << ", jumpCount=" << jumpCount << std::endl;
    assert(jumpCount == 0);
    assert(enemy.position.x > 640.0f); // Reached lower floor smoothly!
    std::cout << "-> PASSED: Enemy walked DOWN stairs without jumping!" << std::endl;

    // Test enemy walking UP stairs in room_1
    std::cout << "Testing enemy walking UP stairs in room_1..." << std::endl;
    playerPos = sf::Vector2f(350.0f, 292.0f);
    enemy.position = sf::Vector2f(750.0f, 420.0f);
    enemy.velocity = {0.0f, 0.0f};
    enemy.forceDetectPlayer(playerPos);
    jumpCount = 0;
    for (int frame = 0; frame < 240; ++frame) {
        enemy.update(0.016f, arena, playerPos);
        if (enemy.velocity.y < -300.0f) {
            jumpCount++;
        }
    }
    std::cout << "Up - Enemy final pos: x=" << enemy.position.x << ", y=" << enemy.position.y << ", jumpCount=" << jumpCount << std::endl;
    assert(jumpCount == 0);
    assert(enemy.position.x < 512.0f); // Reached upper floor smoothly!
    std::cout << "-> PASSED: Enemy walked UP stairs without jumping!" << std::endl;

    return 0;
}
