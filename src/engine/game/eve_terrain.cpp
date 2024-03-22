#include "eve_terrain.hpp"
#include "../utils/eve_utils.hpp"
#include <utility>

namespace eve {

	EveTerrain::EveTerrain(EveDevice &device, EvePhysx &physx) : eveDevice{device}, evePhysx{physx} {
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
		std::unordered_map<glm::ivec3, Chunk*> generated{};

		maxHeight = floor(float(yRange.x * CHUNK_SIZE) - float(CHUNK_SIZE / 2));
		minHeight = floor(float(yRange.y * CHUNK_SIZE) + float(CHUNK_SIZE / 2));

		Chunk *lastChunk;

		for (int x = xRange.x; x <= xRange.y; x++) {
			for (int y = yRange.x; y <= yRange.y; y++) {
				for (int z = zRange.x; z <= zRange.y; z++) {
					chunkCount += 1;
					glm::ivec3 chunkPos = glm::ivec3(x * CHUNK_SIZE, y * CHUNK_SIZE, z * CHUNK_SIZE);
					Octant *octant = new Octant(chunkPos, CHUNK_SIZE, nullptr, nullptr);
					octant->voxel = voxelMap[1];
					lastChunk = new Chunk(octant, chunkPos, this);
					octant->container = lastChunk;
					lastChunk->id = chunkCount;

					generated.emplace(glm::ivec3(x, y, z), lastChunk);
				}
			}
		}

		for (int x = xRange.x; x <= xRange.y; x++) {
			for (int y = yRange.x; y <= yRange.y; y++) {
				for (int z = zRange.x; z <= zRange.y; z++) {
					glm::ivec3 vec;
					auto s = generated.find(glm::ivec3(x, y, z));
					Chunk *self = s->second;
					
					vec = glm::ivec3(x, y - 1, z);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[0] = n->second;
					}

					vec = glm::ivec3(x, y + 1, z);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[1] = n->second;
					}

					vec = glm::ivec3(x - 1, y, z);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[2] = n->second;
					}

					vec = glm::ivec3(x + 1, y, z);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[3] = n->second;
					}

					vec = glm::ivec3(x, y, z - 1);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[4] = n->second;
					}

					vec = glm::ivec3(x, y, z + 1);
					if (generated.find(vec) != generated.end()) {
						auto n = generated.find(vec);
						self->neighbors[5] = n->second;
					}


					self->isQueued = true;
					noisingCandidates.push_back(self);
				}
			}
		}
	}

	void EveTerrain::onMouseWheel(GLFWwindow *window, double xoffset, double yoffset) {
		playerCurrentLevel += -yoffset;
		std::cout << "level: " << playerCurrentLevel << std::endl;
		vkDeviceWaitIdle(eveDevice.device());
		boost::lock_guard<boost::mutex> lock(mutex);
		for (auto it = chunkMap.begin(); it != chunkMap.end();) {
			Chunk *chunk = it->second;
			if ((chunk->position.y <= playerCurrentLevel + CHUNK_SIZE / 2) &&
			(chunk->position.y >= playerCurrentLevel - CHUNK_SIZE / 2)) {
				std::cout << glm::to_string(chunk->position) << std::endl;
				boost::lock_guard<boost::mutex> lock2(chunk->mutex);
				remeshingCandidates.push_back(chunk);
				chunkMap.erase(it++);
				//it++;
			}
			else {
				it++;
			}
		}
	}

	void EveTerrain::tick(float deltaTime) {
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Terrain Tick");

		int passed = 0;
		// Mark processed chunks as available for rendering
		{
			boost::lock_guard<boost::mutex> lock(mutex);
			auto it = remeshingProcessed.begin();
			for (;it != remeshingProcessed.end();) {
				Chunk *chunk = *it;

				passed++;
				if (chunk) {
					if (chunk->id) {
						if (chunk->chunkBuilder.vertices.size()) {
							/*if (chunk->root->position == glm::vec3(-16, -16, -16)) {
								std::cout << "uwu";
							}*/
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
			}
		}

		// Move remeshing candidates in the processing queue
		{
			boost::lock_guard<boost::mutex> lock(mutex);
			for (auto it = remeshingCandidates.begin(); it != remeshingCandidates.end();) {
				Chunk *chunk = *it;
				remeshingProcessing.push_back(*it);
				meshingPool.pushChunkToRemeshingQueue(*it);
				remeshingCandidates.erase(std::find(remeshingCandidates.begin(), remeshingCandidates.end(), *it));
			}
		}

		{
			boost::lock_guard<boost::mutex> lock(mutex);
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
		}

		for (auto it = noisingCandidates.begin(); it != noisingCandidates.end();) {
			Chunk *chunk = *it;
			if (*it) {
				if (chunk->id) {
					chunk->isQueued = true;
					noisingProcessing.push_back(*it);
					//noisingPool.pushChunkToNoisingQueue(*it);
					meshingPool.pushChunkToNoisingQueue(*it);
					noisingCandidates.erase(std::find(noisingCandidates.begin(), noisingCandidates.end(), *it));
				}
			}
		}

		if (shouldReset_) {
			shouldReset_ = false;
			remeshingCandidates.clear();
			remeshingProcessing.clear();
			remeshingProcessed.clear();
			vkDeviceWaitIdle(eveDevice.device());
			for (auto it : chunkMap)
				it.second->~Chunk();
			chunkMap.clear();
			init();
		}

		if (shouldRemesh_) {
			shouldRemesh_ = false;
			remeshingCandidates.clear();
			remeshingProcessing.clear();
			remeshingProcessed.clear();
			meshingPool.meshingMode = meshingMode;
			for (auto kv : chunkMap) {
				Chunk *chunk = kv.second;
				chunk->isQueued = true;
				remeshingCandidates.push_back(chunk);
			}
		}
	}

	/*bool EveTerrain::isFullSolid(Octant *octant) {
		if (octant) {
			if (octant->isAllSame && octant->voxel->id == 0) return false;
			if (octant->isAllSame || octant->isLeaf)
		}
	}*/

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

	/*int getOctantIndexFromPos(glm::ivec3 nodePosition, glm::ivec3 queryPoint) {
		
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
		

		/*int childIndex =  0;
		childIndex += (queryPoint.y > nodePosition.y) ? 4 : 0;
		childIndex += (queryPoint.x > nodePosition.x) ? 2 : 0;
		childIndex += (queryPoint.z > nodePosition.z) ? 1 : 0;
		//std::cout << childIndex;
		return childIndex;
	}*/

	/*Octant EveTerrain::queryTerrain(Octant *node, int depth, glm::ivec3 queryPoint) {
		if(node->isAllSame)
			return *node;

		int childIndex = getOctantIndexFromPos(node->position, queryPoint);
		Octant *child = node->getChild(childIndex);

		return queryTerrain(
			child, 
			depth + 1,
			queryPoint
		);
		
	}*/

	Chunk *EveTerrain::findContainerChunkAt(glm::ivec3 pos) {
		EASY_FUNCTION(profiler::colors::Magenta);
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
						//changeOctantTerrain(chunk->root, pos, voxel);
						return chunk;
					}
				}
			}
		}

		std::cout << "Chunk not found" << std::endl;
		return nullptr;
	}

	/*Octant* EveTerrain::changeOctantTerrain(Octant *node, glm::ivec3 queryPoint, EveVoxel *voxel) {
		EASY_FUNCTION(profiler::colors::Magenta);
		int childIndex = getOctantIndexFromPos(node, queryPoint);
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
	}*/
}