#pragma once

#include "../game/eve_game_object.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext.hpp"
#include "../game/eve_world.hpp"

namespace eve
{
	class EveWorld;
	class EveKeyboardController
	{
	public:
		EveKeyboardController(EveWorld *world);
		~EveKeyboardController();
		struct KeyMappings
		{
			int moveLeft = GLFW_KEY_A;
			int moveRight = GLFW_KEY_D;
			int moveForward = GLFW_KEY_W;
			int moveBackward = GLFW_KEY_S;
			int moveUp = GLFW_KEY_E;
			int moveDown = GLFW_KEY_Q;
			int lookLeft = GLFW_KEY_LEFT;
			int lookRight = GLFW_KEY_RIGHT;
			int lookUp = GLFW_KEY_UP;
			int lookDown = GLFW_KEY_DOWN;

			int toggleDebug = GLFW_KEY_F1;
		};

		void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
		void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);
		void moveInPlaneXZ(GLFWwindow *window, float dt, EveGameObject &gameObject);

		KeyMappings keys{};
		float moveSpeed{30.f};
		float lookSpeed{1.5f};

		glm::vec2 lastCursorPos = glm::vec2(0);

		int leftMouseButton = 0;
		int rightMouseButton = 0;
		int middleMouseButton = 0;

	private:
		EveWorld *eveWorld;
	};

	
}