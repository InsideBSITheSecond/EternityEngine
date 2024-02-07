#include "app.hpp"
#include "engine/simple_render_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <iostream>

namespace eve {

	App::App() {
		loadGameObjects();
	}

	App::~App() {
		
	}

	void App::run() {
		SimpleRenderSystem simpleRenderSystem{eveDevice, eveRenderer.getSwapChainRenderPass()};

		while (!eveWindow.shouldClose()) {
			glfwPollEvents();
			
			if (auto commandBuffer = eveRenderer.beginFrame()) {
				eveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects);
				eveRenderer.endSwapChainRenderPass(commandBuffer);
				eveRenderer.endFrame();
			}
		}
		
		vkDeviceWaitIdle(eveDevice.device());
	}

	void App::loadGameObjects() {
		std::vector<EveModel::Vertex> vertices {
			{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
			{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
			{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
		};
		auto eveModel = std::make_shared<EveModel>(eveDevice, vertices);

		auto triangle = EveGameObject::createGameObject();
		triangle.model = eveModel;
		triangle.color = {.1f, .8f, .1f};
		triangle.transform2d.translation.x = .2f;
		triangle.transform2d.scale = {2.f, .5f};
		triangle.transform2d.rotation = .25f * glm::two_pi<float>();

		gameObjects.push_back(std::move(triangle));
	}
}