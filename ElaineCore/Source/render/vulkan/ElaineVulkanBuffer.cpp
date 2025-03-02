#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanBuffer.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
	VkBufferUsageFlags RHIToVKBufferUsageFlags(VulkanDevice* InDevice, BufferUsageFlags InUEUsage, bool bZeroSize)
	{
		// Always include TRANSFER_SRC since hardware vendors confirmed it wouldn't have any performance cost and we need it for some debug functionalities.
		VkBufferUsageFlags OutVkUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		auto TranslateFlag = [&OutVkUsage, &InUEUsage](BufferUsageFlags SearchUEFlag, VkBufferUsageFlags AddedIfFound, VkBufferUsageFlags AddedIfNotFound = 0)
			{
				const bool HasFlag = EnumHasAnyFlags(InUEUsage, SearchUEFlag);
				OutVkUsage |= HasFlag ? AddedIfFound : AddedIfNotFound;
			};

		TranslateFlag(BufferUsageFlags::VertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::IndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::StructuredBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		TranslateFlag(BufferUsageFlags::AccelerationStructure, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);


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
		: mBufferSize(InSize)
		, mBufferStride(InStride)
		, mDevice(InDevice)
	{
		mBufferUsageFlags = RHIToVKBufferUsageFlags(InDevice, InUsage, InSize == 0);

		if (InSize != 0)
		{
			const bool BufferAlignment = VulkanMemoryManager::CalculateBufferAlignment(*InDevice, InUsage, InSize == 0);
			VkMemoryPropertyFlags BufferMemFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			BufferMemFlags |= (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (!InDevice->GetMemoryManager()->AllocateBufferPooled(mBuffer, nullptr, InSize, BufferAlignment, mBufferUsageFlags, BufferMemFlags, VulkanAllocationMetaMultiBuffer, __FILE__, __LINE__))
			{
				LOG_FATAL("VulkanRHI: Out of Memory");
			}

			mBufferHandle = (VkBuffer)mBuffer.mVulkanHandle;
			mBufferOffset = mBuffer.mOffset;

		}
	}

	VulkanBuffer::~VulkanBuffer()
	{
		mDevice->GetMemoryManager()->FreeVulkanAllocation(mBuffer);
	}

	void VulkanBuffer::MapMemoryDataToBuffer(void* InData, size_t InSize)
	{
		VulkanStagingBuffer* StageBuffer = mDevice->GetStagingManager()->AcquireBuffer(InSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		if (StageBuffer)
		{
			Memory::MemoryCopy(StageBuffer->GetMappingPointer(), InData, InSize);
			StageBuffer->UnMap();
			VulkanCommandBufferManager* CmdBufferManager = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultTransferContext())->GetCommandBufferManager();
			VulkanCommandBuffer* CmdBuffer = CmdBufferManager->GetUploadCmdBuffer();
			if (CmdBuffer)
			{
				CmdBuffer->CopyBuffer(StageBuffer->GetHandle(), mBufferHandle, InSize, StageBuffer->GetBufferOffset(), mBufferOffset);
				CmdBuffer->End();
				CmdBufferManager->SubmitUploadCmdBuffer();
			}
		}
	}
}