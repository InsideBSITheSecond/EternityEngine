#pragma once

#include "engine/eve_device.hpp"
#include "engine/eve_window.hpp"
#include "engine/eve_game_object.hpp"
#include "engine/eve_renderer.hpp"

// std
#include <memory>
#include <vector>

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

			EveWindow eveWindow{WIDTH, HEIGHT, "Vulkan"};
			EveDevice eveDevice{eveWindow};
			EveRenderer eveRenderer{eveWindow, eveDevice};
			std::vector<EveGameObject> gameObjects;
	};
}