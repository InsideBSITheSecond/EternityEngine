#include "engine.hpp"
#include "engine/device/eve_keyboard.hpp"
#include "engine/utils/eve_buffer.hpp"
#include "engine/systems/base_render_system.hpp"
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

namespace eve
{

	App::App()
	{
		EASY_FUNCTION(profiler::colors::Green200);
		EASY_BLOCK("App()");
		globalPool = EveDescriptorPool::Builder(eveDevice)
			.setMaxSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT * 2)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, EveSwapChain::MAX_FRAMES_IN_FLIGHT)			// ubo
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EveDebug::DEBUG_POOL_SIZE)			// debug
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, EveSwapChain::MAX_FRAMES_IN_FLIGHT) // textures
			.setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
			.build();
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
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)			// ubo
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)	// debug
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 32) // textures (max 32 in array)
			.build({
				0, 
				0, 
				VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT
			});

		std::vector<VkDescriptorSet> globalDescriptorSets(EveSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++) {
			auto bufferInfo = uboBuffers[i]->descriptorInfo();

			std::vector<VkDescriptorImageInfo> imageInfos{};
			

			for (int i = 0; i < eveRenderer.textureImageViews.size(); i++) {
				VkDescriptorImageInfo imageInfo;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = eveRenderer.textureImageViews[i];
				imageInfo.sampler = eveRenderer.textureSampler;
				imageInfos.push_back(imageInfo);
			}

			EveDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(2, imageInfos)
				.build(globalDescriptorSets[i]);
		}

		BaseRenderSystem simpleRenderSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout(), eveWorld.eveTerrain};
		PointLightSystem pointLightSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};
		ImGuiSystem imGuiSystem{eveDevice, eveRenderer.getSwapChainRenderPass(), globalSetLayout->getDescriptorSetLayout()};

		auto currentTime = std::chrono::high_resolution_clock::now();

		eveWorld.init();

		while (!eveWindow.shouldClose())
		{
			EASY_BLOCK("App Loop");
			glfwPollEvents();


			auto newTime = std::chrono::high_resolution_clock::now();
			float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
			currentTime = newTime;

			eveWorld.tick(frameTime);

			if (auto commandBuffer = eveRenderer.beginFrame())
			{
				EASY_BLOCK("Command Buffer");

				int frameIndex = eveRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					eveWorld.camera,
					globalDescriptorSets[frameIndex],
					eveWorld.gameObjects,
					eveWorld.debugMenu,
					eveWorld.eveTerrain};

				// update
				EASY_BLOCK("Update");
				GlobalUbo ubo{};
				ubo.projectionMatrix = eveWorld.camera.getProjection();
				ubo.viewMatrix = eveWorld.camera.getView();
				ubo.inverseViewMatrix = eveWorld.camera.getInverseView();
				simpleRenderSystem.update(frameInfo, ubo);
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
}