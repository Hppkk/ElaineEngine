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

	VulkanViewport::VulkanViewport(VulkanDevice* InDevice, void* InWindowHandle, uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen, PixelFormat InFormat)
	{
		mDevice = InDevice;
		mWindowHandle = InWindowHandle;
		mSizeX = InSizeX;
		mSizeY = InSizeY;
		mbInIsFullscreen = bInIsFullscreen;
		mPixelFormat = InFormat;
		CreateSwapchain(nullptr);
		CreateFrameBuffer();
		for (int32 Index = 0, NumBuffers = mSemaphores.size(); Index < NumBuffers; ++Index)
		{
			mSemaphores[Index] = new VulkanSemaphore(InDevice);
		}
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
		for (int32 Index = 0, NumBuffers = mSemaphores.size(); Index < NumBuffers; ++Index)
		{
			SAFE_DELETE(mSemaphores[Index]);
			mBackBufferImages[Index] = VK_NULL_HANDLE;
		}
		mSwapChain->Destroy();
		SAFE_DELETE(mSwapChain);
	}

	void VulkanViewport::CreateSwapchain(VulkanSwapChain* InOldSwapChain)
	{
		RHITextureDesc DepthTextureDesc;
		DepthTextureDesc.mArraySize = 1;
		DepthTextureDesc.mDepth = 0;
		DepthTextureDesc.mDimension = TextureDimension::Texture2D;
		DepthTextureDesc.mExtent = Vector2(mSizeX, mSizeY);
		DepthTextureDesc.mFlags = TextureCreateFlags::DepthStencilTargetable;
		DepthTextureDesc.mFormat = PF_DepthStencil;
		DepthTextureDesc.mNumMips = 1;
		DepthTextureDesc.mNumSamples = 1;
		DepthTextureDesc.mUAVFormat = PF_Unknown;
		mDepthBuffer = new VulkanTexture(mDevice, DepthTextureDesc);
		mDepthBufferView = new VulkanTextureView();
		mDepthBufferView->Create(*mDevice, mDepthBuffer->getHandle(), VK_IMAGE_VIEW_TYPE_2D,
			VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
			PF_DepthStencil, EngineToVkTextureFormat(PF_DepthStencil, false), 0, 1, 0, 1, false, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);



		mSwapChain = new VulkanSwapChain(mDevice, mSizeX, mSizeY, mbInIsFullscreen, InOldSwapChain, mPixelFormat);
		std::vector<VkImage> Images;
		mSwapChain->GetSwapChainImages(Images);
		mBackBufferImages.resize(Images.size());
		mSemaphores.resize(Images.size());
		mTextureViews.resize(Images.size());
		VulkanCommandContext* CurrentCmdCtx = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
		VulkanCommandBuffer* CurrentCmdBuffer = CurrentCmdCtx->GetCommandBufferManager()->GetUploadCmdBuffer();
		for (int32 Index = 0; Index < Images.size(); ++Index)
		{
			mBackBufferImages[Index] = Images[Index];
			mTextureViews[Index] = new VulkanTextureView();
			mTextureViews[Index]->Create(*mDevice, Images[Index], VK_IMAGE_VIEW_TYPE_2D, VK_IMAGE_ASPECT_COLOR_BIT,
				mPixelFormat, EngineToVkTextureFormat(mPixelFormat, true), 0, 1, 0, 1, false, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
			VkImageSubresourceRange Range = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT);
			VkClearColorValue ClearColor;
			Elaine::Memory::MemoryZero(ClearColor);
			VulkanSetImageLayout(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Range);
			vkCmdClearColorImage(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &ClearColor, 1, &Range);
			VulkanSetImageLayout(CurrentCmdBuffer->GetHandle(), Images[Index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, Range);
		}
		CurrentCmdCtx->GetCommandBufferManager()->SubmitUploadCmdBuffer();

	}
	void VulkanViewport::DestroySwapchain()
	{
		GetVulkanDynamicRHI()->SubmitAllCommands();
		GetVulkanDynamicRHI()->WaitUntilIdle();

		if (mSwapChain)
		{
			for (int Index = 0, NumBuffers = mBackBufferImages.size(); Index < NumBuffers; ++Index)
			{
				mTextureViews[Index]->Destroy(*mDevice);
				mBackBufferImages[Index] = VK_NULL_HANDLE;
			}
			mDevice->GetDeferredDeletionQueue().ReleaseResources(true);
			mSwapChain->Destroy();
			SAFE_DELETE(mSwapChain);
			mDevice->GetDeferredDeletionQueue().ReleaseResources(true);
		}

	}

	bool VulkanViewport::AcquireImageIndex()
	{
		if (mSwapChain)
		{
			mAcquiredImageIndex = mSwapChain->AcquireImageIndex(&mAcquiredSemaphore);
			return true;
		}
		return false;
	}

	void VulkanViewport::CreateFrameBuffer()
	{
		mFrameBuffers.resize(mSwapChain->GetNumSwapChainImages());
		for (size_t Index = 0; Index < mFrameBuffers.size(); ++Index)
		{
			VkFramebuffer NewFrameBuffer;
			VkFramebufferCreateInfo FramebufferCreateInfo;
			std::array<VkImageView, 2> TempAttachents{ mTextureViews[Index]->mView,mDepthBufferView->mView };
			Memory::MemoryZero(FramebufferCreateInfo);
			FramebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			FramebufferCreateInfo.height = mSizeY;
			FramebufferCreateInfo.width = mSizeX;
			FramebufferCreateInfo.attachmentCount = 2;
			FramebufferCreateInfo.pAttachments = TempAttachents.data();
			FramebufferCreateInfo.layers = 1;
			FramebufferCreateInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
			vkCreateFramebuffer(mDevice->GetDevice(), &FramebufferCreateInfo, VULKAN_CPU_ALLOCATOR, &NewFrameBuffer);
			mFrameBuffers[Index] = NewFrameBuffer;
		}
	}

	void VulkanViewport::RecreateFrameBuffer()
	{
	}

	const VkSemaphore& VulkanViewport::GetIndexVkSemaphore(size_t InIndex)
	{
		if (InIndex >= mSemaphores.size())
			return G_NULL_SEMAPHORE;

		return mSemaphores[InIndex]->GetHandle();
	}

	void VulkanViewport::RecreateSwapchain()
	{
		std::lock_guard<std::mutex> LockGuard(mMtx);
		CreateSwapchain(mSwapChain);
		CreateFrameBuffer();
		DestroySwapchain();
	}

	void VulkanViewport::RecreateSwapchainFromRT(PixelFormat InPixelFormat)
	{
		mPixelFormat = InPixelFormat;
		RecreateSwapchain();
	}

	void VulkanViewport::Resize(uint32 InSizeX, uint32 InSizeY, bool bIsFullscreen, PixelFormat InPixelFormat)
	{
		mSizeX = InSizeX;
		mSizeY = InSizeY;
		bIsFullscreen = mbInIsFullscreen;

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

		RecreateSwapchainFromRT(InPixelFormat);
	}
	VkFramebuffer VulkanViewport::GetIndexFrameBuffer(uint32 InIndex)
	{
		return mFrameBuffers[InIndex];
	}
}