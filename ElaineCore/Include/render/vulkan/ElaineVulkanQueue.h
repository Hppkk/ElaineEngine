#pragma once
#include "ElaineCorePrerequirements.h"

namespace VulkanRHI
{
	class VulkanDevice;
	class VulkanCommandBuffer;

	class ElaineCoreExport VulkanQueue
	{
	public:
		VulkanQueue(VulkanDevice* Device, uint32 InFamilyIndex);
		~VulkanQueue();
		void		Initilize();
		inline VkQueue GetHandle() const
		{
			return mQueueHandle;
		}

		inline const uint32 GetFamilyIndex()const
		{
			return mFamilyIndex;
		}

		inline const uint32 GetQueueIndex() const
		{
			return mQueueIndex;
		}

		inline uint64 GetSubmitCount() const
		{
			return mSubmitCounter;
		}

		inline VkPipelineStageFlags GetSupportedStageBits()
		{
			return mSupportedStages;
		}

		void Submit(VulkanCommandBuffer* CmdBuffer, uint32 NumSignalSemaphores = 0, VkSemaphore* SignalSemaphores = nullptr);

		inline void Submit(VulkanCommandBuffer* CmdBuffer, VkSemaphore SignalSemaphore)
		{
			Submit(CmdBuffer, 1, &SignalSemaphore);
		}

		void GetLastSubmittedInfo(VulkanCommandBuffer*& OutCmdBuffer, uint64& OutFenceCounter);
	private:
		void UpdateLastSubmittedCommandBuffer(VulkanCommandBuffer* CmdBuffer);
		void FillSupportedStageBits();

	private:
		VulkanDevice*				mDevice;
		VkQueue						mQueueHandle;
		uint32						mFamilyIndex;
		uint32						mQueueIndex;
		VulkanCommandBuffer*		mLastSubmittedCmdBuffer;
		uint64						mLastSubmittedCmdBufferFenceCounter;
		uint64						mSubmitCounter;
		VkPipelineStageFlags		mSupportedStages;
		std::recursive_mutex		mMtx;
	};
}