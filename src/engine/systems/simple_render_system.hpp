#pragma once

#include "../eve_window.hpp"

#include "../device/eve_device.hpp"

#include "../rendering/eve_pipeline.hpp"
#include "../rendering/eve_renderer.hpp"

#include "../data/eve_frame_info.hpp"

#include "../game/eve_camera.hpp"
#include "../game/eve_game_object.hpp"

// std
#include <memory>
#include <vector>
#include <easy/profiler.h>

namespace eve {
	class SimpleRenderSystem {
		public:

			SimpleRenderSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout, EveTerrain &terrain);
			~SimpleRenderSystem();

			void switchRenderMode();

			SimpleRenderSystem(const SimpleRenderSystem&) = delete;
			SimpleRenderSystem &operator=(const SimpleRenderSystem&) = delete;

			void update(FrameInfo &frameInfo, GlobalUbo &ubo);
			void renderGameObjects(FrameInfo &frameInfo);

		private:
			void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
			void createPipeline(VkRenderPass renderPass);

			VkPolygonMode currentRenderMode = VK_POLYGON_MODE_FILL;
			VkPolygonMode requestedRenderMode; // initialized from eve_debug.hpp

			EveDevice &eveDevice;
			EveTerrain &eveTerrain;
			std::unique_ptr<EvePipeline> evePipeline;
			std::unique_ptr<EvePipeline> inactivePipeline;
			VkPipelineLayout pipelineLayout;
	};
}