#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanUtil.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"

namespace VulkanRHI
{
	int32 GWaitForIdleOnSubmit = 0;

	VulkanQueue::VulkanQueue(VulkanDevice* Device, uint32 InFamilyIndex)
		: mDevice(Device)
		, mQueueHandle(nullptr)
		, mFamilyIndex(InFamilyIndex)
		, mQueueIndex(0u)
		, mLastSubmittedCmdBuffer(nullptr)
		, mLastSubmittedCmdBufferFenceCounter(0u)
		, mSubmitCounter(0u)

	{
		
	}

	VulkanQueue::~VulkanQueue()
	{

	}

	void VulkanQueue::Initilize()
	{
		vkGetDeviceQueue(mDevice->GetDevice(), mFamilyIndex, mQueueIndex, &mQueueHandle);
		FillSupportedStageBits();
	}

	void VulkanQueue::GetLastSubmittedInfo(VulkanCommandBuffer*& OutCmdBuffer, uint64& OutFenceCounter)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		OutCmdBuffer = mLastSubmittedCmdBuffer;
		OutFenceCounter = mLastSubmittedCmdBufferFenceCounter;
	}

	void VulkanQueue::Submit(VulkanCommandBuffer* CmdBuffer, uint32 NumSignalSemaphores, VkSemaphore* SignalSemaphores)
	{
		assert(CmdBuffer->HasEnded());
		VulkanFence* Fence = CmdBuffer->mFence;
		assert(!Fence->IsSignaled());

		const VkCommandBuffer CmdBuffers[] = { CmdBuffer->GetHandle() };

		VkSubmitInfo SubmitInfo;
		Memory::MemoryZero(SubmitInfo);
		SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		SubmitInfo.commandBufferCount = 1;
		SubmitInfo.pCommandBuffers = CmdBuffers;
		SubmitInfo.signalSemaphoreCount = NumSignalSemaphores;
		SubmitInfo.pSignalSemaphores = SignalSemaphores;

		std::vector<VkSemaphore> WaitSemaphores;
		if (CmdBuffer->mWaitSemaphores.size() > 0)
		{
			WaitSemaphores.resize(CmdBuffer->mWaitSemaphores.size());
			for (VulkanSemaphore* Semaphore : CmdBuffer->mWaitSemaphores)
			{
				WaitSemaphores.push_back(Semaphore->GetHandle());
			}
			SubmitInfo.waitSemaphoreCount = (uint32)CmdBuffer->mWaitSemaphores.size();
			SubmitInfo.pWaitSemaphores = WaitSemaphores.data();
			SubmitInfo.pWaitDstStageMask = CmdBuffer->mWaitFlags.data();
		}


		vkQueueSubmit(mQueueHandle, 1, &SubmitInfo, Fence->GetHandle());
	

		CmdBuffer->mState = VulkanCommandBuffer::EState::Submitted;
		CmdBuffer->MarkSemaphoresAsSubmitted();
		CmdBuffer->mSubmittedFenceCounter = CmdBuffer->mFenceSignaledCounter;


		UpdateLastSubmittedCommandBuffer(CmdBuffer);

		//CmdBuffer->GetOwner()->RefreshFenceStatus(CmdBuffer);

		mDevice->GetStagingManager()->ProcessPendingFree(false, false);
	}

	void VulkanQueue::UpdateLastSubmittedCommandBuffer(VulkanCommandBuffer* CmdBuffer)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		mLastSubmittedCmdBuffer = CmdBuffer;
		//mLastSubmittedCmdBufferFenceCounter = CmdBuffer->GetFenceSignaledCounterH();
		++mSubmitCounter;
	}

	void VulkanQueue::FillSupportedStageBits()
	{
		const VkQueueFamilyProperties& QueueProps = mDevice->GetPhyDevice()->GetQueueFamilyProps()[mFamilyIndex];

		mSupportedStages =
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT |
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
			VK_PIPELINE_STAGE_HOST_BIT |
			VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

		if (VKHasAnyFlags(QueueProps.queueFlags, VK_QUEUE_GRAPHICS_BIT))
		{
			mSupportedStages |=
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT |
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_TRANSFER_BIT |
				VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;

			if (mDevice->GetPhyDevice()->GetPhysicalFeatures().geometryShader)
			{
				mSupportedStages |= VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
			}

			mSupportedStages |= VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
			mSupportedStages |= VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;

		}

		if (VKHasAnyFlags(QueueProps.queueFlags, VK_QUEUE_COMPUTE_BIT))
		{
			mSupportedStages |=
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT |
				VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
				VK_PIPELINE_STAGE_TRANSFER_BIT;

			mSupportedStages |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
		}

		if (VKHasAnyFlags(QueueProps.queueFlags, VK_QUEUE_TRANSFER_BIT))
		{
			mSupportedStages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
	}
}