#include "core/Common.h"
#include "world/Arena.h"
#include "entities/Player.h"
#include "entities/Enemy.h"
#include "combat/CombatSystem.h"
#include "combat/CombatConfig.h"
#include "entities/PlayerConfig.h"
#include "entities/EnemyConfig.h"
#include <cassert>
#include <iostream>
#include <cmath>

void tick(float dt, Arena& arena, Player& player, Enemy& enemy, CombatSystem& combat, const sf::Vector2f& mousePos = {0.0f, 0.0f}) {
    player.update(dt, arena, mousePos);
    enemy.update(dt, arena, player.position);
    combat.update(dt, player, enemy);
}

void triggerLight(CombatSystem& combat, Player& player) {
    sf::Event::KeyPressed kp;
    kp.code = sf::Keyboard::Key::J;
    sf::Event event(kp);
    combat.handleInput(event, player, {0.0f, 0.0f});
}

void triggerHeavy(CombatSystem& combat, Player& player) {
    sf::Event::KeyPressed kp;
    kp.code = sf::Keyboard::Key::K;
    sf::Event event(kp);
    combat.handleInput(event, player, {0.0f, 0.0f});
}

void triggerSpotDodge(Player& player) {
    sf::Event::KeyPressed kp;
    kp.code = sf::Keyboard::Key::LShift;
    sf::Event event(kp);
    player.handleInput(event);
}

void triggerCounter(Player& player) {
    sf::Event::KeyPressed kp;
    kp.code = sf::Keyboard::Key::Q;
    sf::Event event(kp);
    player.handleInput(event);
}

int main() {
    Arena arena(ARENA_WIDTH, ARENA_HEIGHT);
    Player player(arena.getSpawnPosition());
    Enemy enemy({player.position.x + 40.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)});
    CombatSystem combat;

    std::cout << "======================================================" << std::endl;
    std::cout << "STARTING DIRECTIONAL COMBOS & BALANCING VERIFICATION" << std::endl;
    std::cout << "======================================================" << std::endl;

    // -------------------------------------------------------------
    // TEST 1: COMBO 1f2d1 (Light 1 -> Forward Heavy f2 -> Dash Light d1)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 1] Testing Combo 1f2d1..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        enemy.reset();
        tick(0.1f, arena, player, enemy, combat); // Land on ground
        assert(player.isOnGround());
        enemy.position = {player.position.x + 40.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // 1. Strike 1: neutral light
        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);
        tick(0.12f, arena, player, enemy, combat);

        // 2. Strike f2: forward heavy
        player.setSimulatedMoveDir(1.0f); // Holding forward
        triggerHeavy(combat, player);
        player.setSimulatedMoveDir(0.0f);
        tick(0.05f, arena, player, enemy, combat);
        tick(0.20f, arena, player, enemy, combat);

        // 3. Strike d1: dash light
        sf::Event::KeyPressed shiftEv;
        shiftEv.code = sf::Keyboard::Key::LShift;
        sf::Event ev(shiftEv);
        player.setSimulatedMoveDir(1.0f);
        player.handleInput(ev); // Starts dash
        player.setSimulatedMoveDir(0.0f);
        assert(player.isDashing() || player.isDashWindow());

        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);

        std::cout << "Current combo: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("1-f2-d1") != std::string::npos);
        assert(combat.getCurrentComboName().find("DIAGONAL LAUNCHER") != std::string::npos);
        std::cout << "-> PASSED: Combo 1f2d1 matched and triggered correctly!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 2: COMBO 11b2 (Grab & Back Throw + Turn Around, Facing Protection)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 2] Testing Combo 11b2 (Grab & Back Throw + Turn Around)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        // Start enemy at standard combat engagement distance (42px)
        enemy.position = {player.position.x + 42.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // Strike 1: 1 (hits and pushes enemy with knockX)
        triggerLight(combat, player);
        for (int i = 0; i < 9; ++i) tick(0.016f, arena, player, enemy, combat);

        // Strike 2: 1 (hits and pushes enemy further with knockX)
        triggerLight(combat, player);
        for (int i = 0; i < 9; ++i) tick(0.016f, arena, player, enemy, combat);

        float distBeforeGrab = enemy.position.x - player.position.x;
        std::cout << "Enemy distance after 2 light attacks (pushed by force): " << distBeforeGrab << " px" << std::endl;

        // Strike 3: b2 (holding back)
        // Verify that facing stays locked during combo and does NOT turn around from holding A!
        player.setSimulatedMoveDir(-1.0f); // Backwards
        assert(player.isFacingRight()); // Facing protected!
        triggerHeavy(combat, player);
        player.setSimulatedMoveDir(0.0f);

        // Hit occurs: lengthened hitbox (reaching +115px) must connect and grab!
        tick(0.04f, arena, player, enemy, combat);

        std::cout << "Current combo: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("1-1-b2") != std::string::npos);
        assert(combat.getCurrentComboName().find("BACK THROW") != std::string::npos);

        // Enemy should be thrown backward (< 0) with jump-level velocity (-425)
        std::cout << "Enemy velocity: x=" << enemy.velocity.x << ", y=" << enemy.velocity.y << std::endl;
        assert(enemy.velocity.x < -400.0f);
        assert(enemy.velocity.y < -400.0f);

        // Player should have turned around to face the thrown enemy!
        std::cout << "Player facing right: " << player.isFacingRight() << std::endl;
        assert(!player.isFacingRight()); // Now facing left!

        std::cout << "-> PASSED: Combo 11b2 threw enemy backwards and player turned around!" << std::endl;

        // Additional reach check: verify 11b2 connects even when enemy is 85px away (pushed far by force)
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        enemy.position = {player.position.x + 85.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // Pre-fill combo history with {"1", "1"}
        triggerLight(combat, player);
        tick(0.12f, arena, player, enemy, combat);
        triggerLight(combat, player);
        tick(0.12f, arena, player, enemy, combat);
        // Execute b2 from 85px away
        player.setSimulatedMoveDir(-1.0f);
        triggerHeavy(combat, player);
        player.setSimulatedMoveDir(0.0f);
        tick(0.04f, arena, player, enemy, combat);

        assert(combat.getCurrentComboName().find("1-1-b2") != std::string::npos);
        assert(enemy.velocity.x < -400.0f);
        std::cout << "-> PASSED: Combo 11b2 successfully reached and grabbed enemy at 85px distance!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 3: COMBO CANCELING 2 -> 1 (INSTANT CANCEL OF HEAVY RECOVERY)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 3] Testing Instant Combo Cancel 2 -> 1 (No waiting for full cooldown)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // Strike 1: Heavy 2
        triggerHeavy(combat, player);
        // Advance only 0.04s: attack connected, but cooldown (0.24s) has 0.20s remaining!
        tick(0.04f, arena, player, enemy, combat);
        assert(combat.getCooldown() > 0.15f); // Substantial cooldown would normally block!

        // Press 1 immediately without waiting for cooldown!
        triggerLight(combat, player);
        // Advance a frame: combo cancel should have triggered 1 immediately!
        tick(0.04f, arena, player, enemy, combat);

        std::cout << "Current combo after instant cancel: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("2-1") != std::string::npos);
        std::cout << "Enemy velocity after 21: x=" << enemy.velocity.x << std::endl;
        assert(enemy.velocity.x > 700.0f); // ~850 knockX setup for dash behind

        // Strike 3: db1 (dash + backward + light)
        sf::Event::KeyPressed shiftEv;
        shiftEv.code = sf::Keyboard::Key::LShift;
        sf::Event ev(shiftEv);
        player.setSimulatedMoveDir(1.0f);
        player.handleInput(ev);
        player.setSimulatedMoveDir(0.0f);

        player.setSimulatedMoveDir(-1.0f); // Backwards in dash -> db1
        triggerLight(combat, player);
        player.setSimulatedMoveDir(0.0f);

        tick(0.04f, arena, player, enemy, combat);
        std::cout << "Current combo: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("2-1-db1") != std::string::npos);
        assert(combat.getCurrentComboName().find("REVERSE SWEEP LAUNCHER") != std::string::npos);
        // Enemy launched upwards into air
        std::cout << "Enemy velocity after db1: y=" << enemy.velocity.y << std::endl;
        assert(enemy.velocity.y < -400.0f);
        std::cout << "-> PASSED: Combo 21 canceled heavy recovery instantly and executed combo smoothly!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 4: VERTICAL LAUNCH HEIGHT (JUMP + 10-15 PIXELS = 145-150 PX)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 4] Testing Vertical Launch Apex Height (Jump + 10-15px)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        float groundY = enemy.position.y;

        // Player jump height check:
        // JUMP_SPEED = 900, GRAVITY = 3000 -> H = 900^2 / (2 * 3000) = 135 px
        float expectedJumpHeight = (Player::JUMP_SPEED * Player::JUMP_SPEED) / (2.0f * Player::GRAVITY);
        std::cout << "Player jump height: " << expectedJumpHeight << " px" << std::endl;
        assert(std::abs(expectedJumpHeight - 135.0f) < 0.1f);

        // Perform launcher: u2 (Uppercut)
        player.setSimulatedVerticalDir(-1.0f); // Up
        triggerHeavy(combat, player);
        player.setSimulatedVerticalDir(0.0f);

        float initialEnemyY = enemy.position.y;
        float peakEnemyHeight = 0.0f;

        // Trace enemy flight to peak
        for (int i = 0; i < 60; ++i) {
            tick(0.016f, arena, player, enemy, combat);
            float currentRise = initialEnemyY - enemy.position.y;
            if (currentRise > peakEnemyHeight) peakEnemyHeight = currentRise;
            if (enemy.velocity.y > 0.0f) break; // Reached apex and started falling
        }

        std::cout << "Enemy launcher peak height: " << peakEnemyHeight << " px (Jump + " 
                  << (peakEnemyHeight - expectedJumpHeight) << " px)" << std::endl;
        // Must be in [145, 150] px range (Jump 135 + 10 to 15 px)
        assert(peakEnemyHeight >= 144.0f && peakEnemyHeight <= 151.0f);
        std::cout << "-> PASSED: Vertical launcher lifted enemy precisely to jump height + 10-15px!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 5: COMBO a1a1adw2 (Dual Air Light -> Dive Slam + Ground Bounce)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 5] Testing Combo a1a1adw2..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        player.position.y -= 350.0f; // High in air
        player.velocity.y = 0.0f;
        enemy.reset();
        enemy.position = {player.position.x + 35.0f, player.position.y};
        enemy.velocity.y = 0.0f;

        // Air Strike 1: a1
        triggerLight(combat, player);
        tick(0.04f, arena, player, enemy, combat);
        tick(0.14f, arena, player, enemy, combat);

        // Air Strike 2: a1
        triggerLight(combat, player);
        tick(0.04f, arena, player, enemy, combat);
        tick(0.14f, arena, player, enemy, combat);

        // Air Strike 3: adw2 (holding down)
        player.setSimulatedVerticalDir(1.0f); // Down
        triggerHeavy(combat, player);
        player.setSimulatedVerticalDir(0.0f);

        tick(0.04f, arena, player, enemy, combat);
        std::cout << "Current combo: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("a1-a1-adw2") != std::string::npos);
        assert(combat.getCurrentComboName().find("DUAL AIR DIVE SLAM") != std::string::npos);

        // Both dive down together
        std::cout << "Player vel y: " << player.velocity.y << ", Enemy vel y: " << enemy.velocity.y << std::endl;
        assert(player.velocity.y > 1000.0f);
        assert(enemy.velocity.y > 1000.0f);
        assert(enemy.diveSlamBounce == true);

        // Advance ticks until enemy hits solid floor and bounces
        for (int i = 0; i < 30; ++i) {
            tick(0.016f, arena, player, enemy, combat);
            if (!enemy.diveSlamBounce) break;
        }

        std::cout << "After landing: enemy vel x=" << enemy.velocity.x << ", y=" << enemy.velocity.y << std::endl;
        assert(enemy.velocity.y < -200.0f);
        assert(enemy.velocity.x > 800.0f);
        std::cout << "-> PASSED: Combo a1a1adw2 dive slammed and bounced with dash distance!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 6: COMBO a1a2 (Horizontal Air Push)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 6] Testing Combo a1a2 (Horizontal Air Push)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        player.position.y -= 100.0f;
        enemy.reset();
        enemy.position = {player.position.x + 35.0f, player.position.y};

        // a1
        triggerLight(combat, player);
        tick(0.04f, arena, player, enemy, combat);
        tick(0.14f, arena, player, enemy, combat);

        // a2
        triggerHeavy(combat, player);
        tick(0.04f, arena, player, enemy, combat);

        std::cout << "Current combo: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("a1-a2") != std::string::npos);
        assert(combat.getCurrentComboName().find("HORIZONTAL AIR PUSH") != std::string::npos);
        std::cout << "Enemy velocity: x=" << enemy.velocity.x << ", y=" << enemy.velocity.y << std::endl;
        assert(enemy.velocity.x > 1000.0f);
        assert(std::abs(enemy.velocity.y) < 1.0f); // Strictly horizontal
        std::cout << "-> PASSED: Combo a1a2 pushed horizontally!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 7: REPEATED COMBO ANTI-SPAM EXHAUSTION STUN-LOCK
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 7] Testing Repeated Combo Anti-Spam (Stun-lock on 2nd repeat)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // First execution of combo 111
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);

        assert(combat.getCurrentComboName().find("1-1-1") != std::string::npos);
        assert(!player.isStunned());
        std::cout << "Combo 1-1-1 executed first time. Player stunned: " << player.isStunned() << std::endl;

        // Wait for combo reset timer (0.42s)
        tick(0.48f, arena, player, enemy, combat);
        assert(!player.isStunned());

        // Second execution of EXACT SAME combo 111
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);

        std::cout << "Combo 1-1-1 executed second time! Checking player stun-lock..." << std::endl;
        std::cout << "Player isStunned: " << player.isStunned() 
                  << ", stun timer: " << player.getStunTimer() << std::endl;
        assert(player.isStunned());
        assert(player.getStunTimer() > 0.5f);
        assert(combat.getCurrentComboName().find("STUNNED") != std::string::npos);

        // While stunned, input should be blocked
        triggerLight(combat, player);
        assert(player.isStunned());

        // Wait until stun expires
        tick(0.85f, arena, player, enemy, combat);
        assert(!player.isStunned());
        std::cout << "-> PASSED: Player correctly entered 0.75s stun-lock on 2nd combo repetition within 3s!" << std::endl;

        // -------------------------------------------------------------
        // Now test: Repeating the same combo AFTER 3 seconds has passed
        // -------------------------------------------------------------
        std::cout << "\nTesting repeated combo AFTER 3-second window expires..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);
        enemy.reset();
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // First execution of combo 111
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);

        assert(combat.getCurrentComboName().find("1-1-1") != std::string::npos);
        assert(!player.isStunned());
        assert(combat.getComboRepeatTimer() > 2.5f); // 3.0s window active

        // Wait 3.2 seconds (> 3.0s window)
        for (int i = 0; i < 200; ++i) {
            tick(0.016f, arena, player, enemy, combat);
        }
        assert(combat.getComboRepeatTimer() <= 0.0f); // Window has expired!
        assert(!player.isStunned());

        // Execute combo 111 again after 3 seconds: MUST NOT STUN!
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.15f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.05f, arena, player, enemy, combat);

        assert(combat.getCurrentComboName().find("1-1-1") != std::string::npos);
        std::cout << "Combo 1-1-1 executed after 3.2s. Player isStunned: " << player.isStunned() << std::endl;
        assert(!player.isStunned()); // NOT stunned!
        std::cout << "-> PASSED: Executing same combo after 3 seconds is completely safe and does NOT stun!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 8: SMART AI ENEMY COMBO CHAINING & JUGGLE LINKING
    // (1f2d1 Launcher -> Air a1adw2 Ground Bounce -> Chase Advance -> Air a1a1adw2 Dive Slam)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 8] Testing Smart AI Enemy Combo Chaining & Juggle Linking..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);

        // Configure enemy as Smart AI facing player from 45px away
        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 45.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        std::cout << "Initial: Player at (" << player.position.x << ", " << player.position.y << "), Enemy at ("
                  << enemy.position.x << ", " << enemy.position.y << ")" << std::endl;

        // Trigger combo sequence manually (Starter 0: 1f2d1)
        enemy.triggerComboSequence(0);
        assert(enemy.isComboSequenceActive());

        bool sawLauncher = false;
        bool playerLaunchedInAir = false;
        bool sawAirSlam = false;
        bool playerGroundBounced = false;
        bool sawChaseAdvance = false;
        bool sawDiveSlam = false;
        bool fullComboFinished = false;

        // Simulate 4.5 seconds of combat steps
        for (int frame = 0; frame < 280; ++frame) {
            tick(0.016f, arena, player, enemy, combat);

            // 1. Detect ground launcher (1f2d1)
            if (enemy.aiComboState == AIComboState::LAUNCHER_D1_ACTIVE || 
                enemy.aiComboState == AIComboState::LAUNCHER_D1_RECOVERY) {
                sawLauncher = true;
            }

            // Verify player was launched into air by launcher knockY (-435.0f)
            if (sawLauncher && player.velocity.y < -200.0f) {
                playerLaunchedInAir = true;
            }

            // 2. Detect first air combo (a1adw2)
            if (enemy.aiComboState == AIComboState::AIR_1_ADW2_ACTIVE || 
                enemy.aiComboState == AIComboState::AIR_1_ADW2_RECOVERY) {
                sawAirSlam = true;
            }

            // Verify Ground Bounce from adw2 slam (player velocity.y bouncing upwards)
            if (sawAirSlam && player.velocity.y < -150.0f) {
                playerGroundBounced = true;
            }

            // 3. Detect chase advance on ground (enemy running forward to close the knockX gap)
            if (enemy.aiComboState == AIComboState::CHASE_GROUND_ADVANCE) {
                sawChaseAdvance = true;
            }

            // 4. Detect second air combo finisher (a1a1adw2 dive slam)
            if (enemy.aiComboState == AIComboState::AIR_2_ADW2_ACTIVE || 
                enemy.aiComboState == AIComboState::AIR_2_ADW2_RECOVERY) {
                sawDiveSlam = true;
            }

            if (enemy.aiComboState == AIComboState::COMBO_FINISHED) {
                fullComboFinished = true;
                break;
            }
        }

        std::cout << "Combo verification checklist:" << std::endl;
        std::cout << "  1. Saw Ground Launcher (1f2d1): " << sawLauncher << std::endl;
        std::cout << "  2. Player launched into air: " << playerLaunchedInAir << std::endl;
        std::cout << "  3. Saw Air Slam (a1adw2): " << sawAirSlam << std::endl;
        std::cout << "  4. Player performed Ground Bounce: " << playerGroundBounced << std::endl;
        std::cout << "  5. Saw Ground Advance Chase (+72px): " << sawChaseAdvance << std::endl;
        std::cout << "  6. Saw Dual Air Dive Slam (a1a1adw2): " << sawDiveSlam << std::endl;
        std::cout << "  7. Full combo chain completed: " << fullComboFinished << std::endl;

        assert(sawLauncher);
        assert(playerLaunchedInAir);
        assert(sawAirSlam);
        assert(playerGroundBounced);
        assert(sawChaseAdvance);
        assert(sawDiveSlam);
        assert(fullComboFinished);

        std::cout << "-> PASSED: Smart AI successfully linked 1f2d1 -> Jump a1adw2 -> Chase -> Jump a1a1adw2!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 9: HIT CONFIRM / COMBO DROPPING ON WHIFF
    // (If AI misses the player, it MUST drop combo and not continue)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 9] Testing Hit Confirm / Combo Dropping on Whiff (Missing target)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        tick(0.1f, arena, player, enemy, combat);

        // Position player 150px away from enemy (out of attack range)
        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 150.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Force AI into LAUNCHER_1_ACTIVE directly
        enemy.aiComboState = AIComboState::LAUNCHER_1_ACTIVE;
        enemy.aiStateTimer = 0.12f;
        enemy.attackState = EnemyAttackState::ATTACKING;
        enemy.attackStateTimer = 0.12f;
        enemy.attackHasHit = false;

        // Simulate until active window ends (0.15s)
        for (int frame = 0; frame < 15; ++frame) {
            tick(0.016f, arena, player, enemy, combat);
        }

        std::cout << "After whiff: AI combo state=" << static_cast<int>(enemy.aiComboState) 
                  << " (0 is IDLE), attackHasHit=" << enemy.attackHasHit << std::endl;
        // Verify combo was DROPPED
        assert(enemy.aiComboState == AIComboState::IDLE);
        assert(!enemy.attackHasHit);
        assert(enemy.getCurrentComboName().empty());
        std::cout << "-> PASSED: AI immediately dropped combo on whiff!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 10: HIT CONFIRM / COMBO DROPPING ON PLAYER DODGE
    // (If player dodges through attack with i-frames, AI drops combo)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 10] Testing Combo Dropping when Player Dodges (i-frames)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        tick(0.1f, arena, player, enemy, combat);

        // Position enemy right next to player (within range)
        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Player spot-dodges with Shift (gives i-frames)
        triggerSpotDodge(player);
        assert(player.isInvulnerable());

        // AI starts attack
        enemy.aiComboState = AIComboState::LAUNCHER_1_ACTIVE;
        enemy.aiStateTimer = 0.12f;
        enemy.attackState = EnemyAttackState::ATTACKING;
        enemy.attackStateTimer = 0.12f;
        enemy.attackHasHit = false;

        // Tick 2 frames to intersect hitbox while player is invulnerable
        tick(0.016f, arena, player, enemy, combat);
        tick(0.016f, arena, player, enemy, combat);

        std::cout << "After dodge: AI combo state=" << static_cast<int>(enemy.aiComboState) 
                  << ", attackHasHit=" << enemy.attackHasHit << ", player hp=" << player.hp << std::endl;

        // Player took 0 damage because they were invulnerable
        assert(player.hp == player.maxHp);
        // AI dropped combo immediately!
        assert(enemy.aiComboState == AIComboState::IDLE);
        assert(!enemy.attackHasHit);
        std::cout << "-> PASSED: AI immediately dropped combo when player dodged!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 11: SMART AI STARTER 2 (112 Rapid Combo Launcher)
    // (1 -> 1 -> 2 launcher -> Air a1adw2 -> Chase -> a1a1adw2)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 11] Testing Smart AI Starter 2: 112 (Rapid Combo Launcher)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);

        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 45.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Trigger Starter 1: 112
        enemy.triggerComboSequence(1);
        assert(enemy.isComboSequenceActive());
        assert(enemy.getCurrentStarterType() == AIStarterType::STARTER_112);

        bool saw112Launcher = false;
        bool playerLaunched = false;
        bool sawAirCombo = false;
        bool fullComboFinished = false;

        for (int frame = 0; frame < 280; ++frame) {
            tick(0.016f, arena, player, enemy, combat);

            if (enemy.aiComboState == AIComboState::STARTER_112_2_ACTIVE ||
                enemy.aiComboState == AIComboState::STARTER_112_2_RECOVERY) {
                saw112Launcher = true;
            }
            if (saw112Launcher && player.velocity.y < -200.0f) {
                playerLaunched = true;
            }
            if (enemy.aiComboState == AIComboState::AIR_1_ADW2_ACTIVE) {
                sawAirCombo = true;
            }
            if (enemy.aiComboState == AIComboState::COMBO_FINISHED) {
                fullComboFinished = true;
                break;
            }
        }

        std::cout << "Starter 112 Checklist: sawLauncher=" << saw112Launcher 
                  << ", playerLaunched=" << playerLaunched
                  << ", sawAirCombo=" << sawAirCombo 
                  << ", fullComboFinished=" << fullComboFinished << std::endl;

        assert(saw112Launcher);
        assert(playerLaunched);
        assert(sawAirCombo);
        assert(fullComboFinished);
        std::cout << "-> PASSED: Smart AI successfully linked starter 112 into aerial combo!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 12: SMART AI STARTER 3 (21db1 Reverse Sweep Vertical Launcher)
    // (Heavy 2 -> Follow-up 1 -> Dash Behind & Sweep db1 -> Vertical Air Combo in place)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 12] Testing Smart AI Starter 3: 21db1 (Reverse Sweep Vertical Launcher)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        tick(0.1f, arena, player, enemy, combat);

        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 48.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Trigger Starter 2: 21db1
        enemy.triggerComboSequence(2);
        assert(enemy.isComboSequenceActive());
        assert(enemy.getCurrentStarterType() == AIStarterType::STARTER_21DB1);

        bool sawHeavy2 = false;
        bool sawSweep = false;
        bool verticalLaunch = false;
        bool jumpInPlace = false;
        bool fullComboFinished = false;

        for (int frame = 0; frame < 280; ++frame) {
            tick(0.016f, arena, player, enemy, combat);

            if (enemy.aiComboState == AIComboState::STARTER_21DB1_2_ACTIVE) {
                sawHeavy2 = true;
            }
            if (enemy.aiComboState == AIComboState::STARTER_21DB1_SWEEP_ACTIVE) {
                sawSweep = true;
            }
            if (sawSweep && !verticalLaunch && player.velocity.y < -200.0f) {
                verticalLaunch = true;
                // Verify player horizontal knockback was 0 (strictly vertical launch!)
                std::cout << "Vertical launch player velX=" << player.velocity.x << ", velY=" << player.velocity.y << std::endl;
                assert(std::abs(player.velocity.x) < 5.0f);
            }
            if (enemy.aiComboState == AIComboState::CHASE_JUMP_1) {
                // Verify enemy is jumping on the spot without moving forward (velocity.x == 0)
                if (std::abs(enemy.velocity.x) < 1.0f) {
                    jumpInPlace = true;
                }
            }
            if (enemy.aiComboState == AIComboState::COMBO_FINISHED) {
                fullComboFinished = true;
                break;
            }
        }

        std::cout << "Starter 21db1 Checklist: sawHeavy2=" << sawHeavy2
                  << ", sawSweep=" << sawSweep
                  << ", verticalLaunch=" << verticalLaunch
                  << ", jumpInPlace=" << jumpInPlace
                  << ", fullComboFinished=" << fullComboFinished << std::endl;

        assert(sawHeavy2);
        assert(sawSweep);
        assert(verticalLaunch);
        assert(jumpInPlace);
        assert(fullComboFinished);
        std::cout << "-> PASSED: Smart AI starter 21db1 lifted vertically and jumped in place without forward motion!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 13: AGGRESSIVE AI PURSUIT IN IDLE (Does not stand still)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 13] Testing Aggressive AI Pursuit in IDLE (Does not stand still)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        tick(0.1f, arena, player, enemy, combat);

        // Place enemy 120px away in IDLE with cooldown active
        enemy.reset();
        enemy.setSmartAI(true);
        enemy.position = {player.position.x + 120.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;
        enemy.attackCooldown = 0.5f; // cooldown is active
        enemy.aiComboState = AIComboState::IDLE;

        float startDist = std::abs(player.position.x - enemy.position.x);

        // Tick 6 frames in IDLE
        for (int frame = 0; frame < 6; ++frame) {
            tick(0.016f, arena, player, enemy, combat);
        }

        float currentDist = std::abs(player.position.x - enemy.position.x);
        std::cout << "Initial dist=" << startDist << " px, after 6 frames dist=" << currentDist 
                  << " px, enemy velX=" << enemy.velocity.x << std::endl;

        // Enemy should actively close distance even while cooling down!
        assert(currentDist < startDist);
        assert(enemy.velocity.x < 0.0f); // moving left towards player
        std::cout << "-> PASSED: AI actively pursues player instead of standing motionless!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 14: WHIFF STUNLOCK ON MISS (Longer Stun Penalty on Missed Swing)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 14] Testing Whiff Stunlock on Missed Swing..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        enemy.reset();
        enemy.setSmartAI(false);
        // Enemy is far away (250px), completely out of range of attack 1
        enemy.position = {player.position.x + 250.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        tick(0.1f, arena, player, enemy, combat);

        assert(!player.isStunned());

        // Player swings attack 1 in empty air
        triggerLight(combat, player);
        assert(!player.isStunned());

        // Tick through active frames of attack 1 (0.11s)
        for (int i = 0; i < 9; ++i) {
            tick(0.016f, arena, player, enemy, combat);
        }

        // Active frames ended without hitting!
        std::cout << "Whiff check: player isStunned=" << player.isStunned() 
                  << ", stunTimer=" << player.getStunTimer()
                  << ", hasHitThisSwing=" << combat.hasHitThisSwing() << std::endl;

        assert(!combat.hasHitThisSwing());
        assert(player.isStunned());
        assert(combat.getLastStunReason() == CombatSystem::StunReason::WHIFF);
        // Whiff stun is >= 0.38s (much longer than normal 0.14s recovery!)
        assert(player.getStunTimer() >= 0.30f);

        // While in whiff stunlock, cannot attack again (combo cancel blocked!)
        triggerLight(combat, player);
        triggerHeavy(combat, player);
        // Remains stunned
        assert(player.isStunned());

        // Wait until whiff stunlock expires
        for (int i = 0; i < 30; ++i) {
            tick(0.016f, arena, player, enemy, combat);
        }

        assert(!player.isStunned());
        assert(combat.getLastStunReason() == CombatSystem::StunReason::NONE);
        std::cout << "-> PASSED: Whiff stunlock correctly punished missed attack with extended stun!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 15: HIT CONFIRMATION ALLOWS COMBO CANCEL WITHOUT WHIFF STUN
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 15] Testing Hit-Confirmed Attack (No Whiff Stun, Allows Combo Cancel)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        enemy.reset();
        enemy.setSmartAI(false);
        // Enemy is right in front of player (35px)
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        tick(0.1f, arena, player, enemy, combat);

        // Player swings attack 1
        triggerLight(combat, player);

        // Hit registers on first tick
        tick(0.016f, arena, player, enemy, combat);
        assert(combat.hasHitThisSwing());
        assert(!player.isStunned());

        // Cancel into attack 2 immediately during combo window
        triggerHeavy(combat, player);
        tick(0.016f, arena, player, enemy, combat);

        // Player is NOT stunned and combo has progressed!
        assert(!player.isStunned());
        assert(combat.getComboHistory().size() >= 2);
        std::cout << "-> PASSED: Hit-confirmed attack cancels seamlessly without whiff stunlock penalty!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 16: DUMMY ENEMY WHIFF EXTENDED RECOVERY
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 16] Testing Enemy Whiff Recovery Penalty..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(false);
        // Player is far away (300px)
        enemy.position = {player.position.x + 300.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        tick(0.1f, arena, player, enemy, combat);

        // Trigger enemy attack
        enemy.triggerAttack();
        assert(enemy.isWindingUp());

        // Wait through windup (0.38s)
        while (enemy.isWindingUp()) {
            tick(0.016f, arena, player, enemy, combat);
        }
        assert(enemy.isAttacking());
        assert(!enemy.attackHasHit);

        // Wait through active attacking phase (0.16s)
        while (enemy.isAttacking()) {
            tick(0.016f, arena, player, enemy, combat);
        }

        // Enemy whiffed! Recovery should be 0.50s (longer than hit recovery of 0.20s)
        std::cout << "Enemy attackState=" << static_cast<int>(enemy.attackState)
                  << ", attackStateTimer=" << enemy.attackStateTimer
                  << ", attackHasHit=" << enemy.attackHasHit << std::endl;

        assert(enemy.attackState == EnemyAttackState::RECOVERY);
        assert(!enemy.attackHasHit);
        assert(enemy.attackStateTimer >= 0.40f);
        std::cout << "-> PASSED: Enemy whiff properly triggers extended recovery penalty (0.50s)!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 17: KEY-VALUES (KV) BALANCE TABLE & HOT-RELOAD
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 17] Testing Key-Values (KV) Balance Table & Hot-Reload..." << std::endl;
        // Verify KV accessors
        assert(CombatConfig::hasAttack("1"));
        assert(CombatConfig::hasAttack("f2"));
        assert(CombatConfig::hasAttack("d1"));
        assert(CombatConfig::hasAttack("adw2"));
        assert(CombatConfig::hasCombo("1f2d1"));
        assert(CombatConfig::hasCombo("11b2"));
        assert(CombatConfig::hasCombo("1f2"));
        assert(CombatConfig::hasCombo("a1a1"));

        AttackStats a1 = CombatConfig::getAttack("1");
        assert(a1.damage == 2.0f);
        assert(a1.knockX == 160.0f);
        assert(a1.whiffStun == 0.38f);

        const ComboEntry* combo1f2d1 = CombatConfig::getCombo("1f2d1");
        assert(combo1f2d1 != nullptr);
        assert(combo1f2d1->stats.damage == 5.0f);
        assert(combo1f2d1->stats.knockY == -435.0f);
        assert(combo1f2d1->isUniqueFinisher == true);
        assert(combo1f2d1->isFinisher == true);

        // Test runtime modification
        AttackStats custom = a1;
        custom.damage = 99.0f;
        CombatConfig::setAttack("1", custom);
        assert(CombatConfig::getAttack("1").damage == 99.0f);

        // Test reload restores original from KV file
        CombatConfig::reload();
        assert(CombatConfig::getAttack("1").damage == 2.0f);
        std::cout << "-> PASSED: Key-Values balance table getters, setters, and hot-reload verified successfully!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 18: COMBO 112 UPWARD LAUNCHER (No floor throwing)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 18] Testing Player Combo 112 Upward Launcher (No floor throw)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        if (!player.isFacingRight()) player.turnAround();
        enemy.reset();
        enemy.setSmartAI(false);
        tick(0.1f, arena, player, enemy, combat);

        // Put enemy in front
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};

        // Step 1: Light (1)
        triggerLight(combat, player);
        tick(0.12f, arena, player, enemy, combat);
        assert(combat.hasHitThisSwing());

        // Step 2: Light (1)
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerLight(combat, player);
        tick(0.12f, arena, player, enemy, combat);
        assert(combat.hasHitThisSwing());

        // Step 3: Heavy (2) -> Completes 112
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        triggerHeavy(combat, player);
        tick(0.016f, arena, player, enemy, combat);

        std::cout << "Combo name after 112: " << combat.getCurrentComboName()
                  << ", Enemy vy=" << enemy.velocity.y 
                  << ", vx=" << enemy.velocity.x << std::endl;

        assert(combat.getCurrentComboName().find("1-1-2") != std::string::npos);
        assert(combat.getCurrentComboName().find("LAUNCHER") != std::string::npos);
        // Enemy must be launched UPWARDS (negative Y velocity), NOT down into the floor!
        assert(enemy.velocity.y < -400.0f);
        assert(enemy.velocity.x > 100.0f);

        // Verify apex height reaches jump height + 10-15px
        float startY = enemy.position.y;
        float minY = startY;
        for (int i = 0; i < 40; ++i) {
            tick(0.016f, arena, player, enemy, combat);
            if (enemy.position.y < minY) minY = enemy.position.y;
        }
        float launchHeight = startY - minY;
        std::cout << "Enemy 112 launch peak height: " << launchHeight << " px" << std::endl;
        assert(launchHeight >= 140.0f && launchHeight <= 155.0f);
        std::cout << "-> PASSED: Player combo 112 launches directly UPWARD without throwing into the floor!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 19: PLAYER LANDING WITHOUT BOUNCE (Normal fall lands solidly)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 19] Testing Player Landing Without Bounce (Solid landing)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(false);

        // Put player high up in the air (e.g. y = 300) without stun
        player.position.y = 300.0f;
        player.velocity.y = 0.0f;
        assert(!player.isStunned());

        // Let player fall under gravity until landing on the ground
        bool reachedHighFallingSpeed = false;
        for (int i = 0; i < 120; ++i) {
            tick(0.016f, arena, player, enemy, combat);
            if (player.velocity.y > 600.0f) {
                reachedHighFallingSpeed = true;
            }
            if (player.isOnGround()) {
                break;
            }
        }

        std::cout << "Player reached high falling speed: " << reachedHighFallingSpeed 
                  << ", on ground: " << player.isOnGround()
                  << ", velY: " << player.velocity.y << std::endl;
        assert(reachedHighFallingSpeed);
        assert(player.isOnGround());
        assert(player.velocity.y == 0.0f); // Solid landing, NO bounce!

        // Tick next frame: verify player remains on ground and does NOT jump/bounce up
        tick(0.016f, arena, player, enemy, combat);
        assert(player.isOnGround());
        assert(player.velocity.y >= 0.0f);
        std::cout << "-> PASSED: Player falls and lands solidly without bouncing off the ground!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 20: AIR COMBO MECHANICS a1 -> a1 -> adw2 (Either adw2 or nothing after a1a1)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 20] Testing Air Combo Mechanics: a1 -> a1 -> adw2 (only adw2 allowed after a1a1)..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(false);
        tick(0.1f, arena, player, enemy, combat);

        // Player jumps into air
        sf::Event::KeyPressed spaceEv;
        spaceEv.code = sf::Keyboard::Key::Space;
        sf::Event evSpace(spaceEv);
        player.handleInput(evSpace);
        tick(0.05f, arena, player, enemy, combat);
        assert(!player.isOnGround());

        // Place enemy in front of player at same air height
        enemy.position = {player.position.x + 35.0f, player.position.y};

        // First a1 attack
        triggerLight(combat, player);
        tick(0.02f, arena, player, enemy, combat);
        assert(combat.isAttacking());
        assert(combat.hasHitThisSwing());

        // Wait for attack active phase and recovery
        tick(0.20f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y};

        // Second a1 attack -> completes base combo a1a1 (Dual Air Juggle)
        triggerLight(combat, player);
        tick(0.02f, arena, player, enemy, combat);
        assert(combat.isAttacking());
        std::cout << "Combo after 2nd a1: " << combat.getCurrentComboName() << std::endl;
        assert(combat.getCurrentComboName().find("a1-a1") != std::string::npos);

        // Wait for attack active phase and recovery
        tick(0.20f, arena, player, enemy, combat);
        enemy.position = {player.position.x + 35.0f, player.position.y};

        // Third a1 attack: MUST BE REJECTED! ("либо adw2, либо ничего")
        triggerLight(combat, player);
        tick(0.02f, arena, player, enemy, combat);
        std::cout << "Attempted 3rd a1 after a1a1: isAttacking = " << combat.isAttacking() << std::endl;
        assert(!combat.isAttacking()); // Rejected! Cannot do a1 after a1a1!

        // Attempted a2 attack after a1a1: MUST ALSO BE REJECTED!
        triggerHeavy(combat, player);
        tick(0.02f, arena, player, enemy, combat);
        std::cout << "Attempted a2 after a1a1: isAttacking = " << combat.isAttacking() << std::endl;
        assert(!combat.isAttacking()); // Rejected! Only adw2 or nothing!

        // Valid finisher: adw2 (holding down + heavy attack) -> executes a1a1adw2 Dual Air Dive Slam!
        player.setSimulatedVerticalDir(1.0f); // Down
        triggerHeavy(combat, player);
        player.setSimulatedVerticalDir(0.0f);
        tick(0.02f, arena, player, enemy, combat);
        std::cout << "Triggered adw2 after a1a1: isAttacking = " << combat.isAttacking() 
                  << ", combo = " << combat.getCurrentComboName() << std::endl;
        assert(combat.isAttacking());
        assert(combat.getCurrentComboName().find("a1-a1-adw2") != std::string::npos);
        assert(player.velocity.y > 1000.0f); // Dive slam active

        // Let player and enemy land on ground
        for (int i = 0; i < 120; ++i) {
            tick(0.016f, arena, player, enemy, combat);
            if (player.isOnGround()) break;
        }
        assert(player.isOnGround());

        // Now test: NO per-jump limit on fresh attacks!
        // When player jumps again, they can freely perform a1 again!
        player.handleInput(evSpace); // Jump again
        tick(0.05f, arena, player, enemy, combat);
        assert(!player.isOnGround());
        enemy.position = {player.position.x + 35.0f, player.position.y};
        triggerLight(combat, player);
        tick(0.02f, arena, player, enemy, combat);
        std::cout << "Fresh jump a1 attack: isAttacking = " << combat.isAttacking() << std::endl;
        assert(combat.isAttacking()); // Fresh a1 allowed! No arbitrary per-jump limits!

        std::cout << "-> PASSED: Air combo allows a1->a1, blocks anything except adw2 afterwards, with no artificial per-jump caps!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 21: PLAYER CONFIG KV LOADING & HOT-RELOAD
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 21] Testing PlayerConfig KV Loading & Hot-Reload..." << std::endl;
        const auto& pcfg = PlayerConfig::get();
        std::cout << "Player stats: maxHp=" << pcfg.maxHp 
                  << ", moveSpeed=" << pcfg.moveSpeed 
                  << ", jumpSpeed=" << pcfg.jumpSpeed 
                  << ", gravity=" << pcfg.gravity 
                  << ", dashSpeed=" << pcfg.dashSpeed 
                  << ", dashIframe=" << pcfg.iframeDuration 
                  << ", dodgeDuration=" << pcfg.spotDodgeDuration << std::endl;

        assert(pcfg.maxHp == 100.0f);
        assert(pcfg.moveSpeed == 400.0f);
        assert(pcfg.jumpSpeed == 900.0f);
        assert(pcfg.dashSpeed == 1250.0f);
        assert(pcfg.dashCooldown == 0.30f);
        assert(pcfg.spotDodgeDuration == 0.22f);
        assert(pcfg.halfW == 20.0f);

        // Test mutating and hot-reloading from player.kv
        PlayerConfig::getMutable().maxHp = 777.0f;
        assert(PlayerConfig::get().maxHp == 777.0f);

        PlayerConfig::reload();
        assert(PlayerConfig::get().maxHp == 100.0f);
        std::cout << "-> PASSED: PlayerConfig correctly loads from assets/config/player.kv and hot-reloads!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 22: ENEMY CONFIG KV LOADING, COMBO CUSTOMIZATION & HOT-RELOAD
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 22] Testing EnemyConfig KV Loading, Combo Customization & Hot-Reload..." << std::endl;
        const auto& ecfg = EnemyConfig::get();
        std::cout << "Enemy stats: maxHp=" << ecfg.stats.maxHp 
                  << ", gravity=" << ecfg.stats.gravity 
                  << ", starter1 windup=" << ecfg.starter1.windup 
                  << ", starter1 d1 damage=" << ecfg.starter1.hitD1.damage 
                  << ", starter1 d1 knockY=" << ecfg.starter1.hitD1.knockY 
                  << ", starter2 2 damage=" << ecfg.starter2.hit2.damage 
                  << ", starter3 sweep knockY=" << ecfg.starter3.hitSweep.knockY 
                  << ", air1 a1 damage=" << ecfg.air1.hitA1.damage 
                  << ", air2 diveSpeedY=" << ecfg.air2.diveSpeedY << std::endl;

        assert(ecfg.stats.maxHp == 100.0f);
        assert(ecfg.stats.gravity == 2600.0f);
        assert(ecfg.starter1.windup == 0.07f);
        assert(ecfg.starter1.hitD1.damage == 5.0f);
        assert(ecfg.starter1.hitD1.knockY == -435.0f);
        assert(ecfg.starter2.hit2.damage == 4.5f);
        assert(ecfg.starter3.dashBehindOffset == 38.0f);
        assert(ecfg.air1.jumpVelY == -880.0f);
        assert(ecfg.air2.diveSpeedY == 1200.0f);

        // Test mutating combo stats and reloading from enemy.kv
        EnemyConfig::getMutable().starter1.hitD1.damage = 999.0f;
        assert(EnemyConfig::get().starter1.hitD1.damage == 999.0f);

        EnemyConfig::reload();
        assert(EnemyConfig::get().starter1.hitD1.damage == 5.0f);
        std::cout << "-> PASSED: EnemyConfig correctly loads combos from assets/config/enemy.kv and hot-reloads!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 23: PLAYER COUNTER STANCE SUCCESS [Q] (Enemy attacks -> Enemy gets LONG STUN)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 23] Testing Player Counter Stance Success on [Q]..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(false);

        // Position enemy close in front of player
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Player activates counter stance with Q
        triggerCounter(player);
        assert(player.isCounterStanceActive());

        // Enemy attacks
        enemy.attackState = EnemyAttackState::ATTACKING;
        enemy.attackStateTimer = 0.16f;
        enemy.attackHasHit = false;
        enemy.currentHbw = 55.0f;
        enemy.currentHbh = 42.0f;
        enemy.currentHbyOffset = -18.0f;

        // Tick simulation so attack hitbox hits player in counter stance
        tick(0.016f, arena, player, enemy, combat);

        std::cout << "After counter: player hp=" << player.hp 
                  << ", isStunned=" << player.isStunned() 
                  << ", enemy hp=" << enemy.hp 
                  << ", enemy hurtTimer=" << enemy.hurtTimer << std::endl;

        // Player took 0 damage because they parried!
        assert(player.hp == player.maxHp);
        assert(!player.isStunned());
        assert(!player.isCounterStanceActive());

        // Enemy received counter damage and LONG STUN (1.50s)
        assert(enemy.hp < enemy.maxHp);
        assert(enemy.hurtTimer >= 1.45f);
        assert(combat.getCurrentComboName().find("COUNTER-STRIKE") != std::string::npos);
        std::cout << "-> PASSED: Player counter stance successfully parried enemy, took 0 damage and inflicted 1.5s long stun on enemy!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 24: PLAYER COUNTER STANCE WHIFF [Q] (Enemy does NOT attack -> Player gets LONG STUN)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 24] Testing Player Counter Stance Whiff on [Q]..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();

        // Place enemy far away
        enemy.position = {player.position.x + 400.0f, player.position.y};

        // Player activates counter stance
        triggerCounter(player);
        assert(player.isCounterStanceActive());

        // Simulate elapsed time past counter window (window is 0.30s, simulate 0.35s)
        for (int i = 0; i < 25; ++i) {
            tick(0.016f, arena, player, enemy, combat);
        }

        std::cout << "After counter stance expired: player isStunned=" << player.isStunned() 
                  << ", stunTimer=" << player.getStunTimer() 
                  << ", isCounterWhiffStun=" << player.isCounterWhiffStun() << std::endl;

        // Counter expired without hit: Player receives LONG STUN!
        assert(!player.isCounterStanceActive());
        assert(player.isStunned());
        assert(player.isCounterWhiffStun());
        assert(player.getStunTimer() > 0.8f);
        std::cout << "-> PASSED: Player counter stance whiff correctly inflicted 1.0s long stun on player!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 25: SMART AI BOT COUNTER STANCE SUCCESS (Player attacks into Bot Stance -> Player gets LONG STUN)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 25] Testing Smart AI Bot Counter Stance Success..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(true);

        // Position player in front of bot
        enemy.position = {player.position.x + 35.0f, player.position.y + (Player::BOTTOM_OFFSET - Enemy::HALF_H)};
        enemy.isFacingLeft = true;

        // Bot activates counter stance
        enemy.counterCooldownTimer = 0.0f;
        enemy.triggerCounterStance();
        assert(enemy.isCounterStanceActive());

        // Player attempts to attack the bot with light strike (1)
        triggerLight(combat, player);
        assert(combat.isAttacking());

        // Tick simulation to resolve hit into bot's counter stance
        tick(0.016f, arena, player, enemy, combat);

        std::cout << "After bot countered player: player hp=" << player.hp 
                  << ", isStunned=" << player.isStunned() 
                  << ", stunTimer=" << player.getStunTimer() 
                  << ", bot hurtTimer=" << enemy.hurtTimer << std::endl;

        // Bot countered player: Player took damage and received LONG STUN (1.40s)!
        assert(player.hp < player.maxHp);
        assert(player.isStunned());
        assert(player.getStunTimer() >= 1.35f);
        // Bot was NOT hurt
        assert(enemy.hurtTimer == 0.0f);
        assert(!enemy.isCounterStanceActive());
        assert(combat.getCurrentComboName().find("BOT COUNTER") != std::string::npos);
        std::cout << "-> PASSED: Bot counter stance successfully countered player, dealing damage and inflicting 1.4s long stun on player!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 26: SMART AI BOT COUNTER STANCE WHIFF (Player does NOT attack -> Bot gets LONG STUN)
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 26] Testing Smart AI Bot Counter Stance Whiff..." << std::endl;
        combat.reset();
        player.reset(arena.getSpawnPosition());
        enemy.reset();
        enemy.setSmartAI(true);

        // Place player far away so player does not attack or hit bot
        player.position = {enemy.position.x - 500.0f, enemy.position.y};

        // Bot activates counter stance
        enemy.counterCooldownTimer = 0.0f;
        enemy.triggerCounterStance();
        assert(enemy.isCounterStanceActive());

        // Simulate elapsed time past bot's counter window (window is 0.35s, simulate 0.40s)
        for (int i = 0; i < 30; ++i) {
            tick(0.016f, arena, player, enemy, combat);
        }

        std::cout << "After bot counter stance expired: bot isCounterStanceActive=" << enemy.isCounterStanceActive() 
                  << ", bot hurtTimer=" << enemy.hurtTimer << std::endl;

        // Bot counter expired without hit: Bot receives LONG STUN (1.20s)!
        assert(!enemy.isCounterStanceActive());
        assert(enemy.hurtTimer > 1.0f);
        assert(enemy.getCurrentComboName().find("WHIFFED") != std::string::npos);
        std::cout << "-> PASSED: Bot counter stance whiff correctly inflicted 1.2s long stun on bot!" << std::endl;
    }

    // -------------------------------------------------------------
    // TEST 27: COUNTER CONFIGURATION KV LOADING & HOT-RELOAD
    // -------------------------------------------------------------
    {
        std::cout << "\n[TEST 27] Testing Counter Configuration KV Loading & Hot-Reload..." << std::endl;
        const auto& pcfg = PlayerConfig::get();
        const auto& ecfg = EnemyConfig::get();

        std::cout << "Player counter: window=" << pcfg.counterWindow 
                  << ", whiffStun=" << pcfg.counterWhiffStun 
                  << ", enemyStun=" << pcfg.counterEnemyStun 
                  << ", damage=" << pcfg.counterDamage << std::endl;

        std::cout << "Enemy counter: window=" << ecfg.counter.window 
                  << ", whiffStun=" << ecfg.counter.whiffStun 
                  << ", playerStun=" << ecfg.counter.playerStun 
                  << ", damage=" << ecfg.counter.damage << std::endl;

        assert(pcfg.counterWindow == 0.30f);
        assert(pcfg.counterWhiffStun == 1.00f);
        assert(pcfg.counterEnemyStun == 1.50f);
        assert(pcfg.counterDamage == 6.0f);

        assert(ecfg.counter.window == 0.35f);
        assert(ecfg.counter.whiffStun == 1.20f);
        assert(ecfg.counter.playerStun == 1.40f);
        assert(ecfg.counter.damage == 7.0f);

        // Test mutating and hot-reload from player.kv and enemy.kv
        PlayerConfig::getMutable().counterEnemyStun = 999.0f;
        EnemyConfig::getMutable().counter.whiffStun = 888.0f;
        assert(PlayerConfig::get().counterEnemyStun == 999.0f);
        assert(EnemyConfig::get().counter.whiffStun == 888.0f);

        PlayerConfig::reload();
        EnemyConfig::reload();
        assert(PlayerConfig::get().counterEnemyStun == 1.50f);
        assert(EnemyConfig::get().counter.whiffStun == 1.20f);

        std::cout << "-> PASSED: Counter parameters correctly loaded from player.kv / enemy.kv and hot-reloaded!" << std::endl;
    }

    std::cout << "\n======================================================" << std::endl;
    std::cout << "ALL DIRECTIONAL COMBOS & BALANCING TESTS PASSED 100%!" << std::endl;
    std::cout << "======================================================" << std::endl;
    return 0;
}
