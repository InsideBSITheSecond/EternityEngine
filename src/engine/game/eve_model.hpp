#pragma once

#include "../device/eve_device.hpp"
#include "../utils/eve_buffer.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <vector>
#include <memory>

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