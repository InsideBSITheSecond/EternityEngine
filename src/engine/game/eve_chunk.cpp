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

			EveVoxel *sample = octant->octants[0]->voxel;
			octant->isAllSame = true;
			for (int i = 0; i < 8; i++) {
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

		if (position == glm::ivec3(-64, -16, -32) && octant == root) {
			octant->marked = true;
		}

		int childWidth = root->width / 2;
		for (int i = 0; i < 8; i++) {
			if (!octant->octants[i])
				octant->octants[i] = new Octant((octant->position + glm::vec3(eveTerrain->octreeOffsets[i] * childWidth) / 2), childWidth, octant->container, octant);
			octant->octants[i]->noiseOctant(octant->octants[i]);
			octant->octants[i]->si = i; // this is just for qol
		}

		octant->isAllSame = true;
		for (int i = 0; i < 8; i++) {
			if (!octant->octants[i]->isAllSame) {
				octant->isAllSame = false;
			}
		}
		
		if (octant->isAllSame)
			octant->voxel = octant->octants[0]->voxel;


		EASY_BLOCK("Push Noised Chunk");

		eveTerrain->noisingProcessing.erase(
			std::find(eveTerrain->noisingProcessing.begin(),
			eveTerrain->noisingProcessing.end(),
			octant->container));

		boost::lock_guard<boost::mutex> lock2(eveTerrain->mutex);
		eveTerrain->noisingProcessed.push_back(octant->container);

		//std::cout << "Finished a chunk noising" << glm::to_string(octant->container->position) << " " << glm::to_string(octant->container->countTracker) << std::endl;
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
				std::cout << "Finished chunk id:" << id << " remeshing" << pos << std::endl;
			}
		}
	}

	EveVoxel *Octant::getFirstFoundVoxel(Octant *octant) {
		for (int i = 0; i < 7; i++) {
			if (octant->octants[i]) {
				if (octant->octants[i]->voxel) {
					return octant->octants[i]->voxel;
				}
				else {
					return getFirstFoundVoxel(octant->octants[i]);
				}
			}
		}
		return nullptr; // warning supress (fixme)
	}

	Octant *Octant::transposePathingFromContainerInvDir(const OctantSide side) {
		EASY_FUNCTION(profiler::colors::Orange300);
		std::vector<int> path {};
		Octant *iterator = this;

		while (iterator->parent) {
			path.insert(path.begin(), iterator->si);
			iterator = iterator->parent;
		}

		iterator = container->neighbors[side.neighborDirection]->root;
		for (int index : path) {
			if (!iterator->octants[index + side.direction]) {
				std::cout << "error";
				return nullptr;
			}
			iterator = iterator->octants[index + side.direction];
		}
		return iterator;
	}

	Octant *Octant::findNeighborFromEdge(const OctantSide side){
		EASY_FUNCTION(profiler::colors::Orange600);
		if (si == -1) {
			if (container->neighbors[side.neighborDirection]) {
				return transposePathingFromContainerInvDir(side);
			}
			else {
				return nullptr;
			}
		}

		if (std::find(std::begin(side.members), std::end(side.members), si) != std::end(side.members)) { // top side
			if (parent) {
				Octant *iterator = parent->findNeighborFromEdge(side);
				if (iterator) {
					if (iterator->isAllSame) {
						return iterator;
					}
					else {
						return iterator->octants[si + side.direction];
					}
				}
				else {
					return nullptr;
				}
			}
		}
		else if (std::find(std::begin(side.members), std::end(side.members), si) == std::end(side.members)) { // bot side
			return parent->octants[si - side.direction];
		}
		return nullptr;
	}

	std::vector<Octant *> Octant::getAllSubOctants(const OctantSide side) {
		EASY_FUNCTION(profiler::colors::Orange900);
		std::vector<Octant *> list {};
		int i = 0;
		
		if (!isAllSame && !isLeaf) {
			for (int i : side.members) {
				std::vector<Octant *> subOcts = octants[i]->getAllSubOctants(side);
				for (Octant *sub : subOcts) {
					list.push_back(sub);
				}
			}
		}
		else {
			 list.push_back(this);
		}
		return list;
	}

	std::vector<Octant *> Octant::getNeighbors(const OctantSide side) {
		EASY_FUNCTION(profiler::colors::Blue200);
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
					neighbors = neighbor->getAllSubOctants(OctantSides::reverseSide(side));
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
					neighbors = neighbor->getAllSubOctants(OctantSides::reverseSide(side));
					if (neighbors.size() > 0) {
						return neighbors;
					}
				}

				// same size than us
				if (neighbor->isAllSame || neighbor->isLeaf) 
					return std::vector<Octant *> {neighbor};
			}
		}
		else if (container->neighbors[side.neighborDirection]) { // when max size
			std::vector<Octant *> neighbors; 
			Octant *neighbor = container->neighbors[side.neighborDirection]->root;

			// smaller than us
			if (!neighbor->isAllSame) {
				neighbors = neighbor->getAllSubOctants(OctantSides::reverseSide(side));
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
			rotationMat = glm::rotate(rotationMat, glm::radians(270.f), glm::vec3(0.0, 0.0, 1.0));
			return glm::vec3(rotationMat * glm::vec4(coord, 1.0));
		} else if (side.direction == -2) {
			glm::mat4 rotationMat(1);
			rotationMat = glm::rotate(rotationMat, glm::radians(90.f), glm::vec3(0.0, 0.0, 1.0));
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
		EASY_FUNCTION(profiler::colors::Red200);

		glm::vec3 offset = octant->getChildLocalOffset();
		std::vector<EveModel::Vertex> quadVertices = {
			{glm::vec3(-1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0)},
			{glm::vec3(1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 1)},
			{glm::vec3(-1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 1)},
			{glm::vec3(1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0.000000000000000001)},
		};
		std::vector<uint32_t> quadIndices = {0, 1, 2, 1, 0, 3};

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

	void Chunk::remesh2rec(Octant *octant, bool rec) {
		if (!octant->isAllSame) {
			if (rec) {
				for (Octant *oct : octant->octants) {
					if (oct)
						remesh2rec(oct);
				}
				EASY_END_BLOCK;
			}
		}
		

		std::vector<OctantSide> sidesToCheck;

		if (eveTerrain->sidesToRemesh[0])
			sidesToCheck.push_back(OctantSides::Top);
		if (eveTerrain->sidesToRemesh[1])
			sidesToCheck.push_back(OctantSides::Down);
		if (eveTerrain->sidesToRemesh[2])
			sidesToCheck.push_back(OctantSides::Left);
		if (eveTerrain->sidesToRemesh[3])
			sidesToCheck.push_back(OctantSides::Right);
		if (eveTerrain->sidesToRemesh[4])
			sidesToCheck.push_back(OctantSides::Near);
		if (eveTerrain->sidesToRemesh[5])
			sidesToCheck.push_back(OctantSides::Far);

		if (octant->isAllSame || octant->isLeaf || octant->forceRender) {
			EASY_BLOCK("Worth considering for render");

			for (const OctantSide side : sidesToCheck) {

				std::vector<Octant *> neighbors = octant->getNeighbors(side);

				if (neighbors.size() == 1) { // same size
					if ((neighbors.front()->voxel == eveTerrain->voxelMap[0] &&
						octant->voxel != eveTerrain->voxelMap[0]) || octant->forceRender) {
						if (!octant->marked) {
							createFace(octant, WHITE, side);
						}
						else {createFace(octant, MARK, side);}
					}
				}
				/*else if (neighbors.size() == 0) { // top or down level
					if (octant->voxel != eveTerrain->voxelMap[0] || octant->forceRender) {
						if (octant->container->neighbors[side.neighborDirection]) {
							if (octant->container->neighbors[side.neighborDirection]->root->voxel == eveTerrain->voxelMap[0]) {
								if (!octant->marked) {
									createFace(octant, WHITE, side);
								}
								else {createFace(octant, MARK, side);}
							}
						} else {
							//if (!neighbors[side.neighborDirection]) {
								if (!octant->marked) {
									createFace(octant, WHITE, side);
								}
								else {createFace(octant, MARK, side);}
							//}
						}
					}
				}*/
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
								createFace(octant, WHITE, side);
							}
							else {createFace(octant, MARK, side);}
						}
					}
				}
			}
		}
	}

	void Chunk::remesh2(Chunk *chunk) {
		EASY_BLOCK("Remesh V2");
		EASY_FUNCTION(profiler::colors::Blue100);
		remesh2rec(chunk->root);

		EASY_BLOCK("Push chunk object");
		boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);
		boost::lock_guard<boost::mutex> lock2(mutex);
		EveTerrain *eveTerrain = chunk->eveTerrain;

		eveTerrain->remeshingProcessing.erase(
			std::find(eveTerrain->remeshingProcessing.begin(),
			eveTerrain->remeshingProcessing.end(),
			this));
		
		eveTerrain->remeshingProcessed.push_back(this);
		/*std::cout << "Finished chunk id:" << id 
			<< " remeshing " << glm::to_string(this->position) 
			<< " vertices: " << chunkBuilder.vertices.size() 
			<< std::endl;*/
	}
}