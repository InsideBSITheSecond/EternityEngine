#pragma once

#include "eve_game_object.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"
#include <glm/gtc/matrix_transform.hpp>

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
	static const std::vector<glm::vec3> WHITE = {glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1), glm::vec3(1, 1, 1)};

	struct OctantSide{
		int direction;
		int neighborDirection;
		int members[4];
	};

	class OctantSides{
		public:
			static constexpr OctantSide Top{4, 0, {0, 1, 2, 3}};
			static constexpr OctantSide Down{-4, 1, {4, 5, 6, 7}};
			static constexpr OctantSide Left{2, 2, {0, 1, 4, 5}};
			static constexpr OctantSide Right{-2, 3, {2, 3, 6, 7}};
			static constexpr OctantSide Near{1, 4, {0, 2, 4, 6}};
			static constexpr OctantSide Far{-1, 5, {1, 3, 5, 7}};


			static const OctantSide reverseSide(const OctantSide side) {
				if (side.direction == Top.direction)
					return Down;
				if (side.direction == Down.direction)
					return Top;
				if (side.direction == Left.direction)
					return Right;
				if (side.direction == Right.direction)
					return Left;
				if (side.direction == Near.direction)
					return Far;
				if (side.direction == Far.direction)
					return Near;
			};
	};

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
			bool forceRender = false;

			Chunk *container;
			Octant *parent;

			Octant(glm::vec3 position, int w, Chunk *containerChunk, Octant *parentOctant);
			
			Octant *getChild(int index) { return octants[index]; }
			EveVoxel *getFirstFoundVoxel(Octant *octant);

			Octant* transposePathingFromContainerInvDir(const OctantSide side);

			Octant* findNeighborFromEdge(const OctantSide side);
			std::vector<Octant *> getAllSubOctants(const OctantSide side);

			std::vector<Octant *> getNeighbors(const OctantSide side);
			bool isTopExposed();

			void noiseOctant(Octant *octant);

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
			bool generated = false;

			glm::ivec2 countTracker = glm::ivec2(0);

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

			void createFace(Octant *octant, std::vector<glm::vec3> colors, const OctantSide side);
			void remesh2rec(Octant *octant, bool rec = true);
			void remesh2(Chunk *chunk);

			void backTrackNeighborTD(Chunk *n);
			void backTrackNeighborLR(Chunk *n);
			void backTrackNeighborNF(Chunk *n);
			void backTrackNeighbors();

			void noise(Octant *octant);
			EveTerrain *eveTerrain;
		private:
	};
}