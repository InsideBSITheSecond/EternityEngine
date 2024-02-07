#pragma once

#include "eve_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>

namespace eve {
	class EveModel {
		public:			
			struct Vertex {
				glm::vec3 position;
				glm::vec3 color;

				static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
				static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
			};
			
			EveModel(EveDevice &device, const std::vector<Vertex> &vertices);
			~EveModel();

			EveModel(const EveModel&) = delete;
			EveModel &operator=(const EveModel&) = delete;

			void bind(VkCommandBuffer commandBuffer);
			void draw(VkCommandBuffer commandBuffer);


		private:
			void createVertexBuffers(const std::vector<Vertex> &vertices);

			EveDevice &eveDevice;
			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			uint32_t vertexCount;
	};
}