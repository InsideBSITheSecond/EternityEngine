# Eternity Voxel Engine
very early work in progress

## Current Features:
 - Octree Voxel System
   <p><img src="https://i.imgur.com/4vnXkAy.png" height="200px"></p>
 - Greedy Meshing
   <p><img src="https://i.imgur.com/CTJbDYh.png" height="200px"></p>
   
   
   
 - Multi-threaded WorldGen & Remeshing
 - Dynamic Physics System
 - OBJ Support
 - Terrain Modification
 - Performance Profiling
 - Point Lights
 - Directional Light

## Planned Features:
 - Behavior Tree AI
 - Fluid System
 - FBX Support
 - Entity Component System
 - Dynamic Lights
 - (netcode)
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
 
