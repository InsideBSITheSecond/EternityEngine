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

	// UBOs have to be memory aligned.
	// Be sure that every changes to that structure requires double checking of it memory alignment.
	// Hard to troubleshoot bug may appear if it's not done correctly
	struct GlobalUbo {
		glm::mat4 projectionView{1.f};
		glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f}; // w is intensity
		glm::vec3 lightPosition{-1.f};
		alignas(16) glm::vec4 lightColor{1.f}; // w is intensity
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
			EveGameObject::Map gameObjects;
	};
}