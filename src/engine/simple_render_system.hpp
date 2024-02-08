#pragma once

#include "eve_device.hpp"
#include "eve_pipeline.hpp"
#include "eve_window.hpp"
#include "eve_game_object.hpp"
#include "eve_renderer.hpp"
#include "eve_camera.hpp"
#include "eve_frame_info.hpp"

// std
#include <memory>
#include <vector>

namespace eve {
	class SimpleRenderSystem {
		public:

			SimpleRenderSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
			~SimpleRenderSystem();

			SimpleRenderSystem(const SimpleRenderSystem&) = delete;
			SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

			void renderGameObjects(FrameInfo &frameInfo, std::vector<EveGameObject> &gameObjects);
		private:
			void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
			void createPipeline(VkRenderPass renderPass);

			EveDevice &eveDevice;
			std::unique_ptr<EvePipeline> evePipeline;
			VkPipelineLayout pipelineLayout;
	};
}