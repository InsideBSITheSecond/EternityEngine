#include "eve_chunk.hpp"
#include "eve_terrain.hpp"

namespace eve {
	EveVoxel::EveVoxel(unsigned int i, std::string n, bool v) : id{i}, name{n}, value{v} {}

	Octant::Octant(glm::vec3 pos, int w, Chunk *containerChunk, Octant *parentOctant) {
		
		container = containerChunk;
		parent = parentOctant;

		// (this block exists just for qol)
		
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
				octants[i]->si = i;
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

	glm::vec3 Octant::getChildLocalOffset() {
		glm::vec3 offset = glm::vec3(0);
		if (parent) {
			offset += parent->getChildLocalOffset() + (glm::vec3(container->eveTerrain->octreeOffsets[si]) * (float(width) / 2));
		}
		if (!parent) {
			return glm::vec3(0);
		}
		return offset;
	}

	void Chunk::noise() {
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Chunk noise");

		int childWidth = root->width / 2;
		for (int i = 0; i < 8; i++) {
			if (!root->octants[i])
				root->octants[i] = new Octant((root->position + glm::vec3(eveTerrain->octreeOffsets[i] * childWidth) / 2), childWidth, root->container, root);
			root->octants[i]->noiseOctant();
			root->octants[i]->si = i; // this is just for qol
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
					boost::lock_guard<boost::mutex> lock(mutex);

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
				boost::lock_guard<boost::mutex> lock(mutex);
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

	Octant *Octant::transposePathingFromContainerInvDir(Chunk *container, int direction) {
		std::vector<int> path {};
		Octant *iterator = this;

		while (iterator->parent) {
			path.insert(path.begin(), iterator->si);
			iterator = iterator->parent;
		}

		iterator = container->root;
		for (int index : path) {
			iterator = iterator->octants[index];
		}
		return iterator;
	}

	Octant *Octant::findTopNeighborFromTop(){
		if (parent) {
			if (parent->si <= 3) { // top side
				Octant *neighbor = parent->findTopNeighborFromTop();
				if (neighbor) {
					if (neighbor->position.y == (position.y - width / 2) - (neighbor->width / 2)) {
						return neighbor->octants[si + 4];
					}
				}
				if (container->neighbors[0]) {
					return transposePathingFromContainerInvDir(container->neighbors[0], 4); // this may be the issue since we could need a reversed direction
				}
				
			}
			else if (parent->si >= 4) { // bot side
				if (parent->parent) {
					return parent->parent->octants[parent->si - 4]->octants[si + 4];
				}
				else {
					if (!container->neighbors[0])
						return nullptr;
					return container->neighbors[0]->root->octants[parent->si + 4]->octants[si + 4];
				}
			}
		}
		/*else {
			if (!container->neighbors[0])
				return nullptr;
			return container->neighbors[0]->root->octants[si + 4];
		}*/
		return nullptr;
	}

	std::vector<Octant *> Octant::getAllBotSideSubOctants() {
		std::vector<Octant *> list {};

		if (!isAllSame) {
			for (int i = 4; i <= 7; i++) {
				if (octants[i]) {
					if (octants[i]->isLeaf) {
						list.push_back(octants[i]);
					}
					else {
						std::vector<Octant *> subOcts = octants[i]->getAllBotSideSubOctants();
						for (Octant *sub : subOcts) {
							list.push_back(sub);
						}
					}
				}
				else {
					list.push_back(this);
					return list;
				}
			}
		}
		else {
			 list.push_back(this);
		}
		return list;
	}

	std::vector<Octant *> Octant::getNeighborsTop() {
		if (parent) {	
			if (si <= 3) { // top side of the octant
				std::vector<Octant *> neighbors;
				Octant *neighbor = nullptr;

				neighbor = findTopNeighborFromTop();
				if (!neighbor) // top chunk don't have neighbors
					return std::vector<Octant *> {};

				// smaller than us
				if (!neighbor->isAllSame) {
					neighbors = neighbor->getAllBotSideSubOctants();
					if (neighbors.size() > 0) {
						return neighbors;
					}
				}
				
				// same size than us
				if (neighbor->isAllSame || neighbor->isLeaf) 
					return std::vector<Octant *> {neighbor};
				
				// bigger than us
				if (parent->octants[si + 4]) 
					return std::vector<Octant *> {parent->octants[si + 4]};
					
			}
			else if (si >= 4) { // bot side of the octant
				std::vector<Octant *> neighbors; 
				Octant *neighbor = parent->octants[si - 4];
				
				// smaller than us
				if (!neighbor->isAllSame) {
					neighbors = neighbor->getAllBotSideSubOctants();
					if (neighbors.size() > 0) {
						return neighbors;
					}
				}

				// same size than us
				if (neighbor->isAllSame || neighbor->isLeaf) 
					return std::vector<Octant *> {neighbor};
			}
		}
		else if (container->neighbors[0]) { // when max size
			std::vector<Octant *> neighbors; 
			Octant *neighbor = container->neighbors[0]->root;

			// smaller than us
			if (!neighbor->isAllSame) {
				neighbors = neighbor->getAllBotSideSubOctants();
				if (neighbors.size() > 0) {
					return neighbors;
				}
			}

			//same size than us
			if (neighbor->isAllSame || neighbor->isLeaf) 
				return std::vector<Octant *> {neighbor};
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

	void Chunk::createFace(Octant *octant, std::vector<glm::vec3> colors) {
		boost::lock_guard<boost::mutex> lock(mutex);

		// old seperated objects method
		/*auto quad = EveGameObject::createGameObject();
		if (color == RED)
			quad.model = eveTerrain->eveQuadR;
		if (color == GREEN)
			quad.model = eveTerrain->eveQuadG;
		if (color == BLUE)
			quad.model = eveTerrain->eveQuadB;
		if (color == MARK)
			quad.model = eveTerrain->eveQuad;
		
		quad.transform.translation = octant->position;
		quad.transform.translation.y -= octant->width;
		quad.transform.scale = (glm::vec3(octant->width)) / 2;
		chunkObjectMap.emplace(quad.getId(), std::move(quad));*/


		// new combined buffer method
		glm::vec3 offset = octant->getChildLocalOffset();
		//offset.y -= octant->width;
		std::vector<EveModel::Vertex> quadVertices = {
			{glm::vec3(-1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0)},
			{glm::vec3(1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 1)},
			{glm::vec3(-1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 1)},
			{glm::vec3(1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0.000000000000000001)},
		};
		std::vector<uint32_t> quadIndices = {0, 1, 2, 1, 0, 3};

		int i = 0;
		for (EveModel::Vertex vertex : quadVertices) {
			vertex.position *= float(octant->width) / 2;
			vertex.position += offset;
			vertex.position.y -= float(octant->width) / 2;
			vertex.color = colors[i++];
			chunkBuilder.vertices.push_back(vertex);
		}

		for (uint32_t index : quadIndices) {
			index += chunkBuilder.vertices.size();
			chunkBuilder.indices.push_back(index);
		}

		
	}

	void Chunk::remesh2(Octant *octant) {
		if (octant->isAllSame || octant->isLeaf) {
			//if (octant->container->position == glm::ivec3(16, 0, 16))
			//	std::cout << "hello";
			std::vector<Octant *> neighbors = octant->getNeighborsTop();
			// if (neighbors.size() == 0){
			// 	std::cout << "";
			// }
			// std::cout << neighbors.size();
			if (neighbors.size() == 1) { // same size
				if (neighbors.front()->voxel == eveTerrain->voxelMap[0] &&
					octant->voxel != eveTerrain->voxelMap[0]) {
					if (!octant->marked) {
						createFace(octant, RED);
					}
					else {createFace(octant, MARK);}
				}
			}
			else if (neighbors.size() == 0) { // top or down level
				if (octant->voxel != eveTerrain->voxelMap[0]) {
					if (octant->container->neighbors[0]) {
						if (octant->container->neighbors[0]->root->voxel == eveTerrain->voxelMap[0]) {
							if (!octant->marked) {
								createFace(octant, MARK);
							}
							else {createFace(octant, MARK);}
						}
					} else {
						if (!octant->marked) {
							createFace(octant, GREEN);
						}
						else {createFace(octant, MARK);}
					}
				}
			}
			else if (neighbors.size() > 1) { 
				if (neighbors.size() > 4) {
					std::cout << "r";
				}
				bool allSolid = true;
				for (Octant* oct : neighbors) {
					if (oct->voxel == eveTerrain->voxelMap[0]) {
						allSolid = false;
					}
				}
				if (!allSolid) {
					if (octant->voxel != eveTerrain->voxelMap[0]) {
						if (!octant->marked) {
							createFace(octant, BLUE);
						}
						else {createFace(octant, MARK);}
					}
				}
			}

			//std::cout << " ";

			/*else if (!octant->container->neighbors[0]) {
				int selfIndex = 0;
				if (octant->parent)
					while (octant->parent->octants[selfIndex] != octant)
						selfIndex++;
				//if (selfIndex <= 3)
				//	createFace(octant);
			}*/
		} else {
			for (Octant *oct : octant->octants) {
				remesh2(oct);
			}
		}

		if (octant->container->root == octant) {
			boost::lock_guard<boost::mutex> lock(mutex);
			EveTerrain *eveTerrain = octant->container->eveTerrain;

			if (chunkBuilder.vertices.size() >= 3) {
				chunkModel = std::make_unique<EveModel>(eveTerrain->eveDevice, chunkBuilder);
			}

			auto object = EveGameObject::createGameObject();
			object.model = chunkModel;
			object.transform.translation = octant->position;

			//object.transform.translation.y -= octant->width;
			//object.transform.scale = glm::vec3(octant->width / 2);

			chunkObjectMap.emplace(object.getId(), std::move(object));
			eveTerrain->remeshingProcessing.erase(std::find(eveTerrain->remeshingProcessing.begin(), eveTerrain->remeshingProcessing.end(), this));
			eveTerrain->remeshingProcessed.push_back(this);
			std::string pos = glm::to_string(this->position);
			std::cout << std::endl << "Finished a chunk remeshing" << pos << std::endl;
		}
	}
}