cmake_minimum_required(VERSION 3.5)
set(PROJECT_NAME EternityVoxelEngine)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

find_package(Vulkan REQUIRED)

# EVE
file(GLOB_RECURSE
SOURCE_FILES
RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine/**.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine.hpp
	${CMAKE_CURRENT_SOURCE_DIR}/src/eve.cpp
)
message("SOURCE_FILES ${SOURCE_FILES}")

add_executable(${PROJECT_NAME}
	${SOURCE_FILES}
)

# Compile flags - you can customize these
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
#target_compile_options(${PROJECT_NAME} PUBLIC "-fsanitize=address")

# Set the output directory for the compiled .o files
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# SHADERS
set(SHADERS_IN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADERS_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")

file(GLOB SHADERS "${SHADERS_IN_DIR}/**.vert" "${SHADERS_IN_DIR}/**.frag")
file(MAKE_DIRECTORY ${SHADERS_OUT_DIR})
foreach(SHADER ${SHADERS})
	get_filename_component(SHADER_NAME ${SHADER} NAME)
	set(SHADER_OUT_NAME "${SHADERS_OUT_DIR}/${SHADER_NAME}.spv")
	list(APPEND SHADER_OUT_NAMES ${SHADER_OUT_NAME})
	add_custom_command(
		MAIN_DEPENDENCY ${SHADER}
		OUTPUT ${SHADER_OUT_NAME}
		COMMAND glslc  ${SHADER} "-o" ${SHADER_OUT_NAME}
		VERBATIM)
endforeach()
add_custom_target(build_shaders DEPENDS ${SHADER_OUT_NAMES})
add_dependencies(${PROJECT_NAME} build_shaders)

# copy data folders
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		${CMAKE_SOURCE_DIR}/gamedata $<TARGET_FILE_DIR:${PROJECT_NAME}>/gamedata)


# ------     BOOST     -------
set(BOOST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/boost)
set(BOOST_INCLUDE_LIBRARIES asio system chrono thread)
add_subdirectory(${BOOST_PATH} EXCLUDE_FROM_ALL)
target_link_libraries(${PROJECT_NAME} Boost::asio Boost::system Boost::chrono Boost::thread)

# ------     GLFW     -------
set(GLFW_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/glfw)
add_subdirectory(${GLFW_PATH})
target_link_libraries(${PROJECT_NAME} glfw)

# ------ Easy Profiler -------
set(EP_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/easy_profiler)
add_subdirectory(${EP_PATH} easy_profiler)
target_link_libraries(${PROJECT_NAME} easy_profiler)

# ------ Jolt Physics -------
set(JOLT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/JoltPhysics)
option(USE_AVX "Enable AVX" OFF)
option(USE_AVX2 "Enable AVX2" OFF)
option(USE_LZCNT "Enable LZCNT" OFF)
option(USE_TZCNT "Enable TZCNT" OFF)
option(USE_F16C "Enable F16C" OFF)
option(USE_FMADD "Enable FMADD" OFF)
add_subdirectory(${JOLT_PATH}/Build jolt)
target_link_libraries(${PROJECT_NAME} Jolt)

# ------    IMGUI    -------
set(IMGUI_PATH ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/imgui)
add_library(imgui
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_demo.cpp
)
target_include_directories(imgui
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}>
    $<INSTALL_INTERFACE:imgui.h>
    $<INSTALL_INTERFACE:imgui_internal.h>
)
# namespaced alias
add_library(${PROJECT_NAME}::imgui ALIAS imgui)
target_link_libraries(${PROJECT_NAME} imgui)
target_include_directories(EternityVoxelEngine PRIVATE src/libs/imgui)
target_include_directories(EternityVoxelEngine PRIVATE src/libs/imgui/backends)

# ------ IMGUI VULKAN -------
add_library(imgui-vulkan ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp)
target_include_directories(imgui-vulkan
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}/backends>
    $<INSTALL_INTERFACE:imgui_impl_vulkan.h>
)
target_link_libraries(imgui-vulkan
    imgui
    vulkan)
add_library(${PROJECT_NAME}::imgui-vulkan ALIAS imgui-vulkan)
target_link_libraries(${PROJECT_NAME} imgui-vulkan)

# ------ IMGUI GLFW -------
add_library(imgui-glfw ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp)
target_include_directories(imgui-glfw
    PUBLIC
    $<BUILD_INTERFACE:${IMGUI_PATH}/backends>
    $<INSTALL_INTERFACE:imgui_impl_glfw.h>
)
target_link_libraries(imgui-glfw
    imgui
    glfw)
add_library(${PROJECT_NAME}::imgui-glfw ALIAS imgui-glfw)
target_link_libraries(${PROJECT_NAME} imgui-glfw)

# ------ IMGUI IMPLOT -------
set(IMPLOT ${CMAKE_CURRENT_SOURCE_DIR}/src/libs/implot)
add_library(implot
    ${IMPLOT}/implot.cpp
    ${IMPLOT}/implot_items.cpp
    ${IMPLOT}/implot_demo.cpp
)
target_include_directories(implot
    PUBLIC
    $<BUILD_INTERFACE:${IMPLOT_PATH}>
    $<INSTALL_INTERFACE:implot.h>
    $<INSTALL_INTERFACE:implot_internal.h>
)
target_link_libraries(implot
    imgui
	imgui-vulkan
	imgui-glfw)
# namespaced alias
add_library(${PROJECT_NAME}::implot ALIAS implot)
target_link_libraries(${PROJECT_NAME} implot)
target_include_directories(${PROJECT_NAME} PRIVATE src/libs/implot)



target_include_directories(${PROJECT_NAME} PRIVATE src/libs/stb)

# linux specific libraries
target_link_libraries(${PROJECT_NAME} dl)
target_link_libraries(${PROJECT_NAME} X11)
target_link_libraries(${PROJECT_NAME} Xxf86vm)
target_link_libraries(${PROJECT_NAME} Xi)
target_link_libraries(${PROJECT_NAME} Xrandr)
