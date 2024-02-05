#include <cstdlib>
#include <iostream>

#include "engine/vulkan/vulkan.hpp"

void EternityVoxelEngine::run() {
	initWindow();
	initVulkan();
	//listAvailableExtensions();
	mainLoop();
	cleanup();
}

void EternityVoxelEngine::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}

	vkDeviceWaitIdle(device);
}

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