#pragma once
namespace VulkanRHI
{

	class VulkanUniformBuffer;

	class ElaineCoreExport VulkanDescriptorSetLayout
	{
	public:
	private:

	};

	class ElaineCoreExport VulkanDescriptorSet
	{
	public:
		VulkanDescriptorSet(int32 InSet, bool InEmpty = false);
		VkDescriptorSet& GetHandle() { return mHandle; }
		int32 GetSet() const { return mSet; }
	private:
		VkDescriptorSet mHandle = nullptr;
		int32 mSet;
		bool mbEmpty;
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

		void WriteUniformBufferToDescriptorSet(VulkanUniformBuffer* InUniformBuffer, VulkanDescriptorSet* InDescriptorSet);
		void WriteImageToDescriptorSet(VulkanTexture* InImage, VkSampler InSampler, VulkanDescriptorSet* InDescriptorSet);

	private:
		std::vector<VulkanDescriptorPool*> mPools;
		VulkanDescriptorPool* mFreePool = nullptr;
		VulkanDevice* mDevice = nullptr;
		std::set<VkDescriptorSetLayout> mDescriptorSetLayouts;
		std::vector<VulkanDescriptorSet*> mVulkanDescriptorSets;
	};
}