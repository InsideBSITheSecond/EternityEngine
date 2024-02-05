cmake_minimum_required(VERSION 3.5.0)
project(EternityVoxelEngine VERSION 0.1.0 LANGUAGES C CXX)

file(GLOB_RECURSE
SOURCE_FILES
RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**.hpp
)

file(GLOB_RECURSE
    SHADER_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/**.frag
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/**.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/**.spv
)
FOREACH(SHADER_FILE ${SHADER_FILES})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE}
  )
ENDFOREACH()
list(TRANSFORM SHADER_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

file(GLOB_RECURSE
    MODEL_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/models/**.obj
)
FOREACH(MODEL_FILE ${MODEL_FILES})
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${MODEL_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${MODEL_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL_FILE}
  )
ENDFOREACH()
list(TRANSFORM MODEL_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

file(GLOB_RECURSE
    TEXTURE_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} 
    CONFIGURE_DEPENDS

    ${CMAKE_CURRENT_SOURCE_DIR}/textures/**.jpg
    ${CMAKE_CURRENT_SOURCE_DIR}/textures/**.png
)
FOREACH(TEXTURE_FILE ${TEXTURE_FILES})
    add_custom_command(
		OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_FILE}
        COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_FILE}
            ${CMAKE_CURRENT_BINARY_DIR}/${TEXTURE_FILE}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${TEXTURE_FILE}
  )
ENDFOREACH()
list(TRANSFORM TEXTURE_FILES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/)

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

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

