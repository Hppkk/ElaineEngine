#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanSwapChain.h"
#include "vulkan/ElaineVulkanPhysicalDevice.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanMemory.h"
#include "vulkan/ElaineVulkanQueue.h"
#include "vulkan/ElaineVulkanCommandContext.h"
#include "vulkan/ElaineVulkanTexture.h"
#include "vulkan/ElaineVulkanBarrier.h"
#include "vulkan/ElaineVulkanCommandBuffer.h"

namespace VulkanRHI
{
	VulkanSwapChain::VulkanSwapChain(VulkanDevice* InDevice, uint32 Width, uint32 Height, bool bIsFullscreen, VulkanSwapChain* InSwapChain, PixelFormat InFormat)
		:mFormat(InFormat)
	{
		mInternalHeight = Height;
		mInternalWidth = Width;
		mDevice = InDevice;
		mbInternalFullScreen = bIsFullscreen;

		CreateSwapchain(nullptr);
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

	bool VulkanSwapChain::AcquireImage(VulkanSemaphore* InSemaphore, uint32& OutImageIndex)
	{
		if (InSemaphore == nullptr)
		{
			LOG_FATAL("AcquireImage failed!");
			assert(false);
		}

		VkResult EResult = vkAcquireNextImageKHR(mDevice->GetDevice(), mSwapChainHandle, UINT64_MAX, InSemaphore->GetHandle(), VK_NULL_HANDLE, &OutImageIndex);
		if (EResult == VK_ERROR_OUT_OF_DATE_KHR || EResult == VK_SUBOPTIMAL_KHR)
		{
			return false;
		}
		mCurrentImageIndex = (int32)OutImageIndex;
		return true;
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

	void VulkanSwapChain::CreateTextureView()
	{
		std::vector<VkImage> Images;
		GetSwapChainImages(Images);
		mBackBufferImages.resize(Images.size());
		mBackBufferTextures.resize(Images.size());
		VulkanCommandContext* CurrentCmdCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
		VulkanCommandBuffer* CurrentCmdBuffer = CurrentCmdCtx->GetCommandBufferManager()->GetUploadCmdBuffer();
		for (int32 Index = 0; Index < Images.size(); ++Index)
		{
			mBackBufferImages[Index] = Images[Index];
			VkImageSubresourceRange Range = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1);
			VkClearColorValue ClearColor;
			Elaine::Memory::MemoryZero(ClearColor);
			VulkanSetImageLayout(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Range);
			vkCmdClearColorImage(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearColor, 1, &Range);
			VulkanSetImageLayout(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, Range);
			
			RHITextureDesc BackBufferDesc;
			BackBufferDesc.mArraySize = 1;
			BackBufferDesc.mDepth = 0;
			BackBufferDesc.mDimension = TextureDimension::Texture2D;
			BackBufferDesc.mExtent = Vector2(mInternalWidth, mInternalHeight);
			BackBufferDesc.mFlags = TextureCreateFlags::RenderTargetable | TextureCreateFlags::SRGB;
			BackBufferDesc.mFormat = mFormat;
			BackBufferDesc.mNumMips = 1;
			BackBufferDesc.mNumSamples = 1;
			BackBufferDesc.mUAVFormat = PF_Unknown;
			BackBufferDesc.isSRGB = true;
			
			VulkanTexture* BackBufferTex = new VulkanTexture(mDevice, BackBufferDesc, Images[Index]);
			BackBufferTex->SetAccess(ERHIAccess::Present);
			BackBufferTex->SetImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);  // 同步 mImageLayout
			BackBufferTex->SetIsSwapchainImage(true);
			mBackBufferTextures[Index] = BackBufferTex;
		}

		if (mBackBufferProxy == nullptr && !mBackBufferTextures.empty())
		{
			// Initialize Proxy with the first image's description and handle (handle is just a placeholder)
			mBackBufferProxy = new VulkanTexture(mDevice, mBackBufferTextures[0]->GetDesc(), mBackBufferTextures[0]->getHandle());
			mBackBufferProxy->SetProxy(true, this);
			mBackBufferProxy->SetIsSwapchainImage(true);
			mBackBufferProxy->SetAccess(ERHIAccess::Present);
			mBackBufferProxy->SetImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
		else if(mBackBufferProxy != nullptr && !mBackBufferTextures.empty())
		{
			// Update Proxy if it already exists (e.g. swapchain resize)
			// Note: We might need to recreate it if Desc changes significantly, but usually it's just resolution.
			// Ideally we should destroy and recreate, or update. For now let's assume simple recreate if null, 
			// but if it exists we might want to update its internal handle or view if necessary.
			// Since it's a proxy, the handle doesn't matter much for functionality (it's resolved later),
			// but view properties might. 
			// Let's just delete and recreate to be safe and consistent.
			delete mBackBufferProxy;
			mBackBufferProxy = new VulkanTexture(mDevice, mBackBufferTextures[0]->GetDesc(), mBackBufferTextures[0]->getHandle());
			mBackBufferProxy->SetProxy(true, this);
			mBackBufferProxy->SetIsSwapchainImage(true);
			mBackBufferProxy->SetAccess(ERHIAccess::Present);
			mBackBufferProxy->SetImageLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
		}
		CurrentCmdBuffer->End();
		CurrentCmdCtx->GetCommandBufferManager()->SubmitUploadCmdBuffer();
	}

	void VulkanSwapChain::CreateSwapchain(VulkanSwapChain* InSwapChain)
	{
		uint32 NumFormats = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &NumFormats, nullptr);

		std::vector<VkSurfaceFormatKHR> Formats(NumFormats);
		vkGetPhysicalDeviceSurfaceFormatsKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &NumFormats, Formats.data());

		VkFormat RequestedFormat = EngineToVkTextureFormat(mFormat, true);
		VkColorSpaceKHR RequestedColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		VkSurfaceFormatKHR CurrFormat = Formats[0];
		
		bool bFound = false;

		for (const auto& AvailableFormat : Formats)
		{
			if (AvailableFormat.format == RequestedFormat && AvailableFormat.colorSpace == RequestedColorSpace)
			{
				CurrFormat = AvailableFormat;
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			for (const auto& AvailableFormat : Formats)
			{
				if (AvailableFormat.format == RequestedFormat)
				{
					CurrFormat = AvailableFormat;
					bFound = true;
					break;
				}
			}
		}

		if (!bFound && NumFormats > 0)
		{
			CurrFormat = Formats[0];
			LOG_WARN("Requested Swapchain format not found, falling back to {}", CurrFormat.format);
		}
		// Prioritize Mailbox, fallback to FIFO (always supported)
		VkPresentModeKHR PresentMode = VK_PRESENT_MODE_FIFO_KHR;
		uint32 PresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &PresentModeCount, nullptr);
		std::vector<VkPresentModeKHR> AvailablePresentModes(PresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(GetVulkanDynamicRHI()->GetPhyDevice()->GetPhysicalDeviceHandle(), GetVulkanDynamicRHI()->GetVulkanSurface(), &PresentModeCount, AvailablePresentModes.data());

		bool bMailboxSupported = false;
		for (const auto& AvailablePresentMode : AvailablePresentModes)
		{
			if (AvailablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				bMailboxSupported = true;
				break;
			}
		}

		LOG_INFO("VulkanSwapChain: Selected Present Mode: {} (Mailbox Supported: {})", PresentMode, bMailboxSupported);

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

		uint32 DesiredNumBuffers = Elaine::Math::min(SurfProperties.maxImageCount, (uint32)MAX_FRAMES_IN_FLIGHT);
		if (DesiredNumBuffers < SurfProperties.minImageCount)
		{
			DesiredNumBuffers = SurfProperties.minImageCount;
		}
		// logic above might result in 0 if maxImageCount is 0 (unlimited), handle that
		if (SurfProperties.maxImageCount > 0 && DesiredNumBuffers > SurfProperties.maxImageCount)
		{
			DesiredNumBuffers = SurfProperties.maxImageCount;
		}
		
		VkSwapchainCreateInfoKHR SwapChainInfo;
		Elaine::Memory::MemoryZero(SwapChainInfo);
		SwapChainInfo.surface = GetVulkanDynamicRHI()->GetVulkanSurface();
		SwapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		SwapChainInfo.minImageCount = DesiredNumBuffers;
		SwapChainInfo.imageFormat = CurrFormat.format;
		SwapChainInfo.imageColorSpace = CurrFormat.colorSpace;
		SwapChainInfo.imageExtent.width = mInternalWidth;
		SwapChainInfo.imageExtent.height = mInternalHeight;
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

		VkResult CreateResult = vkCreateSwapchainKHR(mDevice->GetDevice(), &SwapChainInfo, VULKAN_CPU_ALLOCATOR, &mSwapChainHandle);
		if (CreateResult != VK_SUCCESS)
		{
			LOG_ERROR("VulkanSwapChain: Failed to create swapchain! Error Code: %d", CreateResult);
			return;
		}
		
		LOG_INFO("VulkanSwapChain: Created Swapchain. Size: {}x{}, Format: {}, ImageCount: {}", 
			mInternalWidth, mInternalHeight, CurrFormat.format, DesiredNumBuffers);
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
		CreateTextureView();
	}

	void VulkanSwapChain::DestroySwapchain()
	{
		GetVulkanDynamicRHI()->SubmitAllCommands();
		GetVulkanDynamicRHI()->WaitUntilIdle();

		for (int Index = 0, NumBuffers = mBackBufferImages.size(); Index < NumBuffers; ++Index)
		{
			if (mBackBufferTextures[Index])
			{
				delete mBackBufferTextures[Index];
				mBackBufferTextures[Index] = nullptr;
			}
			mBackBufferImages[Index] = VK_NULL_HANDLE;
		}

		if (mBackBufferProxy)
		{
			delete mBackBufferProxy;
			mBackBufferProxy = nullptr;
		}

		mDevice->GetDeferredDeletionQueue().ReleaseResources(true);
		Destroy();
		mDevice->GetDeferredDeletionQueue().ReleaseResources(true);

	}

	void VulkanSwapChain::GetSize(uint32& OutWidth, uint32& OutHeight)
	{
		OutWidth = mInternalWidth;
		OutHeight = mInternalHeight;
	}

	void VulkanSwapChain::RecreateSwapchain()
	{
		std::lock_guard<std::mutex> LockGuard(mMtx);
		CreateSwapchain(this);
		//DestroySwapchain();
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

	RHITexture* VulkanSwapChain::AcquireNextTexture()
	{
		return mBackBufferProxy;
	}

	RHITexture* VulkanSwapChain::AcquireAndGetBackBuffer(void** OutSemaphore)
	{
		std::lock_guard<std::mutex> Lock(mMtx);
		
		mSemaphoreIndex = (mSemaphoreIndex + 1) % mImageAcquiredSemaphore.size();
		uint32 ImageIndex = 0;
		VkResult Result = 
			vkAcquireNextImageKHR(
			mDevice->GetDevice(),
			mSwapChainHandle,
			UINT64_MAX,
			mImageAcquiredSemaphore[mSemaphoreIndex]->GetHandle(),
			VK_NULL_HANDLE,
			&ImageIndex);
		if (Result == VK_SUCCESS || Result == VK_SUBOPTIMAL_KHR)
		{
			if (Result == VK_SUBOPTIMAL_KHR)
			{
				LOG_WARN("VulkanSwapChain: AcquireNextImage returned VK_SUBOPTIMAL_KHR. Swapchain might need recreation soon.");
			}

			if (OutSemaphore)
			{
				*OutSemaphore = mImageAcquiredSemaphore[mSemaphoreIndex];
			}
			
			if (mCurrentImageIndex >= 0 && mCurrentImageIndex < mBackBufferTextures.size())
			{
				return mBackBufferProxy;
			}
		}
		else if (Result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			LOG_WARN("VulkanSwapChain: Swapchain out of date (VK_ERROR_OUT_OF_DATE_KHR), needs recreation. Window size might have changed.");
		}
		else
		{
			LOG_ERROR("VulkanSwapChain: AcquireNextImage failed with error code: {}", Result);
		}
		
		return nullptr;
	}

	void VulkanSwapChain::Resize(uint32 InWidth, uint32 InHeight)
	{

	}

	VulkanTexture* VulkanSwapChain::GetActiveBackBufferTexture()
	{
		if (mCurrentImageIndex >= 0 && mCurrentImageIndex < mBackBufferTextures.size())
		{
			return mBackBufferTextures[mCurrentImageIndex];
		}
		return nullptr;
	}

	RHITexture* VulkanSwapChain::GetBackBufferProxy()
	{
		return mBackBufferProxy;
	}
}
