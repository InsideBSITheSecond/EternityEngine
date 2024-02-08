#pragma once

#include "engine/eve_device.hpp"
#include "engine/eve_window.hpp"
#include "engine/eve_game_object.hpp"
#include "engine/eve_renderer.hpp"
#include "engine/eve_descriptors.hpp"

// std
#include <memory>
#include <vector>

namespace eve {
	struct GlobalUbo {
		glm::mat4 projectionView{1.f};
		glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
	};

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

			// note: order of declarations matters
			std::unique_ptr<EveDescriptorPool> globalPool{};
			std::vector<EveGameObject> gameObjects;
	};
}