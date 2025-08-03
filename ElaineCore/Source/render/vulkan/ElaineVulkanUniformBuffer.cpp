#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanUniformBuffer.h"
#include "vulkan/ElaineVulkanDevice.h"

namespace VulkanRHI
{
	VulkanUniformBuffer::VulkanUniformBuffer(VulkanDevice* InDevice, size_t InSize, ERHIAccess InResourceState, void* InContent)
		: mBufferSize(InSize)
		, mDevice(InDevice)
	{
		if (InSize != 0)
		{
			if (!InDevice->GetMemoryManager()->AllocUniformBuffer(mBuffer, InSize, InContent))
			{
					LOG_FATAL("VulkanRHI: Allocate Uniform Buffer Failed.");
			}	
			mBufferHandle = (VkBuffer)mBuffer.mVulkanHandle;
			mBufferOffset = mBuffer.mOffset;
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		mDevice->GetMemoryManager()->FreeUniformBuffer(mBuffer);
	}

	void VulkanUniformBuffer::UpdateBuffer(const void* InContent, size_t InOffset, size_t InSize)
	{
		(void)InOffset; (void)InSize;

		if (InContent == nullptr)
			return;

		Memory::MemoryCopy(mBuffer.GetMappedPointer(mDevice), InContent, mBufferSize);
	}
}
