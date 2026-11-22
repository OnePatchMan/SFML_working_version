#include <iostream>
#include "world/Arena.h"

int main() {
    Arena arena;
    arena.loadRoom(1);
    std::cout << "tileSize=" << arena.getTileSize() << std::endl;
    return 0;
}
