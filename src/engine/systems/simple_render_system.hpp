#pragma once

#include "../device/eve_device.hpp"
#include "../rendering/eve_pipeline.hpp"
#include "../eve_window.hpp"
#include "../rendering/eve_renderer.hpp"
#include "../data/eve_frame_info.hpp"
#include "../game/eve_camera.hpp"
#include "../game/eve_game_object.hpp"

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

			void renderGameObjects(FrameInfo &frameInfo);

		private:
			void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
			void createPipeline(VkRenderPass renderPass);

			EveDevice &eveDevice;
			std::unique_ptr<EvePipeline> evePipeline;
			VkPipelineLayout pipelineLayout;
	};
}