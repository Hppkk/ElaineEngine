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
		
		// 检查是否需要更新
		bool NeedsUpdate() const { return mDirty; }
		void MarkDirty() { mDirty = true; }
		void MarkClean() { mDirty = false; }
		
		// 图像绑定状态检测
		void SetBoundImage(VkImageView InView, VkSampler InSampler);
		bool IsSameImageBinding(VkImageView InView, VkSampler InSampler) const;
		
		// Buffer 绑定状态检测
		void SetBoundBuffer(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange);
		bool IsSameBufferBinding(VkBuffer InBuffer, VkDeviceSize InOffset, VkDeviceSize InRange) const;
		
	private:
		VkDescriptorSet mHandle = nullptr;
		int32 mSet;
		bool mbEmpty;
		bool mDirty = true;
		
		// 图像绑定状态缓存
		VkImageView mBoundImageView = VK_NULL_HANDLE;
		VkSampler mBoundSampler = VK_NULL_HANDLE;
		
		// Buffer 绑定状态缓存
		VkBuffer mBoundBuffer = VK_NULL_HANDLE;
		VkDeviceSize mBoundBufferOffset = 0;
		VkDeviceSize mBoundBufferRange = 0;
		
		friend class VulkanDescriptorSetManager;
		friend class VulkanFrameDescriptorAllocator;
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

	/**
	 * 帧级 DescriptorSet 分配器
	 */
	class ElaineCoreExport VulkanFrameDescriptorAllocator
	{
	public:
		VulkanFrameDescriptorAllocator(VulkanDevice* InDevice, uint32 InFrameCount, VulkanDescriptorSetManager* InManager);
		~VulkanFrameDescriptorAllocator();

		/**
		 * 为当前帧分配一个 DescriptorSet
		 * 
		 * @param InFrameIndex - 当前帧索引 (0 到 FrameCount-1)
		 * @param InLayout - DescriptorSet 布局
		 * @param InSetIndex - Set 索引号 (用于创建 VulkanDescriptorSet)
		 * @return 分配的 DescriptorSet，已标记为 dirty 以便写入
		 */
		VulkanDescriptorSet* AllocateForFrame(uint32 InFrameIndex, VkDescriptorSetLayout InLayout, int32 InSetIndex = 0);

		/**
		 * 重置指定帧的分配状态
		 * 在帧开始时调用，回收上一轮该帧索引的 DescriptorSet
		 * 
		 * @param InFrameIndex - 帧索引
		 */
		void ResetFrame(uint32 InFrameIndex);

		/**
		 * 获取帧数量
		 */
		uint32 GetFrameCount() const { return mFrameCount; }

	private:
		VulkanDevice* mDevice = nullptr;
		VulkanDescriptorSetManager* mManager = nullptr;
		uint32 mFrameCount = 0;

		// 每帧独立的 DescriptorPool (用于临时分配)
		std::vector<VulkanDescriptorPool*> mFramePools;

		// 每帧的已分配 Set 列表 (用于帧结束时统一 reset)
		std::vector<std::vector<VulkanDescriptorSet*>> mAllocatedSets;

		// 每帧的当前分配索引 (复用已分配的 Set)
		std::vector<uint32> mCurrentAllocIndex;
	};
}