#pragma once
namespace VulkanRHI
{
	class ElaineCoreExport VulkanDescriptorSetLayout
	{
	public:
	private:

	};

	class ElaineCoreExport VulkanDescriptorSet
	{
	public:
	private:
		VkDescriptorSet mHandle = nullptr;
		friend class VulkanDescriptorSetManager;
	};

	class ElaineCoreExport VulkanDescriptorPool
	{
	public:
		VulkanDescriptorPool(VulkanDevice* InDevice, uint32 InMaxSetsAllocations);
		VkDescriptorPool GetHandle() const { return mHandle; }
		bool CanAllocate() const { return mNumAllocatedSets < mMaxSetsAllocations; }
		bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VkDescriptorSet* OutSets);
		uint32 GetNumAllocatedSets() const { return mNumAllocatedSets; }
		void Reset();
	private:
		VkDescriptorPool mHandle = nullptr;
		VulkanDevice* mDevice = nullptr;
		uint32 mMaxSetsAllocations = 0u;
		uint32 mNumAllocatedSets = 0u;

	};

	class ElaineCoreExport VulkanDescriptorSetManager
	{
	public:
		VulkanDescriptorSetManager(VulkanDevice* InDevice);
		~VulkanDescriptorSetManager();
		VkDescriptorSetLayout CreateDescriptorSetLayout(const  VkDescriptorSetLayoutCreateInfo& InCreateInfo);
		bool AllocateDescriptorSets(const VkDescriptorSetAllocateInfo& InDescriptorSetAllocateInfo, VulkanDescriptorSet** OutSets);

	private:
		std::vector<VulkanDescriptorPool*> mPools;
		VulkanDescriptorPool* mFreePool = nullptr;
		VulkanDevice* mDevice = nullptr;
		std::set<VkDescriptorSetLayout> mDescriptorSetLayouts;
	};
}