#pragma once

#include "engine/device/eve_device.hpp"
#include "engine/eve_window.hpp"
#include "engine/game/eve_game_object.hpp"
#include "engine/game/eve_debug.hpp"
#include "engine/rendering/eve_renderer.hpp"
#include "engine/rendering/eve_descriptors.hpp"

// std
#include <memory>
#include <vector>
#include "engine/game/eve_terrain.hpp"

namespace eve {
	class App {
		public:
			static constexpr int WIDTH = 800;
			static constexpr int HEIGHT = 600;

			App();
			~App();

			App(const App&) = delete;
			App &operator=(const App&) = delete;

			void run();
		private:
			void loadGameObjects();

			EveWindow eveWindow{WIDTH, HEIGHT, "Eternity Voxel Engine"};
			EveDevice eveDevice{eveWindow};
			EveRenderer eveRenderer{eveWindow, eveDevice};


			// note: order of declarations matters
			std::unique_ptr<EveDescriptorPool> globalPool{};
			EveGameObject::Map gameObjects;
			EveTerrain eveTerrain{eveDevice};
			EveDebug debugMenu{eveWindow, eveRenderer, eveDevice, globalPool, eveTerrain};
	};
}