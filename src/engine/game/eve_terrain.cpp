#include "eve_terrain.hpp"

namespace eve {
	EveVoxel::EveVoxel(unsigned int i, std::string n, bool v) : id{i}, name{n}, value{v} {}

	Octant::Octant(EveVoxel *vox, glm::vec3 pos, int w) {
		for (int i = 0; i < 8; i++)
			octants[i] = nullptr;

		if (width == MAX_RESOLUTION) {
			isLeaf = true;
			voxel = vox;
			position = pos;
			width = w;
			return;
		} else {
			isAllSame = true;
			voxel = vox;
			position = pos;
			width = w;
		}
	}

	EveTerrain::EveTerrain(EveDevice &device) : eveDevice{device} {
		voxelMap.push_back(new EveVoxel(0, "air", false));
		voxelMap.push_back(new EveVoxel(1, "dirt", true));
		init();
		rebuildTerrainMesh();
	}

	void EveTerrain::init() {
		root = new Octant(voxelMap[1], glm::ivec3(0), ROOT_SIZE);
		root->isRoot = true;
		//root->position = glm::vec3(-ROOT_SIZE, 0, ROOT_SIZE); //del
	}

	void EveTerrain::reset() {
		init();
		needRebuild = true;
	}

	void EveTerrain::rebuildTerrainMesh() {
		needRebuild = false;
		vkDeviceWaitIdle(eveDevice.device());
		std::cout << "Rebuilding terrain mesh" << std::endl;
		terrainObjects.clear();
		buildOctant(root);
	}

	void EveTerrain::buildOctant(Octant *octant) {
		std::shared_ptr<EveModel> eveCube = EveModel::createModelFromFile(eveDevice, "models/cube.obj");
		if (octant)
			if (octant->isAllSame || octant->isLeaf) {
				auto cube = EveGameObject::createGameObject();
				cube.model = eveCube;
				cube.transform.translation = octant->position;
				cube.transform.scale = (glm::vec3(octant->width) - 0.05f) / 2;
				if (octant->voxel->id == 0) {
					cube.transform.scale = glm::vec3(.2f);
				}
				terrainObjects.emplace(cube.getId(), std::move(cube));
			} else {
				for (int i = 0; i < 8; i++) {
					if (octant->octants[i])
						buildOctant(octant->octants[i]);
				}
			}
	}


	EveTerrain::~EveTerrain() {

	}

	void fillOctant(Octant *node, int depth, EveVoxel *voxel) {
		if (node->isLeaf || node->isAllSame) {
			node->voxel = voxel;
		}
		else {
			for (int i = 0; i < 8; i++) {
				node->octants[i]->voxel = voxel;
			}
		}
	}

	void EveTerrain::extendAndFillRoot(Octant *oldRoot, int oldRootIndex) {
		/*Octant *newroot = new Octant(new EveVoxel(), oldRoot->position, octreeOffsets, oldRoot->width * 2);
		newroot->isRoot = true;
		newroot->width = oldRoot->width * 2;
		//newroot->position = TODO;
		
		for (int i = 0; i < 8; i++) {
			if (i == oldRootIndex) {
				newroot->octants[oldRootIndex] = oldRoot;
			} else {
				//newroot->octants[i] = new Octant(new EveVoxel(), );
			}
		}

		oldRoot->isRoot = false;*/
	}

	int getOctantIndexFromPos(glm::ivec3 nodePosition, glm::ivec3 queryPoint) {
		
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

		int childIndex =  0;
		childIndex += (queryPoint.y > nodePosition.y) ? 4 : 0;
		childIndex += (queryPoint.x > nodePosition.x) ? 2 : 0;
		childIndex += (queryPoint.z > nodePosition.z) ? 1 : 0;
		return childIndex;
	}

	Octant EveTerrain::queryTerrain(Octant *node, int depth, glm::ivec3 queryPoint) {
		if(node->isAllSame)
			return *node;

		int childIndex = getOctantIndexFromPos(node->position, queryPoint);
		Octant *child = node->getChild(childIndex);

		return queryTerrain(
			child, 
			depth + 1,
			queryPoint
		);
		
	}

	Octant* EveTerrain::changeTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel) {
		int childIndex = getOctantIndexFromPos(node->position, queryPoint);

		Octant *child = node->getChild(childIndex);
		if (!child) {
			for (int i = 0; i < 8; i++)
				if (node->width != MAX_RESOLUTION) {
					// this part of code represent a problem because we imply a filling type of voxel and we erase everything on our path
					int childWidth = node->width / 2;
					node->octants[i] = new Octant(voxelMap[1], (node->position + glm::vec3(octreeOffsets[i] * childWidth) / 2), childWidth);}
		}
		child = node->getChild(childIndex);

		if (node->width == MAX_RESOLUTION) {
			node->voxel = voxel;
			return node;}

		if (child)
			child = changeTerrain(
				child, 
				queryPoint,
				voxel);

		node->isAllSame = false;
		
		if (node->isRoot)
			needRebuild = true;

		return child;
	}
}