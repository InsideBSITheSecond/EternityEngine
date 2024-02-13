#pragma once

#include "../eve_device.hpp"
#include "../rendering/eve_pipeline.hpp"
#include "../eve_window.hpp"
#include "../rendering/eve_renderer.hpp"
#include "../eve_frame_info.hpp"
#include "../game/eve_game_object.hpp"
#include "../game/eve_camera.hpp"

// std
#include <memory>
#include <vector>
#include "../rendering/eve_descriptors.hpp"

namespace eve {
	class ImGuiSystem {
		public:

			ImGuiSystem(EveDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
			~ImGuiSystem();

			ImGuiSystem(const ImGuiSystem&) = delete;
			ImGuiSystem &operator=(const ImGuiSystem&) = delete;

			void render(FrameInfo &frameInfo);

		private:
			void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
			void createPipeline(VkRenderPass renderPass);

			EveDevice &eveDevice;

			std::unique_ptr<EvePipeline> evePipeline;
			VkPipelineLayout pipelineLayout;
	};
}