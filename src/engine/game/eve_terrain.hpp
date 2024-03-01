#pragma once

#include "eve_model.hpp"
#include "eve_game_object.hpp"
#include "eve_chunk.hpp"
#include "../device/eve_device.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

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

			void rebuildTerrainMeshesLine();
			void rebuildTerrainMeshesFill();

			void init();
			void reset();

			void cookOctantMeshTransparentMode(Octant *octant);
			void createNewVoxel(std::string name, bool value);

			EveDevice &eveDevice;

			std::vector<EveVoxel*> voxelMap;

			std::vector<Chunk*> chunkMap;

			std::shared_ptr<EveModel> eveCube = EveModel::createModelFromFile(eveDevice, "models/cube.obj");

			//std::vector<Chunk> refinementCandidates;
			//std::vector<Chunk> refinementProcessed;

			std::vector<Chunk*> remeshingCandidates;
			std::vector<Chunk*> remeshingProcessing;
			std::vector<Chunk*> remeshingProcessed;

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

		private:
			EveThreadPool pool{12};
	};
}