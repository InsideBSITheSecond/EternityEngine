#include "eve_renderer.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../libs/stb/stb_image.h"

#include <array>
#include <stdexcept>
#include <iostream>
#include "../utils/eve_buffer.hpp"

namespace eve {

	EveRenderer::EveRenderer(EveWindow &window, EveDevice &device) : eveWindow{window}, eveDevice{device} {
		recreateSwapChain();
		createCommandBuffers();
		createTextureImages();
		createTextureImageViews();
		createTextureSampler();
	}

	EveRenderer::~EveRenderer() {
		for (auto textureImageView : textureImageViews)
			vkDestroyImageView(eveDevice.device(), textureImageView, nullptr);

		vkDestroySampler(eveDevice.device(), textureSampler, nullptr);

		for (auto textureImage : textureImages)
			vkDestroyImage(eveDevice.device(), textureImage, nullptr);

		for (auto textureImageMemory : textureImageMemories)
			vkFreeMemory(eveDevice.device(), textureImageMemory, nullptr);

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

	void EveRenderer::createTextureImage(char *filename, VkImage *textureImage, VkDeviceMemory *textureImageMemory) {
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		//stbi_uc* pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels) {
			throw std::runtime_error("failed to load texture image!");
		}
	
		EveBuffer stagingBuffer{
			eveDevice,
			imageSize,
			1,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)pixels);
		stbi_image_free(pixels);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = texWidth;
		imageInfo.extent.height = texHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0; // Optional
		/* The samples flag is related to multisampling. 
		This is only relevant for images that will be used as attachments, 
		so stick to one sample. There are some optional flags for images 
		that are related to sparse images. Sparse images are images where 
		only certain regions are actually backed by memory. If you were using 
		a 3D texture for a voxel terrain, for example, then you could use this 
		to avoid allocating memory to store large volumes of "air" values. 
		We won't be using it in this tutorial, so leave it to its default value of 0. 
		*/

		eveDevice.createImageWithInfo(imageInfo, 
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			*textureImage, 
			*textureImageMemory);

		eveDevice.transitionImageLayout(*textureImage, 
			VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		eveDevice.copyBufferToImage(stagingBuffer.getBuffer(), 
			*textureImage, 
			static_cast<uint32_t>(texWidth), 
			static_cast<uint32_t>(texHeight), 1);
	}
	
	void EveRenderer::createTextureImages() {
		std::vector<std::string> fileList{
			"textures/core/blocks/water.png",
			"textures/core/blocks/dirt.png",
			"textures/core/blocks/stone.png"
		};

		for (std::string file : fileList) {
			VkImage textureImage;
			VkDeviceMemory textureImageMemory;

			createTextureImage(file.data(), &textureImage, &textureImageMemory);
			textureImages.push_back(textureImage);
			textureImageMemories.push_back(textureImageMemory);
		}
	}

	void EveRenderer::createTextureImageViews() {
		for (VkImage textureImage : textureImages) {
			VkImageView textureImageView = eveDevice.createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);
			textureImageViews.push_back(textureImageView);
		}
	}

	void EveRenderer::createTextureSampler() {
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = eveDevice.properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(eveDevice.device(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
    	}
	}
}