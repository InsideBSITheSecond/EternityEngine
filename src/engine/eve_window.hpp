#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <functional>

namespace eve {
	class EveWindow {
		public:
			EveWindow(int w, int h, std::string name);
			~EveWindow();

			EveWindow(const EveWindow &) = delete;
			EveWindow &operator=(const EveWindow &) = delete;

			bool shouldClose() { return glfwWindowShouldClose(window); }
			VkExtent2D getExtent() { return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}; }
			bool wasWindowResized() { return framebufferResized; }
			void resetWindowResizedFlag() { framebufferResized = false; }

			GLFWwindow *getGLFWwindow() const { return window; }

			void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

			void setMouseWheelCallback(std::function<void(GLFWwindow*, double, double)> callback) { mouseWheelCallback_ = callback; };
			void setKeyboardCallback(std::function<void(GLFWwindow*, int, int, int, int)> callback) { keyboardCallback_ = callback; };

		private:
			static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
			static void mouseWheelCallback(GLFWwindow *window, double xoffset, double yoffset);
			static void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

			void initWindow();
			int width;
			int height;
			bool framebufferResized = false;

			std::function<void(GLFWwindow*, double, double)> mouseWheelCallback_;
			std::function<void(GLFWwindow*, int, int, int, int)> keyboardCallback_;

			std::string windowName;
			GLFWwindow *window;
	};
}