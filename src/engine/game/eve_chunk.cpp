#include "eve_chunk.hpp"
#include "eve_terrain.hpp"

namespace eve {
	EveVoxel::EveVoxel(unsigned int i, std::string n, bool v) : id{i}, name{n}, value{v} {}

	Octant::Octant(glm::vec3 pos, int w, Chunk *containerChunk, Octant *parentOctant) {
		
		container = containerChunk;
		parent = parentOctant;
		
		for (int i = 0; i < 8; i++)
			octants[i] = nullptr;

		if (w == MAX_RESOLUTION) {
			isLeaf = true;
			position = pos;
			width = w;
			return;
		} else {
			isAllSame = true;
			position = pos;
			width = w;
		}
	}

	void Octant::noiseOctant(){
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Octant noise");

		EveTerrain *terrain = container->eveTerrain;

		if (isLeaf) {
			float noise = terrain->perlin.octave2D_01((position.x * 0.01), (position.z * 0.01), 4);
			float terrainHeight = std::lerp(terrain->minHeight, terrain->maxHeight, noise);

			if (terrainHeight > position.y)
				voxel = terrain->voxelMap[0];
			else
				voxel = terrain->voxelMap[1];
		}
		else {
			int childWidth = width / 2;
			for (int i = 0; i < 8; i++) {
				if (!octants[i])
					octants[i] = new Octant((position + glm::vec3(terrain->octreeOffsets[i] * childWidth) / 2), childWidth, container, this);
				octants[i]->noiseOctant();
			}
		}

		if (!isLeaf) {
			EveVoxel *sample = octants[0]->voxel;
			for (int i = 1; i < 8; i++) {
				if (!(octants[i]->voxel == sample)) {
					isAllSame = false;
					return;
				}
			}
			isAllSame = true;
			voxel = sample;
		}
	}

	void Chunk::noise() {
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Chunk noise");

		int childWidth = root->width / 2;
		for (int i = 0; i < 8; i++) {
			if (!root->octants[i])
				root->octants[i] = new Octant((root->position + glm::vec3(eveTerrain->octreeOffsets[i] * childWidth) / 2), childWidth, root->container, root);
			root->octants[i]->noiseOctant();
		}
		EveVoxel *sample = root->octants[0]->voxel;
		for (int i = 1; i < 8; i++) {
			if (!(root->octants[i]->voxel == sample)) {
				root->isAllSame = false;
				return;
			}
		}
		root->isAllSame = true;
		root->voxel = sample;
	}

	void Chunk::remesh(Octant *octant) {
		EASY_FUNCTION(profiler::colors::Green100);
		EASY_BLOCK("Threaded Remesh");

		if (octant) 
		{
			if (octant->isAllSame || octant->isLeaf) {
				if (octant->voxel->id != 0) {
					boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);

					auto cube = EveGameObject::createGameObject();
					cube.model = eveTerrain->eveCube;
					cube.transform.translation = octant->position;
					cube.transform.scale = (glm::vec3(octant->width)) / 2;
					chunkObjectMap.emplace(cube.getId(), std::move(cube));
				}
			} else {
				for (int i = 0; i < 8; i++) {
					if (octant->octants[i])
						remesh(octant->octants[i]);
				}
			}

			if (octant->container->root == octant) {
				boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);
				EveTerrain *eveTerrain = octant->container->eveTerrain;
				eveTerrain->remeshingProcessing.erase(std::find(eveTerrain->remeshingProcessing.begin(), eveTerrain->remeshingProcessing.end(), this));
				eveTerrain->remeshingProcessed.push_back(this);
				std::string pos = glm::to_string(this->position);
				std::cout << "Finished a chunk remeshing" << pos << std::endl;
			}
		}
	}

	void Octant::isContained() {

	}

	std::vector<Octant *> Octant::getNeighborsTop() {
		int selfIndex = 0;
		if (parent) {
			while (parent->octants[selfIndex] != this)
				selfIndex++;
		
			if (selfIndex <= 3) { // top side of the octant
				std::vector<Octant *> neighbors;
				Octant *neighbor = parent->octants[selfIndex + 4]; 
				
				// smaller than us
				if (!neighbor->isAllSame) {
					for (int i = 4; i <= 7; i++) {
						if (neighbor->octants[i]) {
							if (!neighbor->octants[i]->isAllSame) {
								for (int j = 4; j <= 7; j++) {
									if (neighbor->octants[i]->octants[j]) {
										neighbors.push_back(neighbor->octants[i]->octants[j]);
									}
								}
								//return (neighbors);
							}
							else {
								neighbors.push_back(neighbor->octants[i]);
							}
						}
					}
					return neighbors;
				}
				
				// same size than us
				if (neighbor->isAllSame) 
					return std::vector<Octant *> {neighbor};
				
				// bigger than us
				if (parent->octants[selfIndex + 4]) 
					return std::vector<Octant *> {parent->octants[selfIndex + 4]};
					
			}
			else if (selfIndex >= 4) { // bot side of the octant
				std::vector<Octant *> neighbors; 
				Octant *neighbor = parent->octants[selfIndex - 4];
				
				// smaller than us
				if (!neighbor->isAllSame) {
					for (int i = 0; i <= 3; i++) {
						if (neighbor->octants[i]) {
							if (!neighbor->octants[i]->isAllSame) {
								for (int j = 0; j <= 3; j++) {
									if (neighbor->octants[i]->octants[j]) {
										neighbors.push_back(neighbor->octants[i]->octants[j]);
									}
								}
								return (neighbors);
							}
							else {
								neighbors.push_back(neighbor->octants[i]);
							}
						}
					}
					return neighbors;
				}

				// same size than us
				if (neighbor->isAllSame) 
					return std::vector<Octant *> {neighbor};
			}
		}
		return std::vector<Octant *> {};
	}

	bool Octant::isTopExposed() {
		int selfIndex = 0;
		if (parent)
			while (parent->octants[selfIndex] != this)
				selfIndex++;
		
		if (selfIndex >= 4) {
			if (parent->octants[7 - selfIndex]->voxel == container->eveTerrain->voxelMap[0]) {
				return true;}
			else {
				return false;}
		}
		return false;
	}

	void Chunk::remesh2(Octant *octant) {
		if (octant->isAllSame || octant->isLeaf) {
			std::vector<Octant *> neighbors = octant->getNeighborsTop();
			std::cout << neighbors.size();
			if (neighbors.size() == 1) { // same size
				if (neighbors.front()->voxel == eveTerrain->voxelMap[0] &&
					octant->voxel != eveTerrain->voxelMap[0]) {

					boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);

					auto quad = EveGameObject::createGameObject();
					quad.model = eveTerrain->eveQuad;
					quad.transform.translation = octant->position;
					quad.transform.translation.y -= octant->width;
					quad.transform.scale = (glm::vec3(octant->width)) / 2;
					chunkObjectMap.emplace(quad.getId(), std::move(quad));
				}
			}
			else if (neighbors.size() == 0) { // top or down level
				if (octant->voxel != eveTerrain->voxelMap[0]) {
					if (octant->container->neighbors[0]) {
						if (octant->container->neighbors[0]->root->voxel == eveTerrain->voxelMap[0]) {
							boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);

							auto quad = EveGameObject::createGameObject();
							quad.model = eveTerrain->eveQuad;
							quad.transform.translation = octant->position;
							quad.transform.translation.y -= octant->width;
							quad.transform.scale = (glm::vec3(octant->width)) / 2;
							chunkObjectMap.emplace(quad.getId(), std::move(quad));
						}
					} else {
						boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);

						auto quad = EveGameObject::createGameObject();
						quad.model = eveTerrain->eveQuad;
						quad.transform.translation = octant->position;
						quad.transform.translation.y -= octant->width;
						quad.transform.scale = (glm::vec3(octant->width)) / 2;
						chunkObjectMap.emplace(quad.getId(), std::move(quad));
					}
				}
			}
			else if (neighbors.size() > 1) { 
				bool allSolid = true;
				for (Octant* oct : neighbors) {
					if (oct->voxel == eveTerrain->voxelMap[0]) {
						allSolid = false;
					}
				}
				if (!allSolid) {
					if (octant->voxel != eveTerrain->voxelMap[0]) {
						boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);

						auto quad = EveGameObject::createGameObject();
						quad.model = eveTerrain->eveQuad;
						quad.transform.translation = octant->position;
						quad.transform.translation.y -= octant->width;
						quad.transform.scale = (glm::vec3(octant->width)) / 2;
						chunkObjectMap.emplace(quad.getId(), std::move(quad));
					}
				}
			}
		} else {
			for (Octant *oct : octant->octants) {
				remesh2(oct);
			}
		}

		if (octant->container->root == octant) {
			boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);
			
			EveTerrain *eveTerrain = octant->container->eveTerrain;
			eveTerrain->remeshingProcessing.erase(std::find(eveTerrain->remeshingProcessing.begin(), eveTerrain->remeshingProcessing.end(), this));
			eveTerrain->remeshingProcessed.push_back(this);
			std::string pos = glm::to_string(this->position);
			std::cout << "Finished a chunk remeshing" << pos << std::endl;
		}
	}
}