#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanCommandBuffer.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanMemory.h"
#include "vulkan/ElaineVulkanQueue.h"
#include "vulkan/ElaineVulkanPipeline.h"
#include "vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
	static int32 GVulkanUploadCmdBufferSemaphore = 0;
	using namespace Elaine;
	VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* InDevice, VulkanCommandPool* InCommandBufferPool, bool bInIsUploadOnly)
		: mDeivce(InDevice)
		, mCmdPool(InCommandBufferPool)
		, mHandle(nullptr)
		, mFenceSignaledCounter(0u)
		, mSubmittedFenceCounter(0u)
		, mbIsUploadOnly(bInIsUploadOnly)
		, mbIsUniformBufferBarrierAdded(false)
	{
		std::lock_guard<std::recursive_mutex> lock(mCmdPool->GetMutex());
		AllocMemory();

		mFence = mDeivce->GetFenceManager()->AllocateFence();
	}

	VulkanCommandBuffer::~VulkanCommandBuffer()
	{
	}


	void VulkanCommandBuffer::AllocMemory()
	{
		assert(mState == EState::NotAllocated);

		VkCommandBufferAllocateInfo CreateCmdBufInfo;
		Memory::MemoryZero(CreateCmdBufInfo);
		CreateCmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CreateCmdBufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		CreateCmdBufInfo.commandBufferCount = 1;
		CreateCmdBufInfo.commandPool = mCmdPool->GetHandle();

		vkAllocateCommandBuffers(mDeivce->GetDevice(), &CreateCmdBufInfo, &mHandle);

		mState = EState::ReadyForBegin;
	}

	void VulkanCommandBuffer::FreeMemory()
	{
		assert(mState == EState::NotAllocated);
		vkFreeCommandBuffers(mDeivce->GetDevice(), mCmdPool->GetHandle(), 1, &mHandle);
		mHandle = VK_NULL_HANDLE;
		mState = EState::NotAllocated;
	}

	void VulkanCommandBuffer::BeginUniformUpdateBarrier()
	{
		if (!mbIsUniformBufferBarrierAdded)
		{
			VkMemoryBarrier Barrier;
			Memory::MemoryZero(Barrier);
			Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			Barrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			Barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(mHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &Barrier, 0, nullptr, 0, nullptr);
			mbIsUniformBufferBarrierAdded = true;
		}
	}

	void VulkanCommandBuffer::EndUniformUpdateBarrier()
	{
		if (mbIsUniformBufferBarrierAdded)
		{
			VkMemoryBarrier Barrier;
			Memory::MemoryZero(Barrier);
			Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			Barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			Barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkCmdPipelineBarrier(mHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &Barrier, 0, nullptr, 0, nullptr);
			mbIsUniformBufferBarrierAdded = false;
		}
	}

	void VulkanCommandBuffer::EndRenderPass()
	{
		if (IsInsideRenderPass())
		{
			vkCmdEndRenderPass(mHandle);
			mState = EState::IsInsideBegin;
		}
	}

	void VulkanCommandBuffer::AddWaitSemaphore(VkPipelineStageFlags InWaitFlags, VulkanSemaphore* InWaitSemaphore)
	{
		mWaitFlags.push_back(InWaitFlags);
		//InWaitSemaphore->AddRef();
		mWaitSemaphores.push_back(InWaitSemaphore);
	}

	void VulkanCommandBuffer::RefreshFenceStatus()
	{
		if (mState == EState::Submitted)
		{
			VulkanFenceManager* FenceMgr = mFence->GetOwner();
			if (FenceMgr->IsFenceSignaled(mFence))
			{
				//for (VulkanSemaphore* Semaphore : mSubmittedWaitSemaphores)
				//{
				//	//Semaphore->Release();
				//}
				mSubmittedWaitSemaphores.clear();
				FenceMgr->ReleaseFence(mFence);
				mState = EState::NeedReset;
			}
		}
	}

	void VulkanCommandBuffer::BindGfxPipeline(VulkanGfxPipeline* InGfxPipeline)
	{
		assert(InGfxPipeline != nullptr);
		vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, InGfxPipeline->GetHandle());

		{
			std::lock_guard<std::recursive_mutex> lock(mCmdPool->GetMutex());
			static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext())->SetCurrentGfxPipeline(InGfxPipeline);
		}
		
	}

	void VulkanCommandBuffer::BindComputePipeline(VulkanComputePipeline* InPipeline)
	{
		assert(InPipeline != nullptr);
		vkCmdBindPipeline(mHandle, VK_PIPELINE_BIND_POINT_COMPUTE, InPipeline->GetHandle());

		{
			std::lock_guard<std::recursive_mutex> lock(mCmdPool->GetMutex());
			static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext())->SetCurrentComputePipeline(InPipeline);
		}
		
	}

	void VulkanCommandBuffer::DrawPrimitive(uint32_t InVertexCount, uint32_t InInstanceCount, uint32_t InFirstVertex, uint32_t InFirstInstance)
	{
		vkCmdDraw(mHandle, InVertexCount, InInstanceCount, InFirstVertex, InInstanceCount);
	}

	void VulkanCommandBuffer::DrawPrimitiveIndex(uint32_t InIndexCount, uint32_t InInstanceCount, uint32_t InFirstIndex, uint32_t InVertexOffset, uint32_t InFirstInstance)
	{
		vkCmdDrawIndexed(mHandle, InIndexCount, InInstanceCount, InFirstIndex, InVertexOffset, InFirstInstance);
	}

	void VulkanCommandBuffer::CopyBuffer(VkBuffer InSrcBuffer, VkBuffer InDstBuffer, size_t InSize, size_t InSrcBufferOffset, size_t InDstBufferOffset)
	{
		VkBufferCopy CopyRegion = {};
		CopyRegion.srcOffset = InSrcBufferOffset;
		CopyRegion.dstOffset = InDstBufferOffset;
		CopyRegion.size = InSize;

		vkCmdCopyBuffer(mHandle, InSrcBuffer, InDstBuffer, 1, &CopyRegion);
	}

	//void VulkanCommandBuffer::BeginRenderPass()
	//{
	//
	//}

	void VulkanCommandBuffer::Begin()
	{
		{
			std::lock_guard<std::recursive_mutex> lock(mCmdPool->GetMutex());
			if (mState == EState::NeedReset)
			{
				vkResetCommandBuffer(mHandle, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
			}
			else
			{
				assert(mState == EState::ReadyForBegin);
			}

			mState = EState::IsInsideBegin;
		}

		VkCommandBufferBeginInfo CmdBufBeginInfo;
		Memory::MemoryZero(CmdBufBeginInfo);
		CmdBufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		CmdBufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(mHandle, &CmdBufBeginInfo);

	}

	void VulkanCommandBuffer::End()
	{
		if (IsOutsideRenderPass())
		{
			vkEndCommandBuffer(mHandle);
			mState = EState::HasEnded;
		}
	}

	VulkanCommandPool::VulkanCommandPool(VulkanDevice* InDevice, VulkanCommandBufferManager* InMgr)
		: mHandle(nullptr)
		, mDevice(InDevice)
		, mMgr(InMgr)
	{

	}

	void VulkanCommandPool::CreateCommandPool(uint32 QueueFamilyIndex, VkCommandPoolCreateFlagBits flags)
	{
		VkCommandPoolCreateInfo CmdPoolInfo;
		Memory::MemoryZero(CmdPoolInfo);
		CmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		CmdPoolInfo.queueFamilyIndex = QueueFamilyIndex;
		CmdPoolInfo.flags = flags;
		vkCreateCommandPool(mDevice->GetDevice(), &CmdPoolInfo, VULKAN_CPU_ALLOCATOR, &mHandle);
	}

	VulkanCommandPool::~VulkanCommandPool()
	{
		for (int32 Index = 0; Index < mCmdBuffers.size(); ++Index)
		{
			VulkanCommandBuffer* CmdBuffer = mCmdBuffers[Index];
			delete CmdBuffer;
		}

		for (int32 Index = 0; Index < mFreeCmdBuffers.size(); ++Index)
		{
			VulkanCommandBuffer* CmdBuffer = mFreeCmdBuffers[Index];
			delete CmdBuffer;
		}

		vkDestroyCommandPool(mDevice->GetDevice(), mHandle, VULKAN_CPU_ALLOCATOR);
		mHandle = VK_NULL_HANDLE;
	}


	VulkanCommandBuffer* VulkanCommandPool::Create(bool bIsUploadOnly)
	{
		for (int32 Index = mFreeCmdBuffers.size() - 1; Index >= 0; --Index)
		{
			VulkanCommandBuffer* CmdBuffer = mFreeCmdBuffers[Index];

			if (CmdBuffer->mbIsUploadOnly == bIsUploadOnly)
			{
				mFreeCmdBuffers.erase(mFreeCmdBuffers.begin() + Index);
				CmdBuffer->AllocMemory();
				mCmdBuffers.push_back(CmdBuffer);
				return CmdBuffer;
			}
		}

		VulkanCommandBuffer* CmdBuffer = new VulkanCommandBuffer(mDevice, this, bIsUploadOnly);
		mCmdBuffers.push_back(CmdBuffer);
		return CmdBuffer;
	}

	void VulkanCommandPool::FreeUnusedCmdBuffers()
	{
		
	}

	VulkanCommandBufferManager::VulkanCommandBufferManager(VulkanDevice* InDevice, VulkanQueue* InQueue)
		: mDevice(InDevice)
		, mPool(nullptr)
		, mQueue(InQueue)
		, mActiveCmdBufferSemaphore(nullptr)
		, mUploadCmdBufferSemaphore(nullptr)
	{
		mPool = new VulkanCommandPool(mDevice, this);
		mPool->CreateCommandPool(mQueue->GetFamilyIndex());
		if (GVulkanUploadCmdBufferSemaphore)
		{
			mActiveCmdBufferSemaphore = new VulkanSemaphore(mDevice);
		}

		mActiveCmdBuffer = mPool->Create(false);
	}

	VulkanCommandBufferManager::~VulkanCommandBufferManager()
	{
		SAFE_DELETE(mPool);
	}

	void VulkanCommandBufferManager::Initilize()
	{
		mActiveCmdBuffer->Begin();
	}

	void VulkanCommandBufferManager::PrepareForNewActiveCommandBuffer()
	{
		std::lock_guard<std::recursive_mutex> lock(mPool->mMtx);
		if (GVulkanUploadCmdBufferSemaphore)
		{
			mActiveCmdBufferSemaphore = new VulkanSemaphore(mDevice);
		}

		auto& CmdBuffers = mPool->mCmdBuffers;

		for (int32 Index = 0; Index < CmdBuffers.size(); ++Index)
		{
			VulkanCommandBuffer* CmdBuffer = CmdBuffers[Index];
			CmdBuffer->RefreshFenceStatus();
			if (!CmdBuffer->mbIsUploadOnly)
			{
				if (CmdBuffer->mState == VulkanCommandBuffer::EState::ReadyForBegin || CmdBuffer->mState == VulkanCommandBuffer::EState::NeedReset)
				{
					mActiveCmdBuffer = CmdBuffer;
					mActiveCmdBuffer->Begin();
					return;
				}
			}
		}
		mActiveCmdBuffer = mPool->Create(false);
		mActiveCmdBuffer->Begin();
	}

	VulkanCommandBuffer* VulkanCommandBufferManager::GetUploadCmdBuffer()
	{
		std::lock_guard<std::recursive_mutex> lock(mPool->mMtx);
		if(!mUploadCmdBuffer)
		{
			if (GVulkanUploadCmdBufferSemaphore)
			{
				mUploadCmdBufferSemaphore = new VulkanSemaphore(mDevice);
			}

			auto& CmdBuffers = mPool->mCmdBuffers;

			for (int32 Index = 0; Index < CmdBuffers.size(); ++Index)
			{
				VulkanCommandBuffer* CmdBuffer = CmdBuffers[Index];
				CmdBuffer->RefreshFenceStatus();
				if (CmdBuffer->mbIsUploadOnly)
				{
					if (CmdBuffer->mState == VulkanCommandBuffer::EState::ReadyForBegin || CmdBuffer->mState == VulkanCommandBuffer::EState::NeedReset)
					{
						mUploadCmdBuffer = CmdBuffer;
						mUploadCmdBuffer->Begin();
						return mUploadCmdBuffer;
					}
				}
			}
			mUploadCmdBuffer = mPool->Create(true);
			mUploadCmdBuffer->Begin();
		}
		return mUploadCmdBuffer;
	}

	void VulkanCommandBufferManager::WaitForCmdBuffer(VulkanCommandBuffer* CmdBuffer, float TimeInSecondsToWait)
	{
		std::lock_guard<std::recursive_mutex> lock(mPool->mMtx);
		mDevice->GetFenceManager()->WaitForFence(CmdBuffer->mFence, TimeInSecondsToWait);
		CmdBuffer->RefreshFenceStatus();
	}

	void VulkanCommandBufferManager::SubmitUploadCmdBuffer(uint32 NumSignalSemaphores, VkSemaphore* SignalSemaphores)
	{
		std::lock_guard<std::recursive_mutex> lock(mPool->mMtx);
		if (!mUploadCmdBuffer->IsSubmitted() && mUploadCmdBuffer->HasBegun())
		{
			mUploadCmdBuffer->End();
			if (GVulkanUploadCmdBufferSemaphore)
			{
				for (VulkanSemaphore* WaitForThis : mRenderingCompletedSemaphores)
				{
					mUploadCmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, WaitForThis);
				}

				if (NumSignalSemaphores == 0)
				{
					VkSemaphore VKSem = mUploadCmdBufferSemaphore->GetHandle();
					mQueue->Submit(mUploadCmdBuffer, 1, &VKSem);
					mUploadCompletedSemaphores.push_back(mUploadCmdBufferSemaphore);
					mUploadCmdBufferSemaphore = nullptr;
				}
				else
				{
					std::vector<VkSemaphore> VkSemaphores;
					VkSemaphores.push_back(mUploadCmdBufferSemaphore->GetHandle());
					for (int Index = 0; Index < NumSignalSemaphores; ++Index)
					{
						VkSemaphores.push_back(SignalSemaphores[Index]);
					}
					mQueue->Submit(mUploadCmdBuffer, VkSemaphores.size(), VkSemaphores.data());
				}
				mRenderingCompletedSemaphores.clear();
			}
		}
		else
		{
			mQueue->Submit(mUploadCmdBuffer, NumSignalSemaphores, SignalSemaphores);
		}
		mUploadCmdBuffer = nullptr;
	}

	void VulkanCommandBufferManager::SubmitActiveCmdBuffer(const std::vector<VulkanSemaphore*>& SignalSemaphores)
	{
		std::lock_guard<std::recursive_mutex> lock(mPool->mMtx);
		std::vector<VkSemaphore> VkSemaphores;
		VkSemaphores.reserve(SignalSemaphores.size());
		for (VulkanSemaphore* Semaphore : SignalSemaphores)
		{
			VkSemaphores.push_back(Semaphore->GetHandle());
		}
		if (!mActiveCmdBuffer->IsSubmitted() && mActiveCmdBuffer->HasBegun())
		{
			if (!mActiveCmdBuffer->IsOutsideRenderPass())
			{
				mActiveCmdBuffer->EndRenderPass();
			}
			mActiveCmdBuffer->End();
			if (GVulkanUploadCmdBufferSemaphore)
			{
				for (VulkanSemaphore* WaitForThis : mUploadCompletedSemaphores)
				{
					mActiveCmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, WaitForThis);
				}

				VkSemaphores.push_back(mActiveCmdBufferSemaphore->GetHandle());
				mQueue->Submit(mActiveCmdBuffer, VkSemaphores.size(), VkSemaphores.data());
				mRenderingCompletedSemaphores.push_back(mActiveCmdBufferSemaphore);
				mActiveCmdBufferSemaphore = nullptr;
				mUploadCompletedSemaphores.clear();
			}
			else
			{
				mQueue->Submit(mActiveCmdBuffer, VkSemaphores.size(), VkSemaphores.data());
			}
		}
		else
		{
			mQueue->Submit(mActiveCmdBuffer, VkSemaphores.size(), VkSemaphores.data());
		}
		mActiveCmdBuffer = nullptr;
		if (GVulkanUploadCmdBufferSemaphore && mActiveCmdBufferSemaphore != nullptr)
		{
			SAFE_DELETE(mActiveCmdBufferSemaphore);
		}
		PrepareForNewActiveCommandBuffer();
	}
}