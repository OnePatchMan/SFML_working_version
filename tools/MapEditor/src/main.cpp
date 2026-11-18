#include "EditorApp.h"
#include <iostream>

int main() {
    try {
        EditorApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "MapEditor Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

