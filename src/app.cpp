#include "app.hpp"
#include "engine/simple_render_system.hpp"
#include "engine/eve_camera.hpp"
#include "engine/eve_keyboard.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <iostream>
#include <chrono>

namespace eve
{

	App::App()
	{
		loadGameObjects();
	}

	App::~App()
	{
	}

	void App::run()
	{
		SimpleRenderSystem simpleRenderSystem{eveDevice, eveRenderer.getSwapChainRenderPass()};
		EveCamera camera{};
		camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

		auto viewerObject = EveGameObject::createGameObject();
		EveKeyboardController cameraController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!eveWindow.shouldClose())
		{
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			cameraController.moveInPlaneXZ(eveWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = eveRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
			if (auto commandBuffer = eveRenderer.beginFrame())
			{
				eveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
				eveRenderer.endSwapChainRenderPass(commandBuffer);
				eveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(eveDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<EveModel> eveModel = EveModel::createModelFromFile(eveDevice, "models/smooth_vase.obj");

		auto gameObject = EveGameObject::createGameObject();
		gameObject.model = eveModel;
		gameObject.transform.translation = {.0f, .0f, 2.5f};
		gameObject.transform.scale = glm::vec3(3.f);
		gameObjects.push_back(std::move(gameObject));
	}
}