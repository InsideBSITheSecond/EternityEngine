#pragma once

#include "../device/eve_device.hpp"
#include "../eve_window.hpp"
#include "eve_swap_chain.hpp"

// std
#include <memory>
#include <vector>
#include <cassert>

namespace eve {
	class EveRenderer {
		public:

			EveRenderer(EveWindow &window, EveDevice &device);
			~EveRenderer();

			EveRenderer(const EveRenderer&) = delete;
			EveRenderer &operator=(const EveRenderer&) = delete;

			VkRenderPass getSwapChainRenderPass() const { return eveSwapChain->getRenderPass(); }
			float getAspectRatio() const { return eveSwapChain->extentAspectRatio(); }
 			bool isFrameInProgress() const { return isFrameStarted; }

			VkCommandBuffer getCurrentCommandBuffer() const {
				assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
				return commandBuffers[currentFrameIndex]; 
			}

			int getFrameIndex() const {
				assert(isFrameStarted && "Cannot get frame index when frame not in progress");
				return currentFrameIndex;
			}

			VkCommandBuffer beginFrame();
			void endFrame();

			void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
			void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

		private:
			void createCommandBuffers();
			void freeCommandBuffers();
			void recreateSwapChain();

			EveWindow& eveWindow;
			EveDevice& eveDevice;
			std::unique_ptr<EveSwapChain> eveSwapChain;
			std::vector<VkCommandBuffer> commandBuffers;

			uint32_t currentImageIndex;
			int currentFrameIndex{0};
			bool isFrameStarted{false};
	};
}