#pragma once

#include "../device/eve_device.hpp"
#include "../utils/eve_buffer.hpp"
#include "../data/eve_frame_info.hpp"
#include "../rendering/eve_renderer.hpp"
#include "../rendering/eve_descriptors.hpp"
#include "eve_game_object.hpp"
#include "eve_terrain.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>
#include <cmath>
#include <easy/profiler.h>

#include <imgui_impl_vulkan.h>
#include <imgui_impl_glfw.h>
#include "../../libs/implot/implot.h"

namespace eve {
	struct FrameInfo;
	class EveDebug {
		public:

			static constexpr int DEBUG_POOL_SIZE = 4;

			struct Vertex {
				glm::vec2 position{};
				glm::vec2 uv{};
				glm::vec4 color{};

				static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
				static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

				bool operator==(const Vertex &other) const {
					return position == other.position && uv == other.uv && color == other.color;
				}
			};
			
			EveDebug(EveWindow &window, EveRenderer &renderer, EveDevice &device, std::unique_ptr<EveDescriptorPool> &pool, EveTerrain &terrain);
			~EveDebug();

			EveDebug(const EveDebug&) = delete;
			EveDebug &operator=(const EveDebug&) = delete;

			void init();

			bool isOpen() const { return open; }

			void update(FrameInfo frameInfo);
			void draw(VkCommandBuffer commandBuffer);

			void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

			VkPolygonMode requestedRenderMode = VK_POLYGON_MODE_FILL;
			bool requestedMeshingMode = false;

		private:
			bool open = false;
			bool showDemo = true;
			bool showPlotDemo = true;
			bool showInfo = true;

			void drawDemo();
			void drawPlotDemo();
			void drawInfo(FrameInfo frameInfo);
			void drawControls();

			EveWindow &eveWindow;
			EveRenderer &eveRenderer;
			EveDevice &eveDevice;
			EveTerrain &eveTerrain;
			std::unique_ptr<EveDescriptorPool> &globalPool;
	};
}