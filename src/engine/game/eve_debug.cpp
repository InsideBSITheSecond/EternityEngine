#include "eve_debug.hpp"
#include "../utils/eve_utils.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <string.h>
#include <iostream>
#include <unordered_map>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

namespace std
{
	template <>
	struct hash<eve::EveDebug::Vertex>
	{
		size_t operator()(eve::EveDebug::Vertex const &vertex) const
		{
			size_t seed = 0;
			eve::hash_combine(seed, vertex.position, vertex.uv, vertex.color);
			return seed;
		}
	};
}

namespace eve
{
	EveDebug::EveDebug(EveWindow &window, EveRenderer &renderer, EveDevice &device, std::unique_ptr<EveDescriptorPool> &pool, EveTerrain &terrain)
	: eveWindow{window}, eveRenderer{renderer}, eveDevice{device}, globalPool{pool}, eveTerrain{terrain}
	{
		//createVertexBuffers(builder.vertices);
		//createIndexBuffers(builder.indices);

	}

	EveDebug::~EveDebug()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImPlot::DestroyContext();
		ImGui::DestroyContext();
	}

	/*std::unique_ptr<EveDebug> EveDebug::createModelFromFile(EveDevice &device, const std::string &filepath)
	{
		Builder builder{};
		builder.loadModel(filepath);
		std::cout << "Vertex count: " << builder.vertices.size() << std::endl;
		return std::make_unique<EveDebug>(device, builder);
	}*/

	/*void EveDebug::createVertexBuffers(const std::vector<Vertex> &vertices)
	{
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		uint32_t vertexSize = sizeof(vertices[0]);
		EveBuffer stagingBuffer{
			eveDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)vertices.data());

		vertexBuffer = std::make_unique<EveBuffer>(
			eveDevice,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		eveDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}*/

	/*void EveDebug::createIndexBuffers(const std::vector<uint32_t> &indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;

		if (!hasIndexBuffer)
			return;

		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

		uint32_t indexSize = sizeof(indices[0]);

		EveBuffer stagingBuffer{
			eveDevice,
			indexSize,
			indexCount,
						VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void *)indices.data());

		indexBuffer = std::make_unique<EveBuffer>(
			eveDevice,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		eveDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}*/

	void EveDebug::draw(VkCommandBuffer commandBuffer)
	{
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
	}

	std::vector<VkVertexInputBindingDescription> EveDebug::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> EveDebug::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
		attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)});
		attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)});
		attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)});

		return attributeDescriptions;
	}

	/*void EveDebug::Builder::loadModel(const std::string &filepath)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		std::string enginePath = ENGINE_DIR + filepath;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
		{
			throw std::runtime_error(warn + err);
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		for (const auto &shape : shapes)
		{
			for (const auto &index : shape.mesh.indices)
			{
				Vertex vertex{};

				if (index.vertex_index >= 0)
				{
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]};

					vertex.color = {
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2]};
				}

				if (index.normal_index >= 0)
				{
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]};
				}

				if (index.texcoord_index >= 0)
				{
					vertex.uv = {
						attrib.texcoords[3 * index.texcoord_index + 0],
						attrib.texcoords[3 * index.texcoord_index + 1],
					};
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(vertex);
				}
				indices.push_back(uniqueVertices[vertex]);
			}
		}
	}*/

	void EveDebug::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
			open = !open;
		}
	}

	void EveDebug::init() {
		ImGui::CreateContext();
		ImPlot::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// Setup Platform/Renderer bindings
		ImGui_ImplGlfw_InitForVulkan(eveWindow.getGLFWwindow(), true);

		ImGui_ImplVulkan_InitInfo init_info{};
		init_info.Instance = eveDevice.getInstance();
		init_info.PhysicalDevice = eveDevice.getPhysicalDevice();
		init_info.Device = eveDevice.device();
		init_info.QueueFamily = eveDevice.findPhysicalQueueFamilies().graphicsFamily;
		init_info.Queue = eveDevice.graphicsQueue();
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = *globalPool->getDescriptorPool();
		init_info.Allocator = nullptr;
		init_info.MinImageCount = 2;
		init_info.ImageCount = EveSwapChain::MAX_FRAMES_IN_FLIGHT;
		init_info.CheckVkResultFn = nullptr;
		init_info.RenderPass = eveRenderer.getSwapChainRenderPass();

		ImGui_ImplVulkan_Init(&init_info);
		//ImGui_ImplVulkan_CreateFontsTexture();
	}

	void EveDebug::update(FrameInfo frameInfo) {
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if (open) {
			EveDebug::drawControls();
			if (showDemo) EveDebug::drawDemo();
			if (showInfo) EveDebug::drawInfo(frameInfo);
			if (showPlotDemo) EveDebug::drawPlotDemo();
		}

		ImGui::Render();
	}

	void EveDebug::drawDemo() {
		ImGui::ShowDemoWindow();
	}

	void EveDebug::drawPlotDemo() {
		ImPlot::ShowDemoWindow();
	}

	void EveDebug::drawControls() {
		ImGui::Begin("Controls");
		ImGui::Checkbox("Infos", &showInfo);
		ImGui::Checkbox("Demo", &showDemo);
		ImGui::Checkbox("Plot Demo", &showPlotDemo);
		ImGui::End();
	}

	void EveDebug::drawInfo(FrameInfo frameInfo) {
    	ImGuiIO& io = ImGui::GetIO(); (void)io;
		
		ImGui::Begin("Infos");
		glm::vec3 camPos = frameInfo.camera.getPosition(); 
		ImGui::Text("camera position: %f %f %f", camPos.x, camPos.y, camPos.z);

		Chunk *chunk = nullptr;
		glm::vec3 chunkPos = glm::vec3(0);
		chunk = eveTerrain.findContainerChunkAt(camPos);
		if (chunk)
			chunkPos = chunk->position;
		ImGui::Text("Camera in chunk: %f %f %f", chunkPos.x, chunkPos.y, chunkPos.z);

		Octant *octant = nullptr;
		glm::vec3 octantPos = glm::vec3(0);
		if (chunk) {
			octant = chunk->root->getSmallestContainerAt(camPos);
			if (octant)
				octantPos = octant->position;
		}
		ImGui::Text("Camera in octant: %f %f %f", octantPos.x, octantPos.y, octantPos.z);


		ImGui::SeparatorText("noising queue");
		ImGui::Text("candidates: %zu ", eveTerrain.noisingCandidates.size()); ImGui::SameLine();
		ImGui::Text("processing: %zu ", eveTerrain.noisingProcessing.size()); ImGui::SameLine();
		ImGui::Text("processed: %zu ", eveTerrain.noisingProcessed.size());
		
		ImGui::SeparatorText("remeshing queue");
		ImGui::Text("candidates: %zu ", eveTerrain.remeshingCandidates.size()); ImGui::SameLine();
		ImGui::Text("processing: %zu ", eveTerrain.remeshingProcessing.size()); ImGui::SameLine();
		ImGui::Text("processed: %zu ", eveTerrain.remeshingProcessed.size());

		ImGui::Separator();
		ImGui::Text("chunk map: %zu ", eveTerrain.chunkMap.size());

		if (ImGui::CollapsingHeader("Rendering")) {
			static int renderMode = 0;
			ImGui::RadioButton("fill", &renderMode, 0); ImGui::SameLine();
			ImGui::RadioButton("line", &renderMode, 1);
			if (renderMode == 0) requestedRenderMode = VK_POLYGON_MODE_FILL;
			else if (renderMode == 1) requestedRenderMode = VK_POLYGON_MODE_LINE;

			static int meshingMode = 1;
			ImGui::RadioButton("octant", &meshingMode, 0); ImGui::SameLine();
			ImGui::RadioButton("chunk", &meshingMode, 1);
			if (meshingMode == 0) eveTerrain.meshingMode = MESHING_OCTANT;
			else if (meshingMode == 1) eveTerrain.meshingMode = MESHING_CHUNK;
		}
		
		if (ImGui::CollapsingHeader("FPS")) {
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

			static bool animate = true;
			ImGui::Checkbox("Animate", &animate);

			static float iofps[90] = {};
			static float ioft[90] = {};
			static float vkft[90] = {};
			static int values_offset = 0;
			static double refresh_time = 0.0;

			//std::cout << frametime << std::endl;
			//std::cout << frameIndex << std::endl;

			if (!animate || refresh_time == 0.0)
				refresh_time = ImGui::GetTime();
			while (refresh_time < ImGui::GetTime()) // Create data at fixed 60 Hz rate for the demo
			{
				iofps[values_offset] = io.Framerate;
				ioft[values_offset] = 1000.0f / io.Framerate;
				vkft[values_offset] = frameInfo.frameTime;
				values_offset = (values_offset + 1) % IM_ARRAYSIZE(iofps);
				refresh_time += 1.0f / 60.0f;
			}

			// Plots can display overlay texts
			// (in this example, we will display an average value)
			{
				float average = 0.0f;
				for (int n = 0; n < IM_ARRAYSIZE(iofps); n++)
					average += iofps[n];
				average /= (float)IM_ARRAYSIZE(iofps);
				char overlay[32];
				sprintf(overlay, "avg %f fps", average);
				ImGui::PlotLines("Graph", iofps, IM_ARRAYSIZE(iofps), values_offset, overlay, 0.0f, 60.0f, ImVec2(0, 80.0f));

				ImPlot::SetNextAxesToFit();
				if (ImPlot::BeginPlot("FPS")) {
					ImPlot::PlotBars("iofps", iofps, IM_ARRAYSIZE(iofps));
					ImPlot::PlotBars("ioft", ioft, IM_ARRAYSIZE(ioft));
					ImPlot::PlotBars("vkft", vkft, IM_ARRAYSIZE(vkft));
					//ImPlot::PlotLine("My Line Plot", x_data, y_data, 1000);
					ImPlot::EndPlot();
				}
			}
		}
		
		if (ImGui::CollapsingHeader("GameObjects")) {
			if (ImGui::CollapsingHeader("Voxel Edit")) {
				static glm::ivec3 pos = glm::ivec3(0);
				static bool liveRebuild = true;

				ImGui::Checkbox("live slider terrain rebuild", &liveRebuild);

				static glm::ivec2 range = glm::ivec2(-24, 24);
				ImGui::InputInt("min", &range.x);
				ImGui::InputInt("max", &range.y);
				if (ImGui::SliderInt3("voxel coords", glm::value_ptr(pos), range.x, range.y)) {
					if (liveRebuild){
						//eveTerrain.changeTerrain(pos, eveTerrain.voxelMap[0]);
						//eveTerrain.needRebuild = true;
					}
				}
				if (ImGui::Button("change terrain at slider position")){
					//eveTerrain.changeTerrain(pos, eveTerrain.voxelMap[0]);
					//eveTerrain.needRebuild = true;
				}
			}
			if (ImGui::CollapsingHeader("Terrain Properties")) {
				if (ImGui::Button("reset terrain")){
					eveTerrain.reset();
				} ImGui::SameLine();

				if (ImGui::Button("remesh")){
					eveTerrain.remesh();
				}


				ImGui::Text("Chunks to generate:");
				ImGui::InputInt2("x", glm::value_ptr(frameInfo.terrain.xRange));
				ImGui::InputInt2("y", glm::value_ptr(frameInfo.terrain.yRange));
				ImGui::InputInt2("z", glm::value_ptr(frameInfo.terrain.zRange));

				ImGui::SeparatorText("Sides to remesh");
				ImGui::Checkbox("top", &frameInfo.terrain.sidesToRemesh[0]); ImGui::SameLine();
				ImGui::Checkbox("left", &frameInfo.terrain.sidesToRemesh[2]); ImGui::SameLine();
				ImGui::Checkbox("near", &frameInfo.terrain.sidesToRemesh[4]);
				ImGui::Checkbox("down", &frameInfo.terrain.sidesToRemesh[1]); ImGui::SameLine();
				ImGui::Checkbox("right", &frameInfo.terrain.sidesToRemesh[3]); ImGui::SameLine();
				ImGui::Checkbox("far", &frameInfo.terrain.sidesToRemesh[5]);
			}
			
			/*for (auto& kv : chunkMap)
			{
				auto& obj = kv.second;
				ImGui::PushID(kv.first);
				ImGui::SeparatorText(obj.pointLightComponent ? "Light" : "Not Light");
            	ImGui::ColorEdit3("color", glm::value_ptr(obj.color));
				ImGui::SliderFloat3("translation", glm::value_ptr(obj.transform.translation), -10.f, 10.f);
				ImGui::SliderFloat3("scale", glm::value_ptr(obj.transform.scale), 0.f, 10.f);
				ImGui::SliderFloat3("rotation", glm::value_ptr(obj.transform.rotation), -3.14f, 3.14f);
				ImGui::PopID();
			}*/
		}

		ImGui::End();
	}
}