#include "imgui_system.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

#include <array>
#include <stdexcept>
#include <iostream>
#include <map>

#include "../../libs/imgui/imgui_impl_glfw.h"
#include "../../libs/imgui/imgui_impl_vulkan.h"

namespace eve
{
	struct ImGuiPushConstants
	{
		glm::vec2 uScale{};
		glm::vec2 uTranslate{};
	};

	ImGuiSystem::ImGuiSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout) : eveDevice{device}
	{
		createPipelineLayout(globalSetLayout);
		createPipeline(renderPass);
	}

	ImGuiSystem::~ImGuiSystem()
	{
		vkDestroyPipelineLayout(eveDevice.device(), pipelineLayout, nullptr);
	}

	void ImGuiSystem::createPipelineLayout(VkDescriptorSetLayout globalSetLayout)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(ImGuiPushConstants);

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

	void ImGuiSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		EvePipeline::defaultPipelineConfigInfo(pipelineConfig);
		EvePipeline::enableAlphaBlending(pipelineConfig);
		pipelineConfig.attributeDescriptions = EveDebug::Vertex::getAttributeDescriptions();
		pipelineConfig.bindingDescriptions = EveDebug::Vertex::getBindingDescriptions();;
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		evePipeline = std::make_unique<EvePipeline>(
			eveDevice,
			"shaders/imgui.vert.spv",
			"shaders/imgui.frag.spv",
			pipelineConfig);
	}

	void ImGuiSystem::render(FrameInfo &frameInfo) {
		evePipeline->bind(frameInfo.commandBuffer);

		vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0, 1,
			&frameInfo.globalDescriptorSet,
			0, nullptr);

		ImGuiPushConstants push{};
		push.uScale = glm::vec2(1.f);
		push.uTranslate = glm::vec2(1.f);

		vkCmdPushConstants(
			frameInfo.commandBuffer,
			pipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(ImGuiPushConstants),
			&push
		);

		frameInfo.debugMenu.update(frameInfo.terrain.chunkMap, frameInfo.frameTime, frameInfo.frameIndex);
		frameInfo.debugMenu.draw(frameInfo.commandBuffer);
	}
}