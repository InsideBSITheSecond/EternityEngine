#include <cstdlib>
#include <iostream>

#include "engine/engine.hpp"

int main() {
    EternityVoxelEngine Engine;

    try {
		std::cout << "yeah it is cached now hello ?...\n";
        Engine.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}