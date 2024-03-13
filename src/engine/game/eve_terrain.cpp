#include "eve_terrain.hpp"

namespace eve {

	EveTerrain::EveTerrain(EveDevice &device) : eveDevice{device} {
		voxelMap.push_back(new EveVoxel(0, "air", false));
		voxelMap.push_back(new EveVoxel(1, "stone", true));
		init();
	}

	EveTerrain::~EveTerrain() {
		/*remeshingCandidates.clear();
		remeshingProcessing.clear();
		remeshingProcessed.clear();
		chunkMap.clear();*/
	}

	void EveTerrain::init() {
		for (int x = -15; x <= 15; x++) {
			for (int z = -15; z <= 15; z++) {
				Chunk *prevY = nullptr;
				for (int y = -2; y <= 2; y++) {
					chunkCount += 1;
					glm::ivec3 chunkPos = glm::ivec3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE);
					Octant *octant = new Octant(chunkPos, CHUNK_SIZE, nullptr, nullptr);
					octant->voxel = voxelMap[1];
					Chunk *chunk = new Chunk(octant, chunkPos, this);
					octant->container = chunk;

					//chunk->noise(chunk->root);
					chunk->id = chunkCount;
					chunk->isQueued = true;
					chunk->neighbors[0] = prevY;
					//remeshingCandidates.push_back(chunk);
					noisingCandidates.push_back(chunk);
					prevY = chunk;
				}
			}
		}

