#include "app.hpp"
#include "engine/simple_render_system.hpp"
#include "engine/eve_camera.hpp"
#include "engine/eve_keyboard.hpp"
#include "engine/eve_buffer.hpp"

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
		globalPool = EveDescriptorPool::Builder(eveDevice)
			.setMaxSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();
		loadGameObjects();
	}

	App::~App()
	{
	}

	void App::run()
	{
		std::vector<std::unique_ptr<EveBuffer>> uboBuffers(EveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++) {
			uboBuffers[i] = std::make_unique<EveBuffer>(
				eveDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			uboBuffers[i]->map();
		}

		auto globalSetLayout = EveDescriptorSetLayout::Builder(eveDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			EveDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
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
				int frameIndex = eveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex]};

				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				eveRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
				eveRenderer.endSwapChainRenderPass(commandBuffer);
				eveRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(eveDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<EveModel> eveModel = EveModel::createModelFromFile(eveDevice, "models/smooth_vase.obj");
		auto smoothVase = EveGameObject::createGameObject();
		smoothVase.model = eveModel;
		smoothVase.transform.translation = {-.5f, .5f, 2.5f};
		smoothVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.push_back(std::move(smoothVase));

		eveModel = EveModel::createModelFromFile(eveDevice, "models/flat_vase.obj");
		auto flatVase = EveGameObject::createGameObject();
		flatVase.model = eveModel;
		flatVase.transform.translation = {.5f, .5f, 2.5f};
		flatVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.push_back(std::move(flatVase));
	}
}