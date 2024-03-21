#pragma once

#include "eve_model.hpp"
#include "eve_game_object.hpp"
#include "eve_chunk.hpp"
#include "eve_physx.hpp"
#include "../device/eve_device.hpp"
#include "../utils/eve_enums.hpp"

#include "../../libs/PerlinNoise/PerlinNoise.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"
#include "glm/gtx/hash.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <easy/profiler.h>

#include "../utils/eve_threads.hpp"

namespace eve {
	class EveWorld;
	class EveDebug;
	class EveTerrain {
		public:
			EveTerrain(EveDevice &device, EvePhysx &physx);
			~EveTerrain();

			void tick(float deltaTime);

			EveTerrain(const EveTerrain&) = delete;
			EveTerrain &operator=(const EveTerrain&) = delete;

			Octant queryTerrain(Octant *node, int depth, glm::ivec3 queryPoint);

			void changeTerrain(glm::ivec3 pos, EveVoxel *voxel);
			Octant* changeOctantTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel);

			void onMouseWheel(GLFWwindow *window, double xoffset, double yoffset);

			void init();

			void reset() { shouldReset_ = true; };
			void remesh() { shouldRemesh_ = true; };

			void createNewVoxel(std::string name, bool value);

			EveDevice &eveDevice;
			EvePhysx &evePhysx;

			std::vector<EveVoxel*> voxelMap;
			unsigned int chunkCount = 0;
			std::map<unsigned int, Chunk*> chunkMap;
			std::map<unsigned int, BodyID*> physxMap;

			EveTerrainMeshingMode meshingMode = MESHING_CHUNK;

			std::shared_ptr<EveModel> eveCube = EveModel::createModelFromFile(eveDevice, "models/cube.obj", glm::vec3(1, 0, 0));
			std::shared_ptr<EveModel> eveQuad = EveModel::createModelFromFile(eveDevice, "models/quad.obj", glm::vec3(1));
			std::shared_ptr<EveModel> eveQuadR = EveModel::createModelFromFile(eveDevice, "models/quad.obj", glm::vec3(1, 0, 0));
			std::shared_ptr<EveModel> eveQuadG = EveModel::createModelFromFile(eveDevice, "models/quad.obj", glm::vec3(0, 1, 0));
			std::shared_ptr<EveModel> eveQuadB = EveModel::createModelFromFile(eveDevice, "models/quad.obj", glm::vec3(0, 0, 1));

			//std::vector<Chunk> refinementCandidates;
			//std::vector<Chunk> refinementProcessed;

			glm::ivec2 xRange = glm::ivec2(-2, 2);
			glm::ivec2 yRange = glm::ivec2(-1, 1);
			glm::ivec2 zRange = glm::ivec2(-2, 2);

			bool sidesToRemesh[6] = {true, true, true, true, true, true};
			boost::mutex mutex;
			std::vector<Chunk*> remeshingCandidates;
			std::vector<Chunk*> remeshingProcessing;
			std::vector<Chunk*> remeshingProcessed;

			std::vector<Chunk*> noisingCandidates;
			std::vector<Chunk*> noisingProcessing;
			std::vector<Chunk*> noisingProcessed;

			//bool needRebuild = false;

			std::vector<glm::ivec3> octreeOffsets = {
				glm::ivec3(-1, -1, -1),	// left		   top		near
				glm::ivec3(-1, -1, 1),	// left		   top		far
				glm::ivec3(1, -1, -1),	// right	   top		near
				glm::ivec3(1, -1, 1),	// right	   top		far
				glm::ivec3(-1, 1, -1),	// left		   bot		near
				glm::ivec3(-1, 1, 1),	// left		   bot		far
				glm::ivec3(1, 1, -1),	// right	   bot		near
				glm::ivec3(1, 1, 1)		// right	   bot		far
			};

			siv::PerlinNoise::seed_type seed = 123456u;
			siv::PerlinNoise perlin{seed};

			int seaLevel = 0;
			int maxHeight = -48;
			int minHeight = 48;

			int playerCurrentLevel = 0;

		private:
			EveThreadPool noisingPool{12};
			EveThreadPool meshingPool{12};
			
			bool shouldReset_ = false;
			bool shouldRemesh_ = false;
	};
}