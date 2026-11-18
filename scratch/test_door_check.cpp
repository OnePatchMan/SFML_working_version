#include "world/Arena.h"
#include <iostream>

int main() {
    Arena arena;
    arena.loadRoomByName("room_1");
    std::cout << "room_1 doors count: " << arena.getDoors().size() << std::endl;
    for (const auto& d : arena.getDoors()) {
        std::cout << "Door at (" << d.x << ", " << d.y << ") -> targetRoomName: '" << d.targetRoomName << "', targetRoom: " << d.targetRoom
                  << ", targetDoor: (" << d.targetDoorX << ", " << d.targetDoorY << ")" << std::endl;
        std::cout << "isDoor(" << d.x << ", " << d.y << "): " << arena.isDoor(d.x, d.y) << std::endl;
    }

    // Check overlap at door (76, 18)
    float ts = arena.getTileSize();
    float px = (76.0f + 0.5f) * ts;
    float py = (18.0f + 1.0f) * ts - 30.0f;
    auto ov = arena.getOverlappingDoor(px, py, 16.0f, 30.0f, 30.0f);
    if (ov.has_value()) {
        std::cout << "Overlap SUCCESS! targetRoomName: '" << ov->targetRoomName << "', targetDoor: (" << ov->targetDoorX << ", " << ov->targetDoorY << ")" << std::endl;
    } else {
        std::cout << "Overlap FAILED!" << std::endl;
    }

    arena.loadRoomByName("Test");
    std::cout << "Test doors count: " << arena.getDoors().size() << std::endl;
    for (const auto& d : arena.getDoors()) {
        std::cout << "Door at (" << d.x << ", " << d.y << ") -> targetRoomName: '" << d.targetRoomName << "', targetRoom: " << d.targetRoom
                  << ", targetDoor: (" << d.targetDoorX << ", " << d.targetDoorY << ")" << std::endl;
        std::cout << "isDoor(" << d.x << ", " << d.y << "): " << arena.isDoor(d.x, d.y) << std::endl;
    }
    return 0;
}

