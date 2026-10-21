#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"
#include "render/common/ElaineRHIProtocol.h"

namespace VulkanRHI
{
	class VulkanDevice;
	class VulkanCommandBufferManager;
	class VulkanCommandPool;
	class VulkanFence;
    class VulkanDescriptorPool;
    class VulkanDescriptorSet;
	class VulkanSemaphore;
	class VulkanRenderPass;
	class VulkanGfxPipeline;
	class VulkanComputePipeline;

	class ElaineCoreExport VulkanCommandBuffer
	{
	public:
		friend class VulkanCommandBufferManager;
		friend class VulkanCommandPool;
		enum class EState : uint8
		{
			ReadyForBegin,
			IsInsideBegin,
			IsInsideRenderPass,
			HasEnded,
			Submitted,
			NotAllocated,
			NeedReset,
		};
	public:
		VulkanCommandBuffer(VulkanDevice* InDevice, VulkanCommandPool* InCommandBufferPool, bool bInIsUploadOnly);
		~VulkanCommandBuffer();
        VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
        VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
		VulkanCommandPool* GetOwner()
		{
			return mCmdPool;
		}
        void Keep(const Elaine::RHIHandle& Resource) { if (Resource) mResources.push_back(Resource); }
        void KeepDescriptorPool(std::unique_ptr<VulkanDescriptorPool> Pool);
        void KeepDescriptorSet(std::unique_ptr<VulkanDescriptorSet> Set);
        void ReleaseCompletedReferences();
		void Begin();
		void End();
		void BeginUniformUpdateBarrier();
		void EndUniformUpdateBarrier();
		void EndRenderPass();
		void AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, VulkanSemaphore* InWaitSemaphore);
		void RefreshFenceStatus(bool bResetFence = true);
		//void BeginRenderPass(const FVulkanRenderTargetLayout& Layout, VulkanRenderPass* RenderPass, FVulkanFramebuffer* Framebuffer, const VkClearValue* AttachmentClearValues);
		void BindGfxPipeline(VulkanGfxPipeline* InGfxPipeline);
		void BindComputePipeline(VulkanComputePipeline* InPipeline);
		void DrawPrimitive(uint32_t InVertexCount, uint32_t InInstanceCount, uint32_t InFirstVertex, uint32_t InFirstInstance);
		void DrawPrimitiveIndex(uint32_t InIndexCount, uint32_t InInstanceCount, uint32_t InFirstIndex, uint32_t InVertexOffset, uint32_t InFirstInstance);
		void CopyBuffer(VkBuffer InSrcBuffer, VkBuffer InDstBuffer, size_t InSize, size_t InSrcBufferOffset, size_t InDstBufferOffset);
		inline bool IsInsideRenderPass() const
		{
			return mState == EState::IsInsideRenderPass;
		}

		inline bool IsOutsideRenderPass() const
		{
			return mState == EState::IsInsideBegin;
		}

		inline bool HasBegun() const
		{
			return mState == EState::IsInsideBegin || mState == EState::IsInsideRenderPass;
		}

		inline bool HasEnded() const
		{
			return mState == EState::HasEnded;
		}

		inline bool IsSubmitted() const
		{
			return mState == EState::Submitted;
		}

		inline bool IsAllocated() const
		{
			return mState != EState::NotAllocated;
		}

		inline VkCommandBuffer GetHandle() const
		{
			return mHandle;
		}

		void MarkSemaphoresAsSubmitted()
		{
			mWaitFlags.clear();
			mSubmittedWaitSemaphores = mWaitSemaphores;
			mWaitSemaphores.clear();
		}

		VulkanFence* GetFence() const { return mFence; }
        uint64 GetCompletedFenceCounter() const { return mFenceSignaledCounter; }

	private:
		void AllocMemory(VkCommandBufferLevel InBufferLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		void FreeMemory();
	private:
        std::vector<Elaine::RHIHandle> mResources;
        std::vector<std::unique_ptr<VulkanDescriptorPool>> mDescriptorPools;
        std::vector<std::unique_ptr<VulkanDescriptorSet>> mDescriptorSets;
		VulkanDevice*							mDevice;
		VulkanCommandPool*						mCmdPool;
		VkCommandBuffer							mHandle;
		VulkanFence*							mFence;
		volatile uint64							mFenceSignaledCounter;
		volatile uint64							mSubmittedFenceCounter;
		bool									mbIsUploadOnly;
		bool									mbIsUniformBufferBarrierAdded;
		EState									mState = EState::NotAllocated;
		std::vector<VkPipelineStageFlags>		mWaitFlags;
		std::vector<VulkanSemaphore*>			mWaitSemaphores;
		std::vector<VulkanSemaphore*>			mSubmittedWaitSemaphores;
		friend class VulkanQueue;
	};

	class ElaineCoreExport VulkanCommandPool
	{
	public:
		VulkanCommandPool(VulkanDevice* InDevice, VulkanCommandBufferManager* InMgr);
		~VulkanCommandPool();
		
		inline VkCommandPool GetHandle() const
		{
			return mHandle;
		}

		inline VulkanCommandBufferManager* GetMgr()
		{
			return mMgr;
		}

		void CreateCommandPool(uint32 QueueFamilyIndex, VkCommandPoolCreateFlagBits flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		VulkanCommandBuffer* Create(bool bIsUploadOnly);

		void FreeUnusedCmdBuffers();

		inline std::recursive_mutex& GetMutex()
		{
			return mMtx;
		}
		
	private:
		VkCommandPool						mHandle;
		VulkanDevice*						mDevice;
		VulkanCommandBufferManager*			mMgr;
		std::recursive_mutex				mMtx;
		std::vector<VulkanCommandBuffer*>	mCmdBuffers;
		std::vector<VulkanCommandBuffer*>	mFreeCmdBuffers;
		std::vector<VulkanCommandBuffer*>	mUsedCmdBuffers;
		friend class VulkanCommandBufferManager;
	};

	class ElaineCoreExport VulkanCommandBufferManager
	{
	public:
		VulkanCommandBufferManager(VulkanDevice* InDevice, VulkanQueue* InQueue);
		~VulkanCommandBufferManager();
		void Initialize();
		void WaitForCmdBuffer(VulkanCommandBuffer* CmdBuffer, float TimeInSecondsToWait);
		void SubmitUploadCmdBuffer(uint32 NumSignalSemaphores = 0, VkSemaphore* SignalSemaphores = nullptr);
		void SubmitActiveCmdBuffer(const std::vector<VulkanSemaphore*>& SignalSemaphores);
		void PrepareForNewActiveCommandBuffer();
		void RefreshCommandBufferState();
		VulkanQueue* GetVulkanQueue() const { return mQueue; }
		VulkanCommandBuffer* GetUploadCmdBuffer();
		VulkanCommandBuffer* GetActiveCmdBuffer()
		{
			return mActiveCmdBuffer;
		}
	private:
		VulkanDevice*			mDevice;
		VulkanCommandPool*		mPool;
		VulkanQueue*			mQueue;
		VulkanSemaphore*		mActiveCmdBufferSemaphore;
		VulkanSemaphore*		mUploadCmdBufferSemaphore;
		VulkanCommandBuffer*	mActiveCmdBuffer = nullptr;
		VulkanCommandBuffer*	mUploadCmdBuffer = nullptr;
		std::vector<VulkanSemaphore*> mRenderingCompletedSemaphores;
		std::vector<VulkanSemaphore*> mUploadCompletedSemaphores;
	};


}