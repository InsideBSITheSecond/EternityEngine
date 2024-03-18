#pragma once
#include "../device/eve_device.hpp"
#include "../device/eve_keyboard.hpp"
#include "../eve_window.hpp"
#include "../rendering/eve_renderer.hpp"
#include "../rendering/eve_descriptors.hpp"
#include "eve_debug.hpp"
//#include "eve_physx.hpp"

#include <memory>

namespace eve {
	class EveKeyboardController;

	class EveWorld {
		public:
			EveWorld(EveDevice &device, EveWindow &window, EveRenderer &renderer, std::unique_ptr<EveDescriptorPool> &pool);
			~EveWorld();

			EveWorld(const EveWorld&) = delete;
			EveWorld &operator=(const EveWorld&) = delete;

			void init();
			void tick(float deltaTime);

			void applyGravity(float deltaTime);
			void spawnObject();
			void loadGameObjects();

		private:
			EveDevice &eveDevice;
			EveWindow &eveWindow;
			EveRenderer &eveRenderer;
			std::unique_ptr<EveDescriptorPool> &globalPool;

			EveGameObject viewerObject;
			EveKeyboardController *keyboardController;

		public: 
			EveGameObject::Map gameObjects;
			EveTerrain eveTerrain{eveDevice};
			EveDebug debugMenu{eveWindow, eveRenderer, eveDevice, globalPool, eveTerrain};
			EveCamera camera{};
			//EvePhysx physx{};
	};
}