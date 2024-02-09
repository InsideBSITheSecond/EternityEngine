#pragma once

#include "eve_camera.hpp"
#include "eve_game_object.hpp"

#include <vulkan/vulkan.h>

namespace eve {
	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		EveCamera &camera;
		VkDescriptorSet globalDescriptorSet;
		EveGameObject::Map &gameObjects;
	};
}