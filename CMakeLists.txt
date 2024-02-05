cmake_minimum_required(VERSION 3.5.0)
project(EternityVoxelEngine VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB_RECURSE
SOURCE_FILES
RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.hpp
)

file(COPY models DESTINATION ${CMAKE_BINARY_DIR})
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_BINARY_DIR})

add_executable(EternityVoxelEngine
	${SOURCE_FILES}
	${SHADER_FILES}
	${TEXTURE_FILES}
)

# Set the output directory for the compiled .o files
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/obj)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Compile flags - you can customize these
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Specify additional include directories if needed
target_include_directories(EternityVoxelEngine PRIVATE src/includes)

# Specify additional libraries if needed
target_link_libraries(EternityVoxelEngine -lglfw)
target_link_libraries(EternityVoxelEngine -lvulkan)
target_link_libraries(EternityVoxelEngine -ldl)
target_link_libraries(EternityVoxelEngine -lX11)
target_link_libraries(EternityVoxelEngine -lXxf86vm)
target_link_libraries(EternityVoxelEngine -lXrandr)
target_link_libraries(EternityVoxelEngine -lXi)

