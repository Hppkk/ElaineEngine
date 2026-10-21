#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanBuffer.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
	VkBufferUsageFlags RHIToVKBufferUsageFlags(VulkanDevice* InDevice, BufferUsageFlags InUsage, bool bZeroSize)
	{
		// Always include TRANSFER_SRC since hardware vendors confirmed it wouldn't have any performance cost and we need it for some debug functionalities.
		VkBufferUsageFlags OutVkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		auto TranslateFlag = [&OutVkUsage, &InUsage](BufferUsageFlags SearchUEFlag, VkBufferUsageFlags AddedIfFound, VkBufferUsageFlags AddedIfNotFound = 0)
			{
				const bool HasFlag = EnumHasAnyFlags(InUsage, SearchUEFlag);
				OutVkUsage |= HasFlag ? AddedIfFound : AddedIfNotFound;
			};

		TranslateFlag(BufferUsageFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::StructuredBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::AccelerationStructure, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
		TranslateFlag(BufferUsageFlags::UniformBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		if (!bZeroSize)
		{
			TranslateFlag(BufferUsageFlags::UnorderedAccess, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
			TranslateFlag(BufferUsageFlags::DrawIndirect, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
			TranslateFlag(BufferUsageFlags::KeepCPUAccessible, (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT));
			TranslateFlag(BufferUsageFlags::ShaderResource, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);

			TranslateFlag(BufferUsageFlags::Volatile, 0, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
			TranslateFlag(BufferUsageFlags::Static, VK_BUFFER_USAGE_TRANSFER_DST_BIT);


			//if (InDevice->GetOptionalExtensions().HasRaytracingExtensions())
			//{
			//	OutVkUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

			//	TranslateFlag(BufferUsageFlags::AccelerationStructure, 0, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
			//}
		}

		return OutVkUsage;
	}
	VulkanBuffer::VulkanBuffer(VulkanDevice* InDevice, size_t InSize, BufferUsageFlags InUsage, ERHIAccess InResourceState, size_t InStride)
		: RHIBuffer({static_cast<uint32>(InSize), static_cast<uint32>(InStride), InUsage, InResourceState})
        , mBufferSize(InSize)
		, mBufferStride(InStride)
		, mDevice(InDevice)
	{
		mBufferUsageFlags = RHIToVKBufferUsageFlags(InDevice, InUsage, InSize == 0);

		if (InSize != 0)
		{
			const uint32 BufferAlignment = VulkanMemoryManager::CalculateBufferAlignment(*InDevice, InUsage, InSize == 0);
			VkMemoryPropertyFlags BufferMemFlags = EnumHasAnyFlags(InUsage, BufferUsageFlags::KeepCPUAccessible)
                ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			if (EnumHasAnyFlags(InUsage , BufferUsageFlags::UniformBuffer))
			{
				if (!InDevice->GetMemoryManager()->AllocUniformBuffer(mBuffer, InSize, nullptr))
				{
					LOG_FATAL("VulkanRHI: Allocate Uniform Buffer Failed.");
				}
			}
			else
			{
				if (!InDevice->GetMemoryManager()->AllocateBufferPooled(mBuffer, nullptr, InSize, BufferAlignment, mBufferUsageFlags, BufferMemFlags, VulkanAllocationMetaMultiBuffer, __FILE__, __LINE__))
				{
					LOG_FATAL("VulkanRHI: Out of Memory.");
				}
			}

			mBufferHandle = (VkBuffer)mBuffer.mVulkanHandle;
			mBufferOffset = mBuffer.mOffset;

		}
	}

	VulkanBuffer::~VulkanBuffer()
	{
		mDevice->GetMemoryManager()->FreeVulkanAllocation(mBuffer);
	}

    void VulkanBuffer::MapMemoryDataToBuffer(void* Data, size_t Size)
    {
        if (!Data || !Size) return;
        if (Size > mBufferSize) throw std::out_of_range("Staging write exceeds buffer size");
        Memory::MemoryCopy(mBuffer.GetMappedPointer(mDevice), Data, Size);
        mBuffer.FlushMappedMemory(mDevice);
    }

}
