#include "eve_terrain.hpp"
namespace eve {
	EveTerrain::EveTerrain() {
		root = new Octant();
		root->isRoot = true;
		root->position = glm::vec3(0.f);
		for (int i = 0; i < 8; i++) {
			root->octants[i] = new Octant();
			root->octants[i]->isLeaf = true;
			root->octants[i]->voxel = new EveVoxel();
		}
	}

	EveTerrain::~EveTerrain() {

	}

	void EveTerrain::queryTerrain(glm::vec3 pos) {
		// if 250 250 250
		
	}
}