#pragma once

#include "eve_game_object.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

namespace eve {
	static constexpr int MAX_RESOLUTION = 1;
	static constexpr int CHUNK_SIZE = 16;
	static constexpr int MAX_THREADS = 16;


	class EveVoxel {
		public:
			unsigned int id;
			std::string name;
			bool value;

			EveVoxel(unsigned int i, std::string n, bool v);
	};

	class Chunk;
	class Octant {
		public:
			EveVoxel *voxel;
			Octant *octants[8];

			int width;

			glm::vec3 position;

			bool isAllSame = false;
			//bool isRoot = false;
			bool isLeaf = false;

			Chunk *container;

			Octant(EveVoxel *voxel, glm::vec3 position, int w, Chunk *containerChunk);
			
			Octant *getChild(int index) { return octants[index]; }
	};

	class EveTerrain;
	class Chunk {
		public:
			Octant *root;
			bool needRebuild = false; // this should go away from constructor
			glm::ivec3 position;
			EveGameObject::Map chunkObjectMap;

			Chunk(Octant *r, glm::vec3 pos, EveTerrain *terrain): root{r}, position{pos}, eveTerrain{terrain} {};

			~Chunk() {}

			void remesh(Octant *octant);

		private:
			EveTerrain *eveTerrain;
	};
}