		//root->position = glm::vec3(-ROOT_SIZE, 0, ROOT_SIZE); //del
	}

	void EveTerrain::onMouseWheel(GLFWwindow *window, double xoffset, double yoffset) {
		playerCurrentLevel += -yoffset;
		std::cout << "level: " << playerCurrentLevel << std::endl;
	}

	void EveTerrain::tick() {
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Terrain Tick");

		if (previousMeshingMode != meshingMode) {
			previousMeshingMode = meshingMode;
			meshingPool.meshingMode = meshingMode;
			for (auto& kv : chunkMap) {
				Chunk *chunk = kv.second;
				remeshingCandidates.push_back(chunk);
			}
			vkDeviceWaitIdle(eveDevice.device());
			chunkMap.clear();
		}

		int passed = 0;
		// Mark processed chunks as available for rendering
		{boost::lock_guard<boost::mutex> lock(mutex);
		for (auto it = remeshingProcessed.begin(); it != remeshingProcessed.end();) {
			Chunk *chunk = *it;

			passed++;
			if (*it) {
				if (chunk->id) {
					if (chunk->chunkBuilder.vertices.size() > 3) {
						chunk->chunkModel = std::make_unique<EveModel>(eveDevice, chunk->chunkBuilder);
						auto object = EveGameObject::createGameObject();
						object.model = chunk->chunkModel;
						object.transform.translation = chunk->position;
						chunk->chunkObjectMap.emplace(object.getId(), std::move(object));
					}
					chunkMap.emplace(chunk->id, chunk);
					chunk->isQueued = false;
					remeshingProcessed.erase(std::find(remeshingProcessed.begin(), remeshingProcessed.end(), *it));
				}
			}
		}}

		// Move remeshing candidates in the processing queue
		for (auto it = remeshingCandidates.begin(); it != remeshingCandidates.end();) {
			Chunk *chunk = *it;
			chunk->isQueued = true;
			chunk->chunkObjectMap.clear();
			remeshingProcessing.push_back(*it);
			meshingPool.pushChunkToRemeshingQueue(*it);
			remeshingCandidates.erase(std::find(remeshingCandidates.begin(), remeshingCandidates.end(), *it));
		}

		for (auto it = noisingProcessed.begin(); it != noisingProcessed.end();) {
			Chunk *chunk = *it;
			if (*it) {
				if (chunk->id) {
					chunk->isQueued = true;
					remeshingCandidates.push_back(*it);
					noisingProcessed.erase(std::find(noisingProcessed.begin(), noisingProcessed.end(), *it));
				}
			}
		}

		for (auto it = noisingCandidates.begin(); it != noisingCandidates.end();) {
			Chunk *chunk = *it;
			if (*it) {
				if (chunk->id) {
					chunk->isQueued = true;
					noisingProcessing.push_back(*it);
					noisingPool.pushChunkToNoisingQueue(*it);
					noisingCandidates.erase(std::find(noisingCandidates.begin(), noisingCandidates.end(), *it));
				}
			}
		}

		if (shouldReset) {
			shouldReset = false;
			remeshingCandidates.clear();
			remeshingProcessing.clear();
			remeshingProcessed.clear();
			chunkMap.clear();
			init();
		}
	}

	/*bool EveTerrain::isFullSolid(Octant *octant) {
		if (octant) {
			if (octant->isAllSame && octant->voxel->id == 0) return false;
			if (octant->isAllSame || octant->isLeaf)
		}
	}*/

	void EveTerrain::reset() {
		shouldReset = true;
	}

	void fillOctantWithVoxel(Octant *node, int depth, EveVoxel *voxel) {
		if (node->isLeaf || node->isAllSame) {
			node->voxel = voxel;
		}
		else {
			for (int i = 0; i < 8; i++) {
				node->octants[i]->voxel = voxel;
			}
		}
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

		/* 
		* (fixme) this cause issues with multi-depth octants & even more with negative coords
		* (the wrong octant gets picked causing a wrong index to return 1/2 of the time for positive queryPoint
		*  & 3/4 of the time for negative queryPoint)
		* maybe moving this to octant class will help getting a better ref point
		* or the nodePosition should belong to another depth
		* idk I'm to drunk to know for sure atm what's going on in there
		*/
		

		int childIndex =  0;
		childIndex += (queryPoint.y > nodePosition.y) ? 4 : 0;
		childIndex += (queryPoint.x > nodePosition.x) ? 2 : 0;
		childIndex += (queryPoint.z > nodePosition.z) ? 1 : 0;
		//std::cout << childIndex;
		return childIndex;
	}
	
	int getOctantIndexFromPos2(Octant *node, glm::ivec3 queryPoint) {
		glm::vec3 topLeftFront = glm::vec3(
			node->position.x + trunc(node->width / 2),
			node->position.y - trunc(node->width / 2),
			node->position.z + trunc(node->width / 2));

		glm::vec3 botRightBack = glm::vec3(
			node->position.x - trunc(node->width / 2),
			node->position.y + trunc(node->width / 2),
			node->position.z - trunc(node->width / 2));

		int index = 0;

		//std::cout << glm::to_string(node->position) << " " << glm::to_string(queryPoint) << std::endl;
		//std::cout << node->width << " " << glm::to_string(topLeftFront) << " " << glm::to_string(botRightBack) << std::endl;
		
		if ((queryPoint.y >= topLeftFront.y && queryPoint.y <= botRightBack.y) || node->width == MAX_RESOLUTION) {
			if ((queryPoint.x <= topLeftFront.x && queryPoint.x >= botRightBack.x) || node->width == MAX_RESOLUTION) {
				if ((queryPoint.z <= topLeftFront.z && queryPoint.z >= botRightBack.z) || node->width == MAX_RESOLUTION) {
					if (queryPoint.y >= node->position.y)
						index |= 4;			
					if (queryPoint.x >= node->position.x)
						index |= 2;
					if (queryPoint.z >= node->position.z)
						index |= 1;
				}else {std::cout << "z prob" << std::endl;}
			}else {std::cout << "x prob" << std::endl;}
		}else {std::cout << "y prob" << std::endl;}

		//std::cout << index << " vs " << getOctantIndexFromPos(node->position, queryPoint) << std::endl; 
		
		return index;
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

	void EveTerrain::changeTerrain(glm::ivec3 pos, EveVoxel *voxel) {
		EASY_FUNCTION(profiler::colors::Magenta);
		//glm::ivec3 lookingFor = glm::vec3(
		//	CHUNK_SIZE * ((pos.x - 1) / (CHUNK_SIZE / 2)), 
		//	CHUNK_SIZE * ((pos.y - 1) / (CHUNK_SIZE / 2)),
		//	CHUNK_SIZE * ((pos.z - 1) / (CHUNK_SIZE / 2)));

		//std::cout << std::endl << "looking for " << glm::to_string(lookingFor) << std::endl;
		//std::cout << "pos request " << glm::to_string(pos) << std::endl;
		for (auto &kv : chunkMap) {
			Chunk *chunk = kv.second;
			glm::vec3 topLeftFront = glm::ivec3(
				chunk->root->position.x + trunc(chunk->root->width / 2),
				chunk->root->position.y - trunc(chunk->root->width / 2),
				chunk->root->position.z + trunc(chunk->root->width / 2));

			glm::vec3 botRightBack = glm::ivec3(
				chunk->root->position.x - trunc(chunk->root->width / 2),
				chunk->root->position.y + trunc(chunk->root->width / 2),
				chunk->root->position.z - trunc(chunk->root->width / 2));

			glm::ivec3 center = glm::ivec3((topLeftFront + botRightBack) / 2);

			if (pos.y >= topLeftFront.y && pos.y <= botRightBack.y - 1) {
				if (pos.x <= topLeftFront.x - 1 && pos.x >= botRightBack.x) {
					if (pos.z <= topLeftFront.z - 1 && pos.z >= botRightBack.z) {
						changeOctantTerrain(chunk->root, pos, voxel);
						return;
					}
				}
			}
		}

		std::cout << "Chunk not found" << std::endl;
	}

	Octant* EveTerrain::changeOctantTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel) {
		EASY_FUNCTION(profiler::colors::Magenta);
		int childIndex = getOctantIndexFromPos2(node, queryPoint);
		Octant *child = node->getChild(childIndex);

		//getOctantIndexFromPos2(node, queryPoint);
		
		// if child doesn't exist yet, we create it
		if (!child) {
			if (node->width != MAX_RESOLUTION) {
				for (int i = 0; i < 8; i++) {
					// this part of code represent a problem because we imply a filling type of voxel and we erase everything on our path
					// (fixme)
					int childWidth = node->width / 2;
					node->octants[i] = new Octant((node->position + glm::vec3(octreeOffsets[i] * childWidth) / 2), childWidth, node->container, node);
				}
			}
		}
		child = node->getChild(childIndex);

		// if isLeaf then we set the voxel
		if (node->isLeaf) {
			//std::cout << std::endl;
			//std::cout << "set voxel at " << glm::to_string(queryPoint) << std::endl;
			//std::cout << queryPoint.x << " ";
			node->voxel = voxel;
			return node;
		}

		// if child exist or just created it, we dive deeper into the octree
		if (child) {
			child = changeOctantTerrain(
				child, 
				queryPoint,
				voxel);
		}

		// we assume here that the requested voxel wasn't == to the current voxel
		// (fixme)
		node->isAllSame = false;
		
		// if we are the root node
		if (node->container->root == node) {
			vkDeviceWaitIdle(eveDevice.device());
			//boost::lock_guard<boost::mutex> lock(mutex);
			//remeshingProcessed.erase(std::find(remeshingProcessed.begin(), remeshingProcessed.end(), node->container));
			node->container->chunkObjectMap.clear();

			//pushIfUnique(&remeshingCandidates, node->container);
		}

		return child;
	}
}