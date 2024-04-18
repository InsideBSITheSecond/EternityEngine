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
 - [boost](https://github.com/boostorg/boost)
 - [easy_profiler](https://github.com/yse/easy_profiler)
 - [glfw](https://github.com/glfw/glfw)
 - [imgui](https://github.com/ocornut/imgui)
 - [implot](https://github.com/epezent/implot)
 - [JoldPhysics](https://github.com/jrouwe/JoltPhysics)
 - [luacpp](https://github.com/jordanvrtanoski/luacpp)
 - [PerlinNoise](https://github.com/Reputeless/PerlinNoise)
 - [stb](https://github.com/nothings/stb)
 - [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)

## Build (only tested on linux)

required :
 - cmake
 - a c++ compiler
 - common sense
 - vulkan sdk (in system path)

```
git clone https://github.com/InsideBSITheSecond/EternityVoxelEngine.git
cd EternityVoxelEngine
git submodule update --init --recursive
cmake -S . -B build && cmake --build build --config Debug --target all --parallel
```
 
