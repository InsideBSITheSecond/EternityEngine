#pragma once

#include "engine/eve_device.hpp"
#include "engine/eve_window.hpp"
#include "engine/eve_game_object.hpp"
#include "engine/eve_renderer.hpp"
#include "engine/eve_descriptors.hpp"

#include "libs/imgui.h"

// std
#include <memory>
#include <vector>

namespace eve {
	class App {
		public:
			static constexpr int WIDTH = 1920;
			static constexpr int HEIGHT = 1080;

			App();
			~App();

			App(const App&) = delete;
			App &operator=(const App&) = delete;

			void createImGui();

			void run();
		private:
			void loadGameObjects();

			EveWindow eveWindow{WIDTH, HEIGHT, "Vulkan"};
			EveDevice eveDevice{eveWindow};
			EveRenderer eveRenderer{eveWindow, eveDevice};

			// note: order of declarations matters
			std::unique_ptr<EveDescriptorPool> globalPool{};
			EveGameObject::Map gameObjects;
	};
}