cmake_minimum_required(VERSION 3.5.0)
set(PROJECT_NAME EternityVoxelEngine)
project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

find_package(Vulkan REQUIRED)
find_package(easy_profiler REQUIRED)

message("CMAKE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}")

set(SHADERS_IN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SHADERS_OUT_DIR "${CMAKE_BINARY_DIR}/shaders")
set(INCL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/libs/**/**")
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

file(GLOB_RECURSE
SOURCE_FILES
RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.hpp
)

add_executable(${PROJECT_NAME}
	${SOURCE_FILES}
)
add_dependencies(${PROJECT_NAME} build_shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/models $<TARGET_FILE_DIR:${PROJECT_NAME}>/models)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_SOURCE_DIR}/textures $<TARGET_FILE_DIR:${PROJECT_NAME}>/textures)
					   
# Set the output directory for the compiled .o files
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compile flags - you can customize these
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

# Specify additional include directories if needed
target_include_directories(EternityVoxelEngine PRIVATE src/libs/easy_profiler/include)

# Specify additional libraries if needed
target_link_libraries(${PROJECT_NAME} -lglfw)
target_link_libraries(${PROJECT_NAME} -lvulkan)
target_link_libraries(${PROJECT_NAME} -ldl)
target_link_libraries(${PROJECT_NAME} -lX11)
target_link_libraries(${PROJECT_NAME} -lXxf86vm)
target_link_libraries(${PROJECT_NAME} -lXrandr)
target_link_libraries(${PROJECT_NAME} -lXi)
target_link_libraries(${PROJECT_NAME} -lboost_thread)
target_link_libraries(${PROJECT_NAME} -lboost_system)
target_link_libraries(${PROJECT_NAME} -lboost_chrono)
target_link_libraries(${PROJECT_NAME} easy_profiler)

include_directories(${PROJECT_NAME} ${INCL_DIR})