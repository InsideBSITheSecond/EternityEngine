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

	void Octant::noiseOctant(Octant *octant){
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Octant noise");

		EveTerrain *terrain = octant->container->eveTerrain;

		if (isLeaf) {
			float noise = terrain->perlin.octave2D_01((octant->position.x * 0.01), (octant->position.z * 0.01), 4);
			float terrainHeight = std::lerp(terrain->minHeight, terrain->maxHeight, noise);
			//std::cout << terrainHeight << " ";

			if (terrainHeight > octant->position.y) {
				voxel = terrain->voxelMap[0];
				octant->container->countTracker.x += 1;
			} else {
				voxel = terrain->voxelMap[1];
				octant->container->countTracker.y += 1;
			}
		}
		else {
			int childWidth = octant->width / 2;
			for (int i = 0; i < 8; i++) {
				if (!octant->octants[i])
					octants[i] = new Octant((octant->position + glm::vec3(terrain->octreeOffsets[i] * childWidth) / 2), childWidth, octant->container, octant);
				octant->octants[i]->noiseOctant(octant->octants[i]);
				octant->octants[i]->si = i;
			}
		}

		if (!octant->isLeaf) {
			EveVoxel *sample = octant->octants[0]->voxel;
			for (int i = 1; i < 8; i++) {
				if (!(octant->octants[i]->voxel == sample)) {
					octant->isAllSame = false;
				}
			}
			if (octant->isAllSame)
				octant->voxel = sample;
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

	void Chunk::noise(Octant *octant) {
		boost::lock_guard<boost::mutex> lock(mutex);
		EASY_FUNCTION(profiler::colors::Magenta);
		EASY_BLOCK("Chunk noise");

		int childWidth = root->width / 2;
		for (int i = 0; i < 8; i++) {
			if (!octant->octants[i])
				octant->octants[i] = new Octant((octant->position + glm::vec3(eveTerrain->octreeOffsets[i] * childWidth) / 2), childWidth, octant->container, octant);
			octant->octants[i]->noiseOctant(octant->octants[i]);
			octant->octants[i]->si = i; // this is just for qol
		}
		EveVoxel *sample = octant->octants[0]->voxel;
		for (int i = 1; i < 8; i++) {
			if (!(octant->octants[i]->voxel == sample)) {
				octant->isAllSame = false;
			}
		}

		if (octant->isAllSame)
			octant->voxel = sample;


		EASY_BLOCK("Push Noised Chunk");
		boost::lock_guard<boost::mutex> lock2(eveTerrain->mutex);

		eveTerrain->noisingProcessing.erase(
			std::find(eveTerrain->noisingProcessing.begin(),
			eveTerrain->noisingProcessing.end(),
			octant->container));

		eveTerrain->noisingProcessed.push_back(octant->container);

		std::cout << "Finished a chunk noising" << glm::to_string(octant->container->position) << " " << glm::to_string(octant->container->countTracker) << std::endl;
	}

	void Chunk::remesh(Octant *octant) {
		EASY_FUNCTION(profiler::colors::Green100);
		EASY_BLOCK("Threaded Remesh");

		if (octant) 
		{
			if (octant->isAllSame || octant->isLeaf) {
				if (octant->voxel) {
					if (octant->voxel->id != 0) {
						boost::lock_guard<boost::mutex> lock(mutex);

						auto cube = EveGameObject::createGameObject();
						cube.model = eveTerrain->eveCube;
						cube.transform.translation = octant->position;
						cube.transform.scale = (glm::vec3(octant->width)) / 2;
						chunkObjectMap.emplace(cube.getId(), std::move(cube));
					}
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

	Octant *Octant::transposePathingFromContainerInvDir(Chunk *container, int direction) {
		std::vector<int> path {};
		Octant *iterator = this;

		while (iterator->parent) {
			path.insert(path.begin(), iterator->si);
			iterator = iterator->parent;
		}

		if (position == glm::vec3(-39.5, -21.5, -21.5))
			std::cout << "h";

		iterator = container->root;
		for (int index : path) {
			if (!iterator->octants[index]) {
				std::cout << "error";
				return nullptr;
			}
			iterator = iterator->octants[index];
		}
		return iterator;
	}

	Octant *Octant::findNeighborFromEdge(const OctantSide side){
		if (parent) {
			if (std::find(std::begin(side.members), std::end(side.members), parent->si) != std::end(side.members)) { // top side
				Octant *neighbor = parent->findNeighborFromEdge(side);
				if (neighbor) {
					if (neighbor->position.y == (position.y - width / 2) - (neighbor->width / 2)) {
						return neighbor->octants[si + side.direction];
					}
				}
				if (container->neighbors[side.neighborDirection]) {
					return transposePathingFromContainerInvDir(container->neighbors[side.neighborDirection], side.direction); // this may be the issue since we could need a reversed direction
				}
				
			}
			else if (std::find(std::begin(side.members), std::end(side.members), parent->si) == std::end(side.members)) { // bot side
				if (!container->neighbors[side.neighborDirection]) {
					return nullptr;
				}
				
				if (container->neighbors[side.neighborDirection]) {
					return transposePathingFromContainerInvDir(container->neighbors[side.neighborDirection], side.direction); // this may be the issue since we could need a reversed direction
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

	std::vector<Octant *> Octant::getAllSubOctants(const OctantSide side) {
		std::vector<Octant *> list {};

		if (!isAllSame) {
			//for (int i : side.members) {  dirty fix
			for (int i : OctantSides::Down.members) {
				if (octants[i]) {
					if (octants[i]->isLeaf) {
						list.push_back(octants[i]);
					}
					else {
						std::vector<Octant *> subOcts = octants[i]->getAllSubOctants(OctantSides::Down);
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

	std::vector<Octant *> Octant::getNeighbors(const OctantSide side) {
		if (parent) {	
			if (std::find(std::begin(side.members), std::end(side.members), si) != std::end(side.members)) { // top side of the octant
				std::vector<Octant *> neighbors;
				Octant *neighbor = nullptr;

				neighbor = findNeighborFromEdge(side);
				if (!neighbor) { // top chunk don't have neighbors
					return std::vector<Octant *> {};
				}

				// smaller than us
				if (!neighbor->isAllSame) {
					neighbors = neighbor->getAllSubOctants(side);
					if (neighbors.size() > 0) {
						return neighbors;
					}
				}
				
				// same size than us
				if (neighbor->isAllSame || neighbor->isLeaf) 
					return std::vector<Octant *> {neighbor};
				
				// bigger than us
				if (parent->octants[si + side.direction]) 
					return std::vector<Octant *> {parent->octants[si + side.direction]};
					
			}
			else if (std::find(std::begin(side.members), std::end(side.members), si) == std::end(side.members)) { // bot side of the octant
				std::vector<Octant *> neighbors;

				Octant *neighbor = parent->octants[si - side.direction];
				
				// smaller than us
				if (!neighbor->isAllSame) {
					neighbors = neighbor->getAllSubOctants(side);
					/*if (width >= 8) {
						marked = true;
						for (Octant *n : neighbors) {
							n->forceRender = true;
							n->marked = true;
							n->container->remesh2(n);
						}
					}*/
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
				neighbors = neighbor->getAllSubOctants(side);
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

	glm::vec3 rotateV(const OctantSide side, glm::vec3 coord) {
		if (side.direction == 4) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(0.f), glm::vec3(0.0, 0.0, 1.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == -4) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(180.f), glm::vec3(0.0, 0.0, 1.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == 2) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(90.f), glm::vec3(0.0, 0.0, 1.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == -2) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(270.f), glm::vec3(0.0, 0.0, 1.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == 1) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(90.f), glm::vec3(1.0, 0.0, 0.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == -1) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(-90.f), glm::vec3(1.0, 0.0, 0.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		}
		return glm::vec3(0);
	}

	glm::vec3 offsetV(const OctantSide side, glm::vec3 coord, float amount) {
		if (side.direction == 4) {
			return glm::vec3(coord.x, coord.y - amount, coord.z);
		} else if (side.direction == -4) {
			return glm::vec3(coord.x, coord.y + amount, coord.z);
		} else if (side.direction == 2) {
			return glm::vec3(coord.x - amount, coord.y, coord.z);
		} else if (side.direction == -2) {
			return glm::vec3(coord.x + amount, coord.y, coord.z);
		} else if (side.direction == 1) {
			return glm::vec3(coord.x, coord.y, coord.z - amount);
		} else if (side.direction == -1) {
			return glm::vec3(coord.x, coord.y, coord.z + amount);
		}
		return glm::vec3(0);
	}

	void Chunk::createFace(Octant *octant, std::vector<glm::vec3> colors, const OctantSide side) {
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

		if (octant->position == glm::vec3(-16, -16, -16))
			std::cout << "uwu";

		int i = 0;
		for (EveModel::Vertex vertex : quadVertices) {
			vertex.position = rotateV(side, vertex.position);
			vertex.normal = rotateV(side, vertex.normal);
			vertex.position *= float(octant->width) / 2;
			vertex.position += offset;
			vertex.position = offsetV(side, vertex.position, float(octant->width) / 2);
			vertex.color = colors[i++];
			chunkBuilder.vertices.push_back(vertex);
		}

		for (uint32_t index : quadIndices) {
			index += chunkBuilder.vertices.size();
			chunkBuilder.indices.push_back(index);
		}
	}

	void Chunk::remesh2(Octant *octant) {
		EASY_FUNCTION(profiler::colors::Green100);
		EASY_BLOCK("Remesh 2");

		if (octant->position == glm::vec3(-16, -16, -16)) {
			std::cout << "uwu";
		}

		std::vector<OctantSide> sidesToCheck = {OctantSides::Top /*, OctantSides::Left, OctantSides::Far*/};
		if (octant) {
			if (octant->isAllSame || octant->isLeaf || octant->forceRender) {
				EASY_BLOCK("Worth considering for render");

				for (const OctantSide side : sidesToCheck) {
					std::vector<Octant *> neighbors = octant->getNeighbors(side);

					if (neighbors.size() == 1) { // same size
						if ((neighbors.front()->voxel == eveTerrain->voxelMap[0] &&
							octant->voxel != eveTerrain->voxelMap[0]) || octant->forceRender) {
							if (!octant->marked) {
								createFace(octant, BLUE, side);
							}
							else {createFace(octant, MARK, side);}
						}
					}
					else if (neighbors.size() == 0) { // top or down level
						if (octant->voxel != eveTerrain->voxelMap[0] || octant->forceRender) {
							if (octant->container->neighbors[0]) {
								if (octant->container->neighbors[0]->root->voxel == eveTerrain->voxelMap[0]) {
									if (!octant->marked) {
										createFace(octant, GREEN, side);
									}
									else {createFace(octant, MARK, side);}
								}
							} else {
								if (!octant->marked) {
									createFace(octant, GREEN, side);
								}
								else {createFace(octant, MARK, side);}
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
						if (!allSolid || octant->forceRender) {
							if (octant->voxel != eveTerrain->voxelMap[0] || octant->forceRender) {
								if (!octant->marked) {
									createFace(octant, RED, side);
								}
								else {createFace(octant, MARK, side);}
							}
						}
					}
				}
			}
			else
			{
				EASY_BLOCK("Recursion");
				for (Octant *oct : octant->octants) {
					remesh2(oct);
				}
			}

			if (octant->container->root == octant) {
				EASY_BLOCK("Push chunk object");
				boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);
				boost::lock_guard<boost::mutex> lock2(mutex);
				EveTerrain *eveTerrain = octant->container->eveTerrain;

				eveTerrain->remeshingProcessing.erase(
					std::find(eveTerrain->remeshingProcessing.begin(),
					eveTerrain->remeshingProcessing.end(),
					this));

				eveTerrain->remeshingProcessed.push_back(this);
				std::cout << "Finished a chunk remeshing " << glm::to_string(this->position) << " vertices: " << octant->container->chunkBuilder.vertices.size() << std::endl;
			}
		}
	}

	void Chunk::backTrackNeighborTD(Chunk *n) {
		neighbors[1] = n;
	}

	void Chunk::backTrackNeighborLR(Chunk *n) {
		neighbors[3] = n;
	}

	void Chunk::backTrackNeighborNF(Chunk *n) {
		neighbors[5] = n;
	}

	void Chunk::backTrackNeighbors() {
		if (neighbors[0])
			neighbors[0]->backTrackNeighborTD(this);
		
		if (neighbors[2])
			neighbors[2]->backTrackNeighborLR(this);

		if (neighbors[4])
			neighbors[4]->backTrackNeighborNF(this);
	}
}