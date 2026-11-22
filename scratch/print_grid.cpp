
#define private public
#include "EditorApp.h"
#include <iostream>

int main() {
    EditorApp app;
    for (size_t s = 0; s < app._autoTileSets.size(); ++s) {
        const auto& set = app._autoTileSets[s];
        std::cout << "AutoTileSet: stem=" << set.stem << ", master=" << set.masterCode << std::endl;
        for (int r = 0; r < set.rows; ++r) {
            std::cout << "  row " << (r+1) << ": ";
            for (int c = 0; c < set.cols; ++c) {
                std::cout << set.gridCodes[r][c] << " ";
            }
            std::cout << std::endl;
        }
    }
    for (size_t s = 0; s < app._autoStairSets.size(); ++s) {
        const auto& ass = app._autoStairSets[s];
        std::cout << "AutoStairSet: stem=" << ass.stem
                  << ", upRightSlope=" << ass.upRightSlopeCode
                  << ", downRightSlope=" << ass.downRightSlopeCode
                  << ", upRightBase=" << ass.upRightBaseCode
                  << ", downRightBase=" << ass.downRightBaseCode << std::endl;
    }
    return 0;
}
