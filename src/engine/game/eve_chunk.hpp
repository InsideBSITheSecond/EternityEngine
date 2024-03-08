#pragma once

#include "eve_game_object.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

#include <boost/thread/thread.hpp>

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

			/*
			* 0: left	   top		near
			* 1: left	   top		far
			* 2: right	   top		near
			* 3: right	   top		far
			* 4: left	   bot		near
			* 5: left	   bot		far
			* 6: right	   bot		near
			* 7: right	   bot		far
			* */

			int width;
			glm::vec3 position;

			bool isAllSame = false;
			bool isLeaf = false;

			Chunk *container;
			Octant *parent;

			Octant(glm::vec3 position, int w, Chunk *containerChunk, Octant *parentOctant);
			
			Octant *getChild(int index) { return octants[index]; }
			void isContained();

			std::vector<Octant *> getNeighborsTop();
			bool isTopExposed();

			void noiseOctant();
	};

	class EveTerrain;
	class Chunk {
		public:
			Octant *root;
			Chunk *neighbors[6];
			/*
			* 0: top
			* 1: down
			* 2: left
			* 3: right
			* 4: near
			* 5: far
			* */
			unsigned int id;

			bool isQueued = false;
			boost::mutex mutex;

			glm::ivec3 position;
			EveGameObject::Map chunkObjectMap;



			Chunk(Octant *r, glm::vec3 pos, EveTerrain *terrain): root{r}, position{pos}, eveTerrain{terrain} {
				for (int i = 0; i < 6; i++)
					neighbors[i] = nullptr;
			};

			~Chunk() {}

			void remesh(Octant *octant);
			void remesh2(Octant *octant);

			void noise();
			EveTerrain *eveTerrain;
		private:
	};
}