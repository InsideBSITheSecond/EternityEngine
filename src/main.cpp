#include "app.hpp"

// std
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <filesystem>
#include <unistd.h>
namespace fs = std::filesystem;

int main() {
	std::cout << fs::current_path() << std::endl;
	std::cout << getpid() << std::endl;
    eve::App app{};
    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}