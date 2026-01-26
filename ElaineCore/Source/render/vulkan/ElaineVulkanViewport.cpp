#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanViewport.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanSwapChain.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanBarrier.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"
#include "render/vulkan/ElaineVulkanRenderPass.h"


namespace VulkanRHI
{
	static VkSemaphore G_NULL_SEMAPHORE = nullptr;

	VulkanViewport::VulkanViewport(VulkanDevice* InDevice, uint32 InSizeX, uint32 InSizeY)
	{
		mDevice = InDevice;
		mSizeX = InSizeX;
		mSizeY = InSizeY;
		mVkViewPort.x = 0;
		mVkViewPort.y = 0;
		mVkViewPort.height = InSizeY;
		mVkViewPort.width = InSizeX;
		mVkViewPort.minDepth = 0;
		mVkViewPort.maxDepth = 1;
		mScissor.extent.height = InSizeY;
		mScissor.extent.width = InSizeX;
		mScissor.offset.x = 0;
		mScissor.offset.y = 0;
	}

	VulkanViewport::~VulkanViewport()
	{

	}

	void VulkanViewport::Resize(uint32 InSizeX, uint32 InSizeY)
	{
		mSizeX = InSizeX;
		mSizeY = InSizeY;

		mVkViewPort.x = 0;
		mVkViewPort.y = 0;
		mVkViewPort.height = InSizeY;
		mVkViewPort.width = InSizeX;
		mVkViewPort.minDepth = 0;
		mVkViewPort.maxDepth = 1;
		mScissor.extent.height = InSizeY;
		mScissor.extent.width = InSizeX;
		mScissor.offset.x = 0;
		mScissor.offset.y = 0;
	}

	void VulkanViewport::SetSize(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
	{
		mVkViewPort.x = MinX;
		mVkViewPort.y = MinY;
		mVkViewPort.height = MaxY;
		mVkViewPort.width = MaxX;
		mVkViewPort.minDepth = MinZ;
		mVkViewPort.maxDepth = MaxZ;
	}
}