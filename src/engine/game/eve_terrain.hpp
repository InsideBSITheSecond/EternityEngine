#pragma once

#include <glm/gtc/matrix_transform.hpp>

namespace eve {

	struct EveVoxel {
		int id;
	};

	struct Octant {
		EveVoxel *voxel;
		Octant *octants[8];

		glm::vec3 position;

		bool isAllSame = false;
		bool isRoot = false;
		bool isLeaf = false;
	};

	class EveTerrain {
		public:
			EveTerrain();
			~EveTerrain();

			EveTerrain(const EveTerrain&) = delete;
			EveTerrain &operator=(const EveTerrain&) = delete;

			void queryTerrain(glm::vec3 pos);

			Octant *root;
		private:

	};
}