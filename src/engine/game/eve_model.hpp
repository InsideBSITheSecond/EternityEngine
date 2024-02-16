#pragma once

#include "../device/eve_device.hpp"
#include "../utils/eve_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>


/*static std::vector<glm::vec3> cubeVertices = {
	glm::vec3(-1, -1, 1), glm::vec3(1, -1, -1), glm::vec3(1, -1, 1),
	glm::vec3(1, -1, -1), glm::vec3(-1, 1, -1), glm::vec3(1, 1, -1),
	glm::vec3(-1, -1, -1), glm::vec3(-1, 1, 1), glm::vec3(-1, 1, -1),
	glm::vec3(1, 1, 1), glm::vec3(-1, 1, -1), glm::vec3(-1, 1, 1),
	glm::vec3(1, -1, 1), glm::vec3(1, 1, -1), glm::vec3(1, 1, 1),
	glm::vec3(-1, -1, 1), glm::vec3(1, 1, 1), glm::vec3(-1, 1, 1),
	glm::vec3(-1, -1, -1), glm::vec3(-1, -1, -1), glm::vec3(-1, -1, 1),
	glm::vec3(1, 1, -1), glm::vec3(1, -1, -1), glm::vec3(1, -1, 1)};
static std::vector<uint32_t> cubeIndices = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0, 18, 1, 3, 19, 4, 6, 20, 7, 9, 21, 10, 12, 22, 13, 14, 23, 16};*/


namespace eve {
	class EveModel {
		public:			
			struct Vertex {
				glm::vec3 position{};
				glm::vec3 color{};
				glm::vec3 normal{};
				glm::vec2 uv{};

				static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
				static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

				bool operator==(const Vertex &other) const {
					return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
				}
			};

			struct Builder {
				std::vector<Vertex> vertices{};
				std::vector<uint32_t> indices{};

				void loadModel(const std::string &filepath);
			};
			
			EveModel(EveDevice &device, const EveModel::Builder &builder);
			~EveModel();

			EveModel(const EveModel&) = delete;
			EveModel &operator=(const EveModel&) = delete;

			static std::unique_ptr<EveModel> createModelFromFile(EveDevice &device, const std::string &filepath);
			static std::unique_ptr<EveModel> createCubeModel(EveDevice &device);

			void bind(VkCommandBuffer commandBuffer);
			void draw(VkCommandBuffer commandBuffer);


		private:
			void createVertexBuffers(const std::vector<Vertex> &vertices);
			void createIndexBuffers(const std::vector<uint32_t> &indices);

			EveDevice &eveDevice;
			
			std::unique_ptr<EveBuffer> vertexBuffer;
			uint32_t vertexCount;

			bool hasIndexBuffer = false;

			std::unique_ptr<EveBuffer> indexBuffer;
			uint32_t indexCount;
	};
}