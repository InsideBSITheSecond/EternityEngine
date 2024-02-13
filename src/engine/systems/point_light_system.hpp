#pragma once

#include "../device/eve_device.hpp"
#include "../rendering/eve_pipeline.hpp"
#include "../eve_window.hpp"
#include "../rendering/eve_renderer.hpp"
#include "../data/eve_frame_info.hpp"
#include "../game/eve_game_object.hpp"
#include "../game/eve_camera.hpp"

// std
#include <memory>
#include <vector>

namespace eve {
	class PointLightSystem {
		public:

			PointLightSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
			~PointLightSystem();

			PointLightSystem(const PointLightSystem&) = delete;
			PointLightSystem &operator=(const PointLightSystem&) = delete;

			void update(FrameInfo &frameInfo, GlobalUbo &ubo);
			void render(FrameInfo &frameInfo);
		private:
			void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
			void createPipeline(VkRenderPass renderPass);

			EveDevice &eveDevice;
			std::unique_ptr<EvePipeline> evePipeline;
			VkPipelineLayout pipelineLayout;
	};
}