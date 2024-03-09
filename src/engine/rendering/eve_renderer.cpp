#include "eve_renderer.hpp"

#include <array>
#include <stdexcept>
#include <iostream>

namespace eve {

	EveRenderer::EveRenderer(EveWindow &window, EveDevice &device) : eveWindow{window}, eveDevice{device} {
		recreateSwapChain();
		createCommandBuffers();
	}

	EveRenderer::~EveRenderer() {
		freeCommandBuffers();
	}

	void EveRenderer::recreateSwapChain() {
		auto extent = eveWindow.getExtent();
		while (extent.width == 0 || extent.width == 0) {
			extent = eveWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(eveDevice.device());

		if (eveSwapChain == nullptr) {
			eveSwapChain = std::make_unique<EveSwapChain>(eveDevice, extent);
		} else {
			std::shared_ptr<EveSwapChain> oldSwapChain = std::move(eveSwapChain);
			eveSwapChain = std::make_unique<EveSwapChain>(eveDevice, extent, oldSwapChain);

			if (!oldSwapChain->compareSwapFormats(*eveSwapChain.get())) {
				throw std::runtime_error("Swap chain image (or depth) format has changed");
			}
		}
		//createPipeline();
	}

	void EveRenderer::createCommandBuffers() {
		commandBuffers.resize(EveSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = eveDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(eveDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void EveRenderer::freeCommandBuffers() {
		vkFreeCommandBuffers(eveDevice.device(), eveDevice.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	VkCommandBuffer EveRenderer::beginFrame() {
		//assert(isFrameStarted && "Can't can begin frame while already in progress");
		
		auto result = eveSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("failed to aquire swap chain image");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer");
		}
		return commandBuffer;
	}
	
	void EveRenderer::endFrame() {
		assert(isFrameStarted && "Can't call end frame while frame is not in progress");
		auto commandBuffer = getCurrentCommandBuffer();
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
		auto result = eveSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || eveWindow.wasWindowResized()) {
			recreateSwapChain();
			eveWindow.resetWindowResizedFlag();
			return;
		}
		if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % EveSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void EveRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass while frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin a render pass on a command buffer from a different frame");
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = eveSwapChain->getRenderPass();
		renderPassInfo.framebuffer = eveSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = eveSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{}; //this was std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
		clearValues[1].depthStencil = {1.0f, 0};
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(eveSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(eveSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{{0, 0}, eveSwapChain->getSwapChainExtent()};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}
	
	void EveRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
		assert(isFrameStarted && "Can't call endSwapChainRenderPass while frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end a render pass on a command buffer from a different frame");
		vkCmdEndRenderPass(commandBuffer);
	}
	
}