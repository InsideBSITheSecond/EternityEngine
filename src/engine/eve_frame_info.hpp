#pragma once

#include "eve_camera.hpp"

#include <vulkan/vulkan.h>

namespace eve {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		EveCamera &camera;
		VkDescriptorSet globalDescriptorSet;
	};
}