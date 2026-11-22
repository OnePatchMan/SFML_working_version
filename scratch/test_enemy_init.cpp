#include <iostream>
#include "world/Arena.h"
#include "entities/Enemy.h"

int main() {
    Arena arena;
    arena.loadRoom(1);
    Enemy enemy(sf::Vector2f(200.0f, 132.0f), EnemyType::NORMAL);
    enemy.behaviorState = AIBehaviorState::CHASE;
    sf::Vector2f playerPos(360.0f, 196.0f);

    std::cout << "Before update: pos=(" << enemy.position.x << "," << enemy.position.y << ")" << std::endl;
    std::cout << "overlapsSolid at (200, 132): " << arena.overlapsSolid(200.0f, 132.0f, enemy.halfW, enemy.halfH, enemy.halfH) << std::endl;
    std::cout << "overlapsSolid at (200, 132+1): " << arena.overlapsSolid(200.0f, 133.0f, enemy.halfW, enemy.halfH, enemy.halfH) << std::endl;
    enemy.update(0.016f, arena, playerPos);
    std::cout << "After update 1: pos=(" << enemy.position.x << "," << enemy.position.y << ") vy=" << enemy.velocity.y << std::endl;
    return 0;
}
