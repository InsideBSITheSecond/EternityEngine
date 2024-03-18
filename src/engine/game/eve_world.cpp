#include "eve_world.hpp"
#include <PxShape.h>
#include <PxMaterial.h>
#include <PxRigidDynamic.h>

namespace eve {

	EveWorld::EveWorld(EveDevice &device, EveWindow &window, EveRenderer &renderer, std::unique_ptr<EveDescriptorPool> &pool)
	: eveDevice{device}, eveWindow{window}, eveRenderer{renderer}, globalPool{pool}, viewerObject{EveGameObject::createGameObject()}{
		loadGameObjects();
		keyboardController = new EveKeyboardController(this);
	}

	void EveWorld::init() {
		debugMenu.init();

		eveWindow.setMouseWheelCallback(std::bind(&EveTerrain::onMouseWheel, &eveTerrain, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		eveWindow.setKeyboardCallback(std::bind(&EveDebug::key_callback, &debugMenu, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5));
		eveWindow.setMouseButtonCallback(std::bind(&EveKeyboardController::mouseButtonCallback, keyboardController, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		eveWindow.setCursorPosCallback(std::bind(&EveKeyboardController::cursorPosCallback, keyboardController, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

		camera.setViewTarget(glm::vec3(-1.f, -2.f, 2.f), glm::vec3(0.f, 0.f, 2.5f));

		physx.createPhysxSimulation();
	}

	EveWorld::~EveWorld() {
		
	}

	void EveWorld::tick(float deltaTime) {
		eveTerrain.tick(deltaTime);
		applyGravity(deltaTime);

		keyboardController->moveInPlaneXZ(eveWindow.getGLFWwindow(), deltaTime, viewerObject);
		camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

		float aspect = eveRenderer.getAspectRatio();
		camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
	}

	void EveWorld::spawnObject() {
		auto newObject = EveGameObject::makeGravityObject(glm::vec3(0, 1, 0), 0.5f);
		newObject.transform.translation = camera.getPosition();
		newObject.model = eveTerrain.eveCube;
		gameObjects.emplace(newObject.getId(), std::move(newObject));
		std::cout << "Spawned a gravity object" << std::endl;
	}

	void EveWorld::applyGravity(float deltaTime) {
		for (auto &kv : gameObjects) {
			EveGameObject &object = kv.second;
			if (object.gravityComponent) {
				object.transform.translation.y += 9.f * object.gravityComponent->force * deltaTime;
			}
		}
	}

	void EveWorld::loadGameObjects()
	{
		std::shared_ptr<EveModel> eveModel;
		
		/*eveModel = EveModel::createModelFromFile(eveDevice, "models/smooth_vase.obj");
		auto smoothVase = EveGameObject::createGameObject();
		smoothVase.model = eveModel;
		smoothVase.transform.translation = {-.5f, .5f, 0.f};
		smoothVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

		eveModel = EveModel::createModelFromFile(eveDevice, "models/flat_vase.obj");
		auto flatVase = EveGameObject::createGameObject();
		flatVase.model = eveModel;
		flatVase.transform.translation = {.5f, .5f, 0.f};
		flatVase.transform.scale = {3.f, 1.5f, 3.f};
		gameObjects.emplace(flatVase.getId(), std::move(flatVase));

		eveModel = EveModel::createModelFromFile(eveDevice, "models/quad.obj");
		auto floor = EveGameObject::createGameObject();
		floor.model = eveModel;
		floor.transform.translation = {0.f, .5f, 0.f};
		floor.transform.scale = {3.f, 1.f, 3.f};
		gameObjects.emplace(floor.getId(), std::move(floor));*/

		 std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}  //
		};

		auto sun = EveGameObject::makeDirectionalLight(0.5f);
		sun.transform.rotation = {1.f, -3.f, -1.f};
		gameObjects.emplace(sun.getId(), std::move(sun));

		for (int i = 0; i < lightColors.size(); i++) {
			auto pointLight = EveGameObject::makePointLight(0.5f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{0.f, -1.f, 0.f});
			pointLight.transform.translation = glm::vec3(rotateLight *glm::vec4(-10.f, -10.f, -10.f, 1.f));
			pointLight.transform.translation.y = -10;
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}
}