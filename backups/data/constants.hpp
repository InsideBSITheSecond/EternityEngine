#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "includes.hpp"
#include <string>
#include <cstdint>
#include <vector>

//#define NDEBUG

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#endif