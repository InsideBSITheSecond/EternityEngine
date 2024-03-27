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
		if (si == -1 ) {
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

	int Octant::getChildIndexFromPos(glm::vec3 queryPoint) {
		glm::vec3 topLeftFront = glm::vec3(
			position.x + float(width) / 2,
			position.y - float(width) / 2,
			position.z + float(width) / 2);

		glm::vec3 botRightBack = glm::vec3(
			position.x - float(width) / 2,
			position.y + float(width) / 2,
			position.z - float(width) / 2);

		int index = 0;

		//std::cout << glm::to_string(position) << " " << glm::to_string(queryPoint) << std::endl;
		//std::cout << width << " " << glm::to_string(topLeftFront) << " " << glm::to_string(botRightBack) << std::endl;
		
		if ((queryPoint.y >= topLeftFront.y && queryPoint.y <= botRightBack.y) || width == MAX_RESOLUTION) {
			if ((queryPoint.x <= topLeftFront.x && queryPoint.x >= botRightBack.x) || width == MAX_RESOLUTION) {
				if ((queryPoint.z <= topLeftFront.z && queryPoint.z >= botRightBack.z) || width == MAX_RESOLUTION) {
					if (queryPoint.y >= position.y)
						index |= 4;			
					if (queryPoint.x >= position.x)
						index |= 2;
					if (queryPoint.z >= position.z)
						index |= 1;
				}else {std::cout << "z prob" << std::endl;}
			}else {std::cout << "x prob" << std::endl;}
		}else {std::cout << "y prob" << std::endl;}

		//std::cout << index << " vs " << getOctantIndexFromPos(position, queryPoint) << std::endl; 
		
		return index;
	}

	Octant *Octant::getSmallestContainerAt(glm::vec3 coord) {
		if (!isLeaf && !isAllSame) {
			int childIndex = getChildIndexFromPos(coord);
			return octants[childIndex]->getSmallestContainerAt(coord);
		}
		return this;
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

	Chunk::Chunk(Octant *r, glm::vec3 pos, EveTerrain *terrain): root{r}, position{pos}, eveTerrain{terrain} {
		for (int i = 0; i < 6; i++)
			neighbors[i] = nullptr;
	};

	Chunk::~Chunk(){
		std::cout << "Destroyed chunk" << std::endl;
		eveTerrain->evePhysx.body_interface->RemoveBody(chunkPhysxObject);
		eveTerrain->evePhysx.body_interface->DestroyBody(chunkPhysxObject);
	};

	void Chunk::createFace(Octant *octant, std::vector<glm::vec3> colors, const OctantSide side) {
		/*if (octant->container->position == glm::ivec3(16, 0, -16)) {
			if (octant->position == glm::vec3(9, -3, -23)) {
				std::cout << ">>>>>";
			}
			std::cout << glm::to_string(octant->position) << std::endl;
		}*/
		
		boost::lock_guard<boost::mutex> lock(mutex);
		EASY_FUNCTION(profiler::colors::Red200);

		glm::vec3 offset = octant->getChildLocalOffset();
		int texOffset = abs((int)offset.x) % 2;
		std::vector<EveModel::Vertex> quadVertices = {
			{glm::vec3(-1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(1, 0), octant->voxel->id + texOffset - 1},
			{glm::vec3(1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 1), octant->voxel->id + texOffset - 1},
			{glm::vec3(-1, 0, 1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(1, 1), octant->voxel->id + texOffset - 1},
			{glm::vec3(1, 0, -1), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0), octant->voxel->id + texOffset - 1},
		};
		std::vector<uint32_t> quadIndices = {0, 1, 2, 1, 0, 3};

		/*if (chunkBuilder.vertices.size() == 0) {
			std::cout << glm::to_string(octant->position) << std::endl;
		}*/

		/*eveTerrain->evePhysx.createStaticPlane(
			glm::vec3(float(octant->width) / 2, float(octant->width) / 2, float(octant->width) / 2), 
			position,
			octant->getChildLocalOffset());*/

		if (!octant->octantPhysxObject) {
			BoxShapeSettings floor_shape_settings(Vec3(float(octant->width) / 2, float(octant->width) / 2, float(octant->width) / 2));
			octant->octantPhysxObject = floor_shape_settings.Create().Get();
			chunkShapeSettings.AddShape(Vec3(offset.x, -offset.y, offset.z), Quat::sIdentity(), octant->octantPhysxObject);
		}

		int i = 0;
		for (EveModel::Vertex vertex : quadVertices) {
			vertex.position = rotateV(side, vertex.position);
			vertex.normal = rotateV(side, vertex.normal);
			vertex.position *= float(octant->width) / 2;
			vertex.position += offset;
			vertex.position = offsetV(side, vertex.position, float(octant->width) / 2);
			vertex.color = colors[i++];
			vertex.uv *= octant->width;
			chunkBuilder.vertices.push_back(vertex);
		}

		for (uint32_t index : quadIndices) {
			index += chunkBuilder.vertices.size() - i;
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

				if (octant->position == glm::vec3(7.5, -2.5, -38.5) && side.direction == -2) {
					octant->marked = true;
				}

				if (side.direction == OctantSides::Top.direction) {
					if (octant->container->eveTerrain->playerCurrentLevel == floor(octant->position.y - octant->width)) {
						octant->marked = true;
						//std::cout << "m";
					}
				}

				std::vector<Octant *> neighbors = octant->getNeighbors(side);

				if (neighbors.size() == 1) { // same size
					if ((neighbors.front()->voxel == eveTerrain->voxelMap[0] &&
						octant->voxel != eveTerrain->voxelMap[0]) || octant->forceRender) {
						if (!octant->marked) {
							createFace(octant, WHITE, side);
						}
						else {
							createFace(octant, MARK, side);
						}
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
							else {
								createFace(octant, MARK, side);
							}
						}
					}
				}
			}
		}
	}

	void Chunk::remesh2(Chunk *chunk) {
		//std::cout << chunk->id << "s" << std::endl;
		EASY_BLOCK("Remesh V2");
		EASY_FUNCTION(profiler::colors::Blue100);
		{
			boost::lock_guard<boost::mutex> chunkLock(chunk->mutex);
			chunk->isQueued = true;
			chunk->chunkBuilder.indices.clear();
			chunk->chunkBuilder.vertices.clear();
			chunk->chunkObjectMap.clear();
			chunk->chunkModel.reset();
		}

		if (!chunkPhysxObject.IsInvalid()) {
			eveTerrain->evePhysx.body_interface->RemoveBody(chunkPhysxObject);
			eveTerrain->evePhysx.body_interface->DestroyBody(chunkPhysxObject);
		}

		remesh2rec(chunk->root);

		{
			Ref<Shape> chunkShape = chunkShapeSettings.Create().Get();

			RotatedTranslatedShapeSettings translatedChunkShapeSettings(Vec3(root->position.x, -root->position.y, root->position.z), Quat::sIdentity(), chunkShape);
			Ref<Shape> translatedChunkShape = translatedChunkShapeSettings.Create().Get();

			BodyCreationSettings chunkSettings(translatedChunkShape, Vec3(0, 0, 0), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
			BodyID id = eveTerrain->evePhysx.body_interface->CreateAndAddBody(chunkSettings, EActivation::DontActivate);
			chunkPhysxObject = id;
		}

		EASY_BLOCK("Push chunk object");
		EveTerrain *eveTerrain = chunk->eveTerrain;
		boost::lock_guard<boost::mutex> terrainLock(eveTerrain->mutex);
		boost::lock_guard<boost::mutex> chunkLock(mutex);

		eveTerrain->remeshingProcessing.erase(
			std::find(eveTerrain->remeshingProcessing.begin(),
			eveTerrain->remeshingProcessing.end(),
			this));
		
		eveTerrain->remeshingProcessed.push_back(this);
		/*std::cout << "Finished chunk id:" << id 
			<< " remeshing " << glm::to_string(this->position) 
			<< " vertices: " << chunkBuilder.vertices.size() 
			<< std::endl;*/
		
		//std::cout << chunk->id << "e" << std::endl;
	}
}