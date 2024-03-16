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
		glfwSetScrollCallback(window, mouseWheelCallback);
		glfwSetKeyCallback(window, keyboardCallback);
		glfwSetMouseButtonCallback(window, mouseButtonCallback);
		glfwSetCursorPosCallback(window, cursorPosCallback);
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

	void EveWindow::mouseWheelCallback(GLFWwindow *window, double xoffset, double yoffset) {
		auto eveWindow = reinterpret_cast<EveWindow *>(glfwGetWindowUserPointer(window));
		eveWindow->mouseWheelCallback_(window, xoffset, yoffset);
	}

	void EveWindow::keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		auto eveWindow = reinterpret_cast<EveWindow *>(glfwGetWindowUserPointer(window));
		eveWindow->keyboardCallback_(window, key, scancode, action, mods);
	}

	void EveWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
		auto eveWindow = reinterpret_cast<EveWindow *>(glfwGetWindowUserPointer(window));
		eveWindow->mouseButtonCallback_(window, button, action, mods);
	}

	void EveWindow::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
		auto eveWindow = reinterpret_cast<EveWindow *>(glfwGetWindowUserPointer(window));
		eveWindow->cursorPosCallback_(window, xpos, ypos);
	}
}