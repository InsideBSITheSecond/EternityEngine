#pragma once

#include "eve_model.hpp"
#include "eve_game_object.hpp"
#include "../device/eve_device.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

#include <vector>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace eve {

	static constexpr int MAX_RESOLUTION = 1;
	static constexpr int ROOT_SIZE = 16;

	class EveVoxel {
		public:
			unsigned int id;
			std::string name;
			bool value;

			EveVoxel(unsigned int i, std::string n, bool v);
	};

	class Octant {
		public:
			EveVoxel *voxel;
			Octant *octants[8];

			int width;
			int depth;

			glm::vec3 position;

			bool isAllSame = false;
			bool isRoot = false;
			bool isLeaf = false;

			Octant(EveVoxel *voxel, glm::vec3 position, int w);
			
			Octant *getChild(int index) { return octants[index]; }
	};

	class Chunk {
		public:
			
	};

	class EveTerrain {
		public:
			EveTerrain(EveDevice &device);
			~EveTerrain();

			EveTerrain(const EveTerrain&) = delete;
			EveTerrain &operator=(const EveTerrain&) = delete;

			Octant queryTerrain(Octant *node, int depth, glm::ivec3 queryPoint);
			Octant* changeTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel);

			void rebuildTerrainMesh();

			void init();
			void reset();

			void buildOctant(Octant *octant);
			void createNewVoxel(std::string name, bool value);

			EveDevice &eveDevice;

			Octant *root;

			EveGameObject::Map terrainObjects;

			std::vector<EveVoxel*> voxelMap;
			std::map<glm::ivec3, Chunk*> chunkmap;

			std::vector<Chunk> refinementCandidates;
			std::vector<Chunk> refinementProcessed;

			bool needRebuild = false;

			std::vector<glm::ivec3> octreeOffsets = {
				glm::ivec3(-1, -1, -1),	// left		   top		near
				glm::ivec3(-1, -1, 1),	// left		   top		far
				glm::ivec3(1, -1, -1),	// right	   top		near
				glm::ivec3(1, -1, 1),	// right	   top		far
				glm::ivec3(-1, 1, -1),	// left		   bot		near
				glm::ivec3(-1, 1, 1),	// left		   bot		far
				glm::ivec3(1, 1, -1),	// right	   bot		near
				glm::ivec3(1, 1, 1)};	// right	   bot		far
			};
}