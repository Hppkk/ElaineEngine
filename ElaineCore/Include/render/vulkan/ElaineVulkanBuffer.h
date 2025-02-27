#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include "render/vulkan/ElaineVulkanMemory.h"

namespace VulkanRHI
{
	class VulkanDevice;

	VkBufferUsageFlags RHIToVKBufferUsageFlags(VulkanDevice* InDevice, BufferUsageFlags InUEUsage, bool bZeroSize);


	class ElaineCoreExport VulkanBuffer : public RHIBuffer
	{
	public:
		VulkanBuffer(VulkanDevice* InDevice, size_t InSize, BufferUsageFlags InUsage, ERHIAccess InResourceState, size_t InStride);
		~VulkanBuffer();
		void MapMemoryDataToBuffer(void* InData,size_t InSize);
		inline VkBuffer GetHandle() const
		{
			return mBufferHandle;
		}

		inline size_t GetOffset() const
		{
			return mBufferOffset;
		}

		inline size_t GetBufferSize() const
		{
			return mBufferSize;
		}

		inline VkBufferUsageFlags GetBufferUsage() const
		{
			return mBufferUsageFlags;
		}

		inline bool IsEffective() const
		{
			return mBufferHandle != nullptr;
		}

		VulkanAllocation& GetBufferAllocation() { return mBuffer; }

	private:
		VkBuffer			mBufferHandle = VK_NULL_HANDLE;
		VkBufferUsageFlags	mBufferUsageFlags;
		VulkanAllocation	mBuffer;
		size_t				mBufferSize;
		size_t				mBufferOffset = 0;
		size_t				mBufferStride;
		VulkanDevice*		mDevice;
	};
}