#include "eve_world.hpp"

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

		//physx.createPhysxSimulation(true);
		//physx.createStack(PxTransform(PxVec3(0,0,stackZ-=10.0f)), 10, 2.0f);

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
		
		//physx.stepPhysics();

		jolt.tick(deltaTime);
	}

	void EveWorld::spawnObject() {
		auto newObject = EveGameObject::makeGravityObject(glm::vec3(0, 1, 0), 0.5f);
		newObject.transform.translation = camera.getPosition();
		newObject.model = eveTerrain.eveCube;

		// Now create a dynamic body to bounce on the floor
		// Note that this uses the shorthand version of creating and adding a body to the world
		//BodyCreationSettings sphere_settings(new BoxShape(2.f), RVec3(camera.getPosition().x, -camera.getPosition().y, camera.getPosition().z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
		//newObject.gravityComponent->bodyID = jolt.body_interface->CreateAndAddBody(sphere_settings, EActivation::Activate);

		BoxShapeSettings box_shape_settings(Vec3(1.0f, 1.0f, 1.0f));

		// Create the shape
		ShapeSettings::ShapeResult box_shape_result = box_shape_settings.Create();
		ShapeRefC box_shape = box_shape_result.Get(); // We don't expect an error here, but you can check box_shape_result for HasError() / GetError()

		// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
		BodyCreationSettings box_settings(box_shape, RVec3(camera.getPosition().x, -camera.getPosition().y, camera.getPosition().z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);

		// Create the actual rigid body
		newObject.gravityComponent->bodyID = jolt.body_interface->CreateAndAddBody(box_settings, EActivation::Activate); // Note that if we run out of bodies this can return nullptr

		jolt.sphere_id = newObject.gravityComponent->bodyID;

		// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
		// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
		
		jolt.body_interface->SetLinearVelocity(newObject.gravityComponent->bodyID, Vec3(0.0f, -5.0f, 0.0f));

		gameObjects.emplace(newObject.getId(), std::move(newObject));
		std::cout << "Spawned a gravity object" << std::endl;
	}

	void EveWorld::applyGravity(float deltaTime) {
		for (auto &kv : gameObjects) {
			EveGameObject &object = kv.second;
			if (object.gravityComponent) {
				JPH::RVec3 position = jolt.body_interface->GetCenterOfMassPosition(object.gravityComponent->bodyID);
				object.transform.translation.x = position.GetX();
				object.transform.translation.y = -position.GetY();
				object.transform.translation.z = position.GetZ();

				JPH::Quat rotation = jolt.body_interface->GetRotation(object.gravityComponent->bodyID);
				object.transform.rotation.x = rotation.GetX();
				object.transform.rotation.y = rotation.GetY();
				object.transform.rotation.z = rotation.GetZ();
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