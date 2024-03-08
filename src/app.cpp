#include "app.hpp"
#include "engine/device/eve_keyboard.hpp"
#include "engine/utils/eve_buffer.hpp"
#include "engine/systems/simple_render_system.hpp"
#include "engine/systems/point_light_system.hpp"
#include "engine/systems/imgui_system.hpp"
#include "engine/game/eve_camera.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


#include <array>
#include <stdexcept>
#include <iostream>
#include <chrono>
#include <easy/profiler.h>

#include "libs/imgui/imgui_impl_vulkan.h"

namespace eve
{

	App::App()
	{
		EASY_FUNCTION(profiler::colors::Green200);
		EASY_BLOCK("App()");
		globalPool = EveDescriptorPool::Builder(eveDevice)
			.setMaxSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EveSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EveDebug::DEBUG_POOL_SIZE)
			.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
			.build();
		loadGameObjects();
	}

	App::~App()
	{
	}

	void App::run()
	{
		std::vector<std::unique_ptr<EveBuffer>> uboBuffers(EveSwapChain::MAX_FRAMES_IN_FLIGHT);
		
		eveWindow.setMouseWheelCallback(std::bind(&EveTerrain::onMouseWheel, &eveTerrain, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		eveWindow.setKeyboardCallback(std::bind(&EveDebug::key_callback, &debugMenu, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));

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
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			EveDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(), eveTerrain};
		PointLightSystem pointLightSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
		ImGuiSystem imGuiSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

		debugMenu.init();

		EveCamera camera{};
		camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));
		

		auto viewerObject = EveGameObject::createGameObject();
		EveKeyboardController keyboardController{};

		auto currentTime = std::chrono::high_resolution_clock::now();

		while (!eveWindow.shouldClose())
		{
			EASY_BLOCK("App Loop");
			glfwPollEvents();

			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			keyboardController.moveInPlaneXZ(eveWindow.getGLFWwindow(), frameTime, viewerObject);
			camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

			float aspect = eveRenderer.getAspectRatio();
			camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);

			/*if (eveTerrain.needRebuild) {
				eveTerrain.needRebuild = false;
				eveTerrain.rebuildTerrainMeshThreaded();
			}*/

			eveTerrain.tick();

			if (auto commandBuffer = eveRenderer.beginFrame())
			{
				EASY_BLOCK("Command Buffer");

				int frameIndex = eveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects,
					debugMenu,
					eveTerrain};

				// update
				EASY_BLOCK("Update");
				GlobalUbo ubo{};
				ubo.projectionMatrix = camera.getProjection();
				ubo.viewMatrix = camera.getView();
				ubo.inverseViewMatrix = camera.getInverseView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();
				EASY_END_BLOCK;


				// render
				EASY_BLOCK("Render");
				//boost::lock_guard<boost::mutex> lock(eveTerrain.mutex);
				eveRenderer.beginSwapChainRenderPass(commandBuffer);

				// order here matters
				EASY_BLOCK("Simple render system");
				simpleRenderSystem.renderGameObjects(frameInfo);
				EASY_END_BLOCK;

				EASY_BLOCK("Point light system");
				pointLightSystem.render(frameInfo);
				EASY_END_BLOCK;

				EASY_BLOCK("IMGUI system");
				imGuiSystem.render(frameInfo);
				EASY_END_BLOCK;
				
				eveRenderer.endSwapChainRenderPass(commandBuffer);
				eveRenderer.endFrame();
				//boost::lock_guard<boost::mutex> unlock(eveTerrain.mutex);
				EASY_END_BLOCK;
			}
			EASY_END_BLOCK;
		}

		vkDeviceWaitIdle(eveDevice.device());
	}

	void App::loadGameObjects()
	{
		std::shared_ptr<EveModel> eveModel;
		
		/*eveModel = EveModel::createModelFromFile(eveDevice, "models/smooth_vase.obj");
		auto smoothVase = EveGameObject::createGameObject();
		smoothVase.model = eveModel;
		smoothVase.transform.translation = {-.5f, .5f, 0.f};
		smoothVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		eveModel = EveModel::createModelFromFile(eveDevice, "models/flat_vase.obj");
		auto flatVase = EveGameObject::createGameObject();
		flatVase.model = eveModel;
		flatVase.transform.translation = {.5f, .5f, 0.f};
		flatVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.emplace(flatVase.getId(), std::move(flatVase));

		eveModel = EveModel::createModelFromFile(eveDevice, "models/quad.obj");
		auto floor = EveGameObject::createGameObject();
		floor.model = eveModel;
		floor.transform.translation = {0.f, .5f, 0.f};
		floor.transform.scale = {3.f, 1.f, 3.f};
		gameObjects.emplace(floor.getId(), std::move(floor));*/

		 std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}  //
		};

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = EveGameObject::makePointLight(0.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{0.f, -1.f, 0.f});
			pointLight.transform.translation = glm::vec3(rotateLight *glm::vec4(-1.f, -1.f, -1.f, 1.f));
			pointLight.transform.translation.y = -20;
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}


	}
}