#include "base_render_system.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "glm/ext.hpp"

#include <array>
#include <stdexcept>
#include <iostream>
#include <string>

namespace eve
{

	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{1.f};
		glm::mat4 normalMatrix{1.f};
	};

	BaseRenderSystem::BaseRenderSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, EveTerrain &terrain) 
	: eveDevice{device}, eveTerrain{terrain}
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	BaseRenderSystem::~BaseRenderSystem()
	{
		vkDestroyPipelineLayout(eveDevice.device(), pipelineLayout, nullptr);
	}

	void BaseRenderSystem::switchRenderMode() {
		if (requestedRenderMode != currentRenderMode) {
			vkDeviceWaitIdle(eveDevice.device());
			evePipeline.swap(inactivePipeline);
			currentRenderMode = requestedRenderMode;
		}
	}

	void BaseRenderSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(eveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout");
		}
	}

	void BaseRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		EvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
		evePipeline = std::make_unique<EvePipeline>(
			eveDevice,
			"shaders/base_shader.vert.spv",
			"shaders/base_shader.frag.spv",
			pipelineConfig);

		pipelineConfig.rasterizationInfo.polygonMode = VK_POLYGON_MODE_LINE;
		inactivePipeline = std::make_unique<EvePipeline>(
			eveDevice,
			"shaders/base_shader.vert.spv",
			"shaders/base_shader.frag.spv",
			pipelineConfig);
	}

	void BaseRenderSystem::update(FrameInfo &frameInfo, GlobalUbo &ubo){
		auto rotateLight = glm::rotate(
			glm::mat4(1.f),
			frameInfo.frameTime,
			{-1.f, -1.f, 1.f});
			
		for (auto& kv: frameInfo.gameObjects) {
			auto& obj = kv.second;
			if (obj.directionalLightComponent == nullptr) continue;

			// update light position
			//obj.transform.rotation = glm::vec3(rotateLight * glm::vec4(obj.transform.rotation, 1.f));
			//std::cout << glm::to_string(obj.transform.rotation) << std::endl;
			// copy lights to ubo
			ubo.directionalLight = glm::vec3(obj.transform.rotation);
			//ubo.directionalLight.color = glm::vec4(obj.color, obj.pointLightComponent->lightIntensity);
		}
	}

	void BaseRenderSystem::renderGameObjects(FrameInfo &frameInfo)
	{
		EASY_FUNCTION(profiler::colors::Blue);
		EASY_BLOCK("renderGameObjects");

		requestedRenderMode = frameInfo.debugMenu.requestedRenderMode;
		switchRenderMode();

		evePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&frameInfo.globalDescriptorSet,
			0, nullptr);

		EASY_BLOCK("gameObjects");
		for (auto& kv : frameInfo.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;

			SimplePushConstantData push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);

			obj.model->bind(frameInfo.commandBuffer);
			obj.model->draw(frameInfo.commandBuffer);
		}
		EASY_END_BLOCK;
		
		//std::cout << std::endl << std::endl;

		EASY_BLOCK("chunkObjects");
		for (auto &kv : frameInfo.terrain.chunkMap) {
			Chunk *chunk = kv.second;
			if (!chunk->isQueued) {
				boost::lock_guard<boost::mutex> lock(chunk->mutex);

				for (auto& kv : chunk->chunkObjectMap) {
					EASY_BLOCK("single cube");
					auto& obj = kv.second;

					if (obj.model) {
						SimplePushConstantData push{};
						push.modelMatrix = obj.transform.mat4();

						// this is placeholder bullshit values to illustrate that the normal matrix sides aren't used
						push.normalMatrix[0].w = 69;
						push.normalMatrix[1].w = 68;
						push.normalMatrix[2].w = 67;
						push.normalMatrix[3].w = 66;
						push.normalMatrix[3].x = 65;
						push.normalMatrix[3].y = 64;
						push.normalMatrix[3].z = 63;

						push.normalMatrix = obj.transform.normalMatrix();
						//std::cout << glm::to_string(push.modelMatrix) << std::endl;
						//std::cout << glm::to_string(push.normalMatrix) << std::endl;
						//std::cout << "-------------------------" << sizeof(float) << std::endl;
						vkCmdPushConstants(
							frameInfo.commandBuffer,
							pipelineLayout,
							VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
							0,
							sizeof(SimplePushConstantData),
							&push);

						obj.model->bind(frameInfo.commandBuffer);
						obj.model->draw(frameInfo.commandBuffer);
					}
					EASY_END_BLOCK;
				}
			}
		}
		EASY_END_BLOCK;
	}
}