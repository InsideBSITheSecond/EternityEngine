#pragma once

#include "eve_game_object.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"

#include <boost/thread/thread.hpp>
#include <boost/range/join.hpp>
#include "eve_model.hpp"

namespace eve {
	static constexpr int MAX_RESOLUTION = 1;
	static constexpr int CHUNK_SIZE = 16;
	static constexpr int MAX_THREADS = 16;

	static const std::vector<glm::vec3> RED = {glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0)};
	static const std::vector<glm::vec3> GREEN = {glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)};
	static const std::vector<glm::vec3> BLUE = {glm::vec3(0, 0, 1), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1)};
	static const std::vector<glm::vec3> MARK = {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 0)};

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

			int si = -1; // qol variable, unused
			int width;
			glm::vec3 position;

			bool isAllSame = false;
			bool isLeaf = false;

			bool marked = false;

			Chunk *container;
			Octant *parent;

			Octant(glm::vec3 position, int w, Chunk *containerChunk, Octant *parentOctant);
			
			Octant *getChild(int index) { return octants[index]; }
			void isContained();

			Octant* transposePathingFromContainerInvDir(Chunk *container, int direction);

			Octant* findTopNeighborFromTop();
			std::vector<Octant *> getAllBotSideSubOctants();

			std::vector<Octant *> getNeighborsTop();
			bool isTopExposed();

			void noiseOctant();

			glm::vec3 getChildLocalOffset();
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

			EveModel::Builder chunkBuilder{};
			std::shared_ptr<EveModel> chunkModel;
			std::shared_ptr<EveGameObject> chunkObject;


			Chunk(Octant *r, glm::vec3 pos, EveTerrain *terrain): root{r}, position{pos}, eveTerrain{terrain} {
				for (int i = 0; i < 6; i++)
					neighbors[i] = nullptr;
			};

			~Chunk() {}

			void remesh(Octant *octant);

			void createFace(Octant *octant, std::vector<glm::vec3> colors);
			void remesh2(Octant *octant);

			void noise();
			EveTerrain *eveTerrain;
		private:
	};
}