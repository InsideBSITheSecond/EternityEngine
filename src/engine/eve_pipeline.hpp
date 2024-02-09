#pragma once

#include "eve_device.hpp"

//std
#include <string>
#include <vector>

namespace eve {

	struct PipelineConfigInfo {
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class EvePipeline {
		public:
			EvePipeline(
				EveDevice& device, 
				const std::string& vertexShaderPath, 
				const std::string& fragShaderPath, 
				const PipelineConfigInfo& configInfo);

			~EvePipeline();

			EvePipeline(const EvePipeline&) = delete;
			EvePipeline &operator=(const EvePipeline&) = delete;

			void bind (VkCommandBuffer commandBuffer);

			static void defaultPipelineConfigInfo(PipelineConfigInfo& configInfo);

		private:
			static std::vector<char> readFile(const std::string& filepath);

			void createGraphicsPipeline(
				EveDevice& device,
				const std::string& vertexShaderPath, 
				const std::string& fragShaderPath,
				const PipelineConfigInfo& configInfo);

			void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

			EveDevice& eveDevice;
			VkPipeline graphicsPipeline;
			VkShaderModule vertShaderModule;
			VkShaderModule fragShaderModule;
	};
}