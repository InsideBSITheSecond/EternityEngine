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

			struct Builder {
				std::vector<Vertex> vertices{};
				std::vector<uint32_t> indices{};
			};
			
			EveModel(EveDevice &device, const EveModel::Builder &builder);
			~EveModel();

			EveModel(const EveModel&) = delete;
			EveModel &operator=(const EveModel&) = delete;

			void bind(VkCommandBuffer commandBuffer);
			void draw(VkCommandBuffer commandBuffer);


		private:
			void createVertexBuffers(const std::vector<Vertex> &vertices);
			void createIndexBuffers(const std::vector<uint32_t> &indices);

			EveDevice &eveDevice;
			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			uint32_t vertexCount;

			bool hasIndexBuffer = false;
			VkBuffer indexBuffer;
			VkDeviceMemory indexBufferMemory;
			uint32_t indexCount;
	};
}