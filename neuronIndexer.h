#pragma once

#include <iostream>

//vk::Shader MLcomp("ML.comp", VK_SHADER_STAGE_COMPUTE_BIT);

int testIndexer(int nX, int nY, int wX, int wY, int lX, int lY) {
    return (nX * wX) + (wY * nY * (nX + nY)) + (lX + (lY * nY * nX));
}

void indexTester(int(*testIndexer)(int, int, int, int, int, int)) {
    for (int X = 0; X < 3; X++) {
        for (int Y = 0; Y < 3; Y++) {
            for (int x = 0; x < 3; x++) {
                for (int y = 0; y < 3; y++) {
                    std::cout << testIndexer(3, 3, x, X, y, Y) << std::endl;
                }
            }
        }
    }
}