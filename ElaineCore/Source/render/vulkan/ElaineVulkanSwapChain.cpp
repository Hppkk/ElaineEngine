#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanSwapChain.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanQueue.h"

namespace VulkanRHI
{
	VulkanSwapChain::VulkanSwapChain(VulkanDevice* InDevice, uint32 Width, uint32 Height, bool bIsFullscreen, VulkanSwapChain* InSwapChain, PixelFormat InFormat)
		:mFormat(InFormat)
	{
		mInternalHeight = Height;
		mInternalWidth = Width;
		mDevice = InDevice;
		mbInternalFullScreen = bIsFullscreen;

		uint32 NumFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &NumFormats, nullptr);

		std::vector<VkSurfaceFormatKHR> Formats(NumFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &NumFormats, Formats.data());

		VkColorSpaceKHR RequestedColorSpace = Formats[2].colorSpace;
		RequestedColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkSurfaceFormatKHR CurrFormat = Formats[0];
		VkFormat VkPixelFt = EngineToVkTextureFormat(InFormat, true);
		for(uint32 Index = 0;Index<NumFormats;++Index)
		{
			if (VkPixelFt == Formats[Index].format)
			{
				CurrFormat = Formats[Index];
				break;
			}
		}
		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceCapabilitiesKHR SurfProperties;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(),
			GetVulkanDynamicRHI()->GetVulkanSurface(),
			&SurfProperties);
		VkSurfaceTransformFlagBitsKHR PreTransform;
		if (SurfProperties.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
		{
			PreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		}
		else
		{
			PreTransform = SurfProperties.currentTransform;
		}

		VkCompositeAlphaFlagBitsKHR CompositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
		if (SurfProperties.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
		{
			CompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		}

		uint32 DesiredNumBuffers = Elaine::Math::min(SurfProperties.maxImageCount, 2u);

		VkSwapchainCreateInfoKHR SwapChainInfo;
		Elaine::Memory::MemoryZero(SwapChainInfo);
		SwapChainInfo.surface = GetVulkanDynamicRHI()->GetVulkanSurface();
		SwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapChainInfo.minImageCount = DesiredNumBuffers;
		SwapChainInfo.imageFormat = CurrFormat.format;
		SwapChainInfo.imageColorSpace = CurrFormat.colorSpace;
		SwapChainInfo.imageExtent.width = Width;
		SwapChainInfo.imageExtent.height = Height;
		SwapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		SwapChainInfo.preTransform = PreTransform;
		SwapChainInfo.imageArrayLayers = 1;
		SwapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		SwapChainInfo.presentMode = PresentMode;
		SwapChainInfo.oldSwapchain = VK_NULL_HANDLE;
		if (InSwapChain != nullptr)
		{
			SwapChainInfo.oldSwapchain = InSwapChain->GetSwapChain();
		}
		SwapChainInfo.clipped = VK_TRUE;
		SwapChainInfo.compositeAlpha = CompositeAlpha;

		vkCreateSwapchainKHR(mDevice->GetDevice(), &SwapChainInfo, VULKAN_CPU_ALLOCATOR, &mSwapChainHandle);
		mImageFormat = CurrFormat.format;
		//if (InSwapChain != nullptr)
		//{
		//	vkDestroySwapchainKHR(mDevice->GetDevice(), InSwapChain->GetSwapChain(), VULKAN_CPU_ALLOCATOR);
		//	SAFE_DELETE(InSwapChain);
		//}

		vkGetSwapchainImagesKHR(mDevice->GetDevice(), mSwapChainHandle, &mNumSwapChainImages, nullptr);
		mImageAcquiredFences.resize(mNumSwapChainImages);
		mImageAcquiredSemaphore.resize(mNumSwapChainImages);
		VulkanFenceManager* FenceMgr = mDevice->GetFenceManager();
		for (uint32 BufferIndex = 0; BufferIndex < mNumSwapChainImages; ++BufferIndex)
		{
			mImageAcquiredFences[BufferIndex] = mDevice->GetFenceManager()->AllocateFence(true);
			mImageAcquiredSemaphore[BufferIndex] = new VulkanSemaphore(mDevice);
		}
	}

	void VulkanSwapChain::Present(VulkanQueue* InGfxQueue, VulkanSemaphore* InSemaphore)
	{
		VkPresentInfoKHR PresentInfo;
		Elaine::Memory::MemoryZero(PresentInfo);
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		if (InSemaphore != nullptr)
		{
			VkSemaphore Semaphore = InSemaphore->GetHandle();
			PresentInfo.waitSemaphoreCount = 1;
			PresentInfo.pWaitSemaphores = &Semaphore;
		}
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &mSwapChainHandle;
		PresentInfo.pImageIndices = (uint32*)&mCurrentImageIndex;
		vkQueuePresentKHR(InGfxQueue->GetHandle(), &PresentInfo);
	}

	int32 VulkanSwapChain::AcquireImageIndex(VulkanSemaphore** OutSemaphore)
	{
		uint32 ImageIndex = 0;
		const int32 PrevSemaphoreIndex = mSemaphoreIndex;
		mSemaphoreIndex = (mSemaphoreIndex + 1) % mImageAcquiredSemaphore.size();
		VulkanFenceManager* FenceMgr = mDevice->GetFenceManager();
		FenceMgr->ResetFence(mImageAcquiredFences[mSemaphoreIndex]);
		const VkFence AcquiredFence = mImageAcquiredFences[mSemaphoreIndex]->GetHandle();
		vkAcquireNextImageKHR(mDevice->GetDevice(), mSwapChainHandle, UINT64_MAX, mImageAcquiredSemaphore[mSemaphoreIndex]->GetHandle(), AcquiredFence, &ImageIndex);
		FenceMgr->WaitForFence(mImageAcquiredFences[mSemaphoreIndex], UINT64_MAX);
		*OutSemaphore = mImageAcquiredSemaphore[mSemaphoreIndex];
		mCurrentImageIndex = (int32)ImageIndex;
		return mCurrentImageIndex;
	}

	void VulkanSwapChain::GetSwapChainImages(std::vector<VkImage>& InOutImages)
	{
		InOutImages.resize(mNumSwapChainImages);
		vkGetSwapchainImagesKHR(mDevice->GetDevice(), mSwapChainHandle, &mNumSwapChainImages, InOutImages.data());
	}

	const VkSemaphore& VulkanSwapChain::GetIndexVkSemaphore(size_t InIndex)
	{
		return mImageAcquiredSemaphore[InIndex]->GetHandle();
	}

	void VulkanSwapChain::Destroy()
	{
		vkDestroySwapchainKHR(mDevice->GetDevice(), mSwapChainHandle, VULKAN_CPU_ALLOCATOR);
		mSwapChainHandle = nullptr;
		VulkanFenceManager* FenceMgr = mDevice->GetFenceManager();
		for (uint32 BufferIndex = 0; BufferIndex < mImageAcquiredFences.size(); ++BufferIndex)
		{
			FenceMgr->ReleaseFence(mImageAcquiredFences[BufferIndex]);
			//todo
			SAFE_DELETE(mImageAcquiredSemaphore[BufferIndex]);
		}
	}
}