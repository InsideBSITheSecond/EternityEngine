#pragma once

#include "../device/eve_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace eve
{

	class EveDescriptorSetLayout
	{
	public:
		class Builder
		{
		public:
			Builder(EveDevice &eveDevice) : eveDevice{eveDevice} {}

			Builder &addBinding(
				uint32_t binding,
				VkDescriptorType descriptorType,
				VkShaderStageFlags stageFlags,
				uint32_t count = 1);

			std::unique_ptr<EveDescriptorSetLayout> build(std::vector<VkDescriptorBindingFlags> flags) const;

		private:
			EveDevice &eveDevice;
			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
		};

		EveDescriptorSetLayout(
			EveDevice &eveDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings, std::vector<VkDescriptorBindingFlags> flags);
		~EveDescriptorSetLayout();
		EveDescriptorSetLayout(const EveDescriptorSetLayout &) = delete;
		EveDescriptorSetLayout &operator=(const EveDescriptorSetLayout &) = delete;

		VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

	private:
		EveDevice &eveDevice;
		VkDescriptorSetLayout descriptorSetLayout;
		std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
		std::vector<VkDescriptorBindingFlags> flags;

		friend class EveDescriptorWriter;
	};

	class EveDescriptorPool
	{
	public:
		class Builder
		{
		public:
			Builder(EveDevice &eveDevice) : eveDevice{eveDevice} {}

			Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
			Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
			Builder &setMaxSets(uint32_t count);
			std::unique_ptr<EveDescriptorPool> build() const;

		private:
			EveDevice &eveDevice;
			std::vector<VkDescriptorPoolSize> poolSizes{};
			uint32_t maxSets = 1000;
			VkDescriptorPoolCreateFlags poolFlags = 0;
		};

		EveDescriptorPool(
			EveDevice &eveDevice,
			uint32_t maxSets,
			VkDescriptorPoolCreateFlags poolFlags,
			const std::vector<VkDescriptorPoolSize> &poolSizes);
		~EveDescriptorPool();
		EveDescriptorPool(const EveDescriptorPool &) = delete;
		EveDescriptorPool &operator=(const EveDescriptorPool &) = delete;

		bool allocateDescriptor(
			const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet &descriptor) const;

		void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

		void resetPool();

		VkDescriptorPool *getDescriptorPool() { return &descriptorPool; }

	private:
		EveDevice &eveDevice;
		VkDescriptorPool descriptorPool;

		friend class EveDescriptorWriter;
	};

	class EveDescriptorWriter
	{
	public:
		EveDescriptorWriter(EveDescriptorSetLayout &setLayout, EveDescriptorPool &pool);

		EveDescriptorWriter &writeBuffer(uint32_t binding, VkDescriptorBufferInfo *bufferInfo);
		EveDescriptorWriter &writeImage(uint32_t binding, std::vector<VkDescriptorImageInfo> imageInfo);

		bool build(VkDescriptorSet &set);
		void overwrite(VkDescriptorSet &set);

	private:
		EveDescriptorSetLayout &setLayout;
		EveDescriptorPool &pool;
		std::vector<VkWriteDescriptorSet> writes;
	};

} // namespace lve