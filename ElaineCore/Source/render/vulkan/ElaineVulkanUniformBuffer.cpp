#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanUniformBuffer.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
	// 向后兼容：无槽位构造
	VulkanUniformBuffer::VulkanUniformBuffer(VulkanDevice* InDevice, size_t InSize, ERHIAccess InResourceState, void* InContent)
		: RHIUniformBuffer()
		, mBufferSize(InSize)
		, mDevice(InDevice)
	{
		(void)InResourceState;
		InitBuffer(InSize, InContent);
	}

	// 带槽位的构造
	VulkanUniformBuffer::VulkanUniformBuffer(VulkanDevice* InDevice, Elaine::RHIUniformSlot InSlot, size_t InSize, ERHIAccess InResourceState, void* InContent)
		: RHIUniformBuffer(InSlot)
		, mBufferSize(InSize)
		, mDevice(InDevice)
	{
		(void)InResourceState;
		InitBuffer(InSize, InContent);
	}

	void VulkanUniformBuffer::InitBuffer(size_t InSize, void* InContent)
	{
		if (InSize != 0)
		{
			InSize = InSize * MAX_FRAMES_IN_FLIGHT;
			if (!mDevice->GetMemoryManager()->AllocUniformBuffer(mBuffer, InSize, InContent))
			{
				LOG_FATAL("VulkanRHI: Allocate Uniform Buffer Failed.");
			}
			mBufferHandle = (VkBuffer)mBuffer.mVulkanHandle;
			mBufferOffset = mBuffer.mOffset;
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		if (mDevice && mBuffer.mVulkanHandle)
		{
			mDevice->GetMemoryManager()->FreeUniformBuffer(mBuffer);
		}
	}

	void VulkanUniformBuffer::UpdateBuffer(const void* InContent, size_t InOffset, size_t InSize)
	{
		(void)InOffset; (void)InSize;

		if (InContent == nullptr)
			return;

		VulkanCommandContext* VkCommandCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
		void* StartPrt = (uint8_t*)mBuffer.GetMappedPointer(mDevice) + mBufferSize * VkCommandCtx->GetCurrentFrameIndex();

		Memory::MemoryCopy(StartPrt, InContent, mBufferSize);
	}
}
