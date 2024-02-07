#include "eve_window.hpp"

#include <stdexcept>

namespace eve {
	EveWindow::EveWindow(int w, int h, std::string name) : width{w}, height{h}, windowName{name}{
		initWindow();
	}

	EveWindow::~EveWindow() {
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void EveWindow::initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
	}

	void EveWindow::createWindowSurface(VkInstance instance, VkSurfaceKHR *surface) {
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void EveWindow::framebufferResizedCallback(GLFWwindow *window, int width, int height) {
		auto eveWindow = reinterpret_cast<EveWindow *>(glfwGetWindowUserPointer(window));
		eveWindow->framebufferResized = true;
		eveWindow->width = width;
		eveWindow->height = height;
	}
}