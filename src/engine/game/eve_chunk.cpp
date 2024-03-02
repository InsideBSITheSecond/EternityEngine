#include "eve_chunk.hpp"
#include "eve_terrain.hpp"

namespace eve {
	EveVoxel::EveVoxel(unsigned int i, std::string n, bool v) : id{i}, name{n}, value{v} {}

	Octant::Octant(EveVoxel *vox, glm::vec3 pos, int w, Chunk *containerChunk) {
		
		container = containerChunk;
		
		for (int i = 0; i < 8; i++)
			octants[i] = nullptr;

		if (w == MAX_RESOLUTION) {
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

	void Chunk::remesh(Octant *octant) {
		
		if (octant) 
		{
			if (octant->isAllSame || octant->isLeaf) {
				if (octant->voxel->id != 0) {
					auto cube = EveGameObject::createGameObject();
					cube.model = eveTerrain->eveCube;
					cube.transform.translation = octant->position;
					cube.transform.scale = (glm::vec3(octant->width) - 0.05f) / 2;

					boost::lock_guard<boost::mutex> lock(eveTerrain->mutex);
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
				
				vkDeviceWaitIdle(eveTerrain->eveDevice.device());
				EveTerrain *eveTerrain = octant->container->eveTerrain;
				eveTerrain->remeshingProcessing.erase(std::find(eveTerrain->remeshingProcessing.begin(), eveTerrain->remeshingProcessing.end(), this));
				eveTerrain->remeshingProcessed.push_back(this);
				std::string pos = glm::to_string(this->position);
				std::cout << "Finished a chunk remeshing" << pos << std::endl;
			}
		}
	}
}