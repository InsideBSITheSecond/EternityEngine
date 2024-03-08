#pragma once

#include "eve_model.hpp"
#include "eve_game_object.hpp"
#include "eve_chunk.hpp"
#include "../device/eve_device.hpp"
#include "../utils/eve_enums.hpp"

#include "../../libs/PerlinNoise/PerlinNoise.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <easy/profiler.h>

#include "../utils/eve_threads.hpp"

namespace eve {
	
	class EveTerrain {
		public:
			EveTerrain(EveDevice &device);
			~EveTerrain();

			void tick();

			EveTerrain(const EveTerrain&) = delete;
			EveTerrain &operator=(const EveTerrain&) = delete;

			Octant queryTerrain(Octant *node, int depth, glm::ivec3 queryPoint);

			void changeTerrain(glm::ivec3 pos, EveVoxel *voxel);
			Octant* changeOctantTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel);

			void pushIfUnique(std::vector<Chunk*> *list, Chunk *chunk);

			void init();
			void reset();

			void createNewVoxel(std::string name, bool value);

			EveDevice &eveDevice;

			std::vector<EveVoxel*> voxelMap;
			unsigned int chunkCount = 0;
			std::map<unsigned int, Chunk*> chunkMap;

			EveTerrainMeshingMode meshingMode = MESHING_CHUNK;

			std::shared_ptr<EveModel> eveCube = EveModel::createModelFromFile(eveDevice, "models/cube.obj");
			std::shared_ptr<EveModel> eveQuad = EveModel::createModelFromFile(eveDevice, "models/quad.obj");

			//std::vector<Chunk> refinementCandidates;
			//std::vector<Chunk> refinementProcessed;

			boost::mutex mutex;
			std::vector<Chunk*> remeshingCandidates;
			std::vector<Chunk*> remeshingProcessing;
			std::vector<Chunk*> remeshingProcessed;

			std::vector<Chunk*> noisingQueue;

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

			bool shouldReset = false;
		private:
			EveThreadPool pool{1};
			EveTerrainMeshingMode previousMeshingMode = MESHING_CHUNK;
	};
}