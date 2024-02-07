#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

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
		private:
			static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
			void initWindow();
			int width;
			int height;
			bool framebufferResized = false;

			std::string windowName;
			GLFWwindow *window;

	};
}