#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "vulkan/ElaineVulkanUniformBuffer.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanPhysicalDevice.h"
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
	VulkanUniformBuffer::VulkanUniformBuffer(VulkanDevice* InDevice, Elaine::RHIResourceGroup InSlot, size_t InSize, ERHIAccess InResourceState, void* InContent)
		: RHIUniformBuffer(InSlot)
		, mBufferSize(InSize)
		, mDevice(InDevice)
	{
		(void)InResourceState;
		InitBuffer(InSize, InContent);
	}

	void VulkanUniformBuffer::InitBuffer(size_t InSize, void* InContent)
	{
        mDataSize = InSize;
		if (InSize != 0)
		{
			const size_t Alignment = mDevice->GetPhyDevice()->GetDeviceProperties().limits.minUniformBufferOffsetAlignment;
            mBufferSize = ((InSize + Alignment - 1) / Alignment) * Alignment;
            if (!mDevice->GetMemoryManager()->AllocUniformBuffer(mBuffer, mBufferSize, nullptr))
			{
				LOG_FATAL("VulkanRHI: Allocate Uniform Buffer Failed.");
			}
			mBufferHandle = (VkBuffer)mBuffer.mVulkanHandle;
			mBufferOffset = mBuffer.mOffset;
            if (InContent) UpdateBuffer(InContent, 0, InSize);
		}
	}

	VulkanUniformBuffer::~VulkanUniformBuffer()
	{
		if (mDevice && mBuffer.mVulkanHandle)
		{
			mDevice->GetMemoryManager()->FreeUniformBuffer(mBuffer);
		}
	}

    size_t VulkanUniformBuffer::GetOffset() const
    {
        return mBufferOffset;
    }
	void VulkanUniformBuffer::UpdateBuffer(const void* InContent, size_t InOffset, size_t InSize)
	{
		if (InSize == 0) InSize = mDataSize;
        if (InOffset > mDataSize || InSize > mDataSize - InOffset) throw std::out_of_range("Uniform update exceeds buffer");

		if (InContent == nullptr)
			return;

        void* StartPtr = mBuffer.GetMappedPointer(mDevice);
        Memory::MemoryCopy(static_cast<uint8_t*>(StartPtr) + InOffset, InContent, InSize);
        mBuffer.FlushMappedMemory(mDevice);
	}
}
