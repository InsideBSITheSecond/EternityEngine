#pragma once

#include "../game/eve_camera.hpp"
#include "../game/eve_game_object.hpp"
#include "../game/eve_debug.hpp"

#include <vulkan/vulkan.h>
#include "../game/eve_terrain.hpp"

namespace eve {
	#define MAX_LIGHTS 10
	class EveDebug;

	struct PointLight {
		glm::vec4 position{}; // ignore w
		glm::vec4 color{}; // w is intensity
	};

	// UBOs have to be memory aligned.
	// Be sure that every changes to that structure requires double checking of it memory alignment.
	// Hard to troubleshoot bug may appear if it's not done correctly
	struct GlobalUbo {
		glm::mat4 projectionMatrix{1.f};
		glm::mat4 viewMatrix{1.f};
		glm::mat4 inverseViewMatrix{1.f};
		glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f}; // w is intensity
		alignas(16) glm::vec3 directionalLight{1.f};
		alignas(16) PointLight pointLights[MAX_LIGHTS];
		alignas(4) int numLights;
	};

	struct FrameInfo {
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		EveCamera &camera;
		VkDescriptorSet globalDescriptorSet;
		EveGameObject::Map &gameObjects;
		EveDebug &debugMenu;
		EveTerrain &terrain;
	};
}