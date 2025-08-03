#pragma once
#include "Common/ElaineRHIProtocol.h"
#include "vulkan/ElaineVulkanMemory.h"

namespace VulkanRHI
{
	class ElaineCoreExport VulkanUniformBuffer :public RHIUniformBuffer
	{
	public:
		VulkanUniformBuffer(VulkanDevice* InDevice, size_t InSize, ERHIAccess InResourceState, void* InContent);
		virtual ~VulkanUniformBuffer();

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

		inline bool IsEffective() const
		{
			return mBufferHandle != nullptr;
		}

		VulkanAllocation& GetBufferAllocation() { return mBuffer; }

		void UpdateBuffer(const void* InContent, size_t InOffset = 0u, size_t InSize = 0u);

	private:
		VkBuffer			mBufferHandle = VK_NULL_HANDLE;
		VulkanAllocation	mBuffer;
		size_t				mBufferSize;
		size_t				mBufferOffset = 0;
		VulkanDevice*		mDevice;
	};
}