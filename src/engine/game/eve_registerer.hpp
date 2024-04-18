#pragma once

#include "eve_game_object.hpp"

#include <string>
#include <map>

namespace eve {
	enum t_registry {
		ET_BLOCK,
		ET_FLUID,
		ET_RECIPE,
		ET_MULTIBLOCK,
		ET_ENTITY,
		ET_FACTION,
		ET_BIOME,
	};

	class EveRegisterer {
		public:
			using id_t = unsigned int;

			id_t registeryAdd(std::string sid, t_registry type);
			void registeryGet(id_t id, t_registry type);

		private:
			std::map<unsigned int, EveGameObject> blockRegistry;
			std::map<unsigned int, EveGameObject> fluidRegistry;

			std::map<unsigned int, int> recipeRegistry;
			std::map<unsigned int, int> multiblockRegistry;
			std::map<unsigned int, int> entityRegistry;
			std::map<unsigned int, int> factionRegistry;
			std::map<unsigned int, int> biomeRegistry;
	};
}