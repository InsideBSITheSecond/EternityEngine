# Eternity Voxel Engine
powered by Vulkan

## Features:
 - Octree Voxel System
 - OBJ model support
 - Entity Component System
 - World Generation
 - Dynamic Light
 - Terrain Modification
 - Dynamic Collision
 - Performance Profiling
 - Point light
 - Directional light
 - (modding api)
 - (data driven)

## Libraries
 - Vulkan
 - GLFW3
 - Boost
 - PerlinNoise
 - JoltPhysics
 - ImGUI + ImPlot
 - Easy Profiler

## Build (only tested on linux)
required :
 - cmake
 - a c++ compiler
 - common sense
 - vulkan sdk (in system path)
 - glfw3 (in system path)
 - boost (in system path)

```
git clone https://github.com/InsideBSITheSecond/EternityVoxelEngine.git
cd EternityVoxelEngine
git submodule update --init --recursive
mkdir build
cd build
cmake -S ..
cmake --build . --config Debug --target all --parallel
```
 
