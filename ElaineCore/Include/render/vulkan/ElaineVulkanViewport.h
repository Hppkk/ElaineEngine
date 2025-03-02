#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"
#include "ElaineCorePrerequirements.h"

namespace VulkanRHI
{
	class VulkanSwapChain;
	class VulkanTextureView;
	class VulkanSemaphore;
	class VulkanDevice;
	class ElaineCoreExport VulkanViewport :public Elaine::RHIViewport
	{
	public:
		VulkanViewport(VulkanDevice* InDevice, void* InWindowHandle, uint32 InSizeX, uint32 InSizeY, bool bInIsFullscreen, PixelFormat InFormat);
		virtual ~VulkanViewport();
		void GetViewSize(uint32& InOutSizeX, uint32& InOutSizeY) const { InOutSizeX = mSizeX; InOutSizeY = mSizeY; }
		VulkanSwapChain* GetSwapChain() const { return mSwapChain; }
		void CreateSwapchain(VulkanSwapChain* InOldSwapChain);
		void DestroySwapchain();
		bool AcquireImageIndex();
		void CreateFrameBuffer();
		void RecreateFrameBuffer();
		const VkSemaphore& GetIndexVkSemaphore(size_t InIndex);
		VulkanSemaphore* GetIndexSemaphore(size_t InIndex);
		void RecreateSwapchain();
		void RecreateSwapchainFromRT(PixelFormat InPixelFormat);
		void Resize(uint32 InSizeX, uint32 InSizeY, bool bIsFullscreen, PixelFormat InPixelFormat);
		const VkViewport& GetDefaultViewPort() const { return mVkViewPort; }
		const VkRect2D& GetDefaultScissor() const { return mScissor; }
		VkFramebuffer GetIndexFrameBuffer(uint32 InIndex);
	private:
		uint32 mSizeX = 0;
		uint32 mSizeY = 0;
		void* mWindowHandle = nullptr;
		bool mbInIsFullscreen = false;
		PixelFormat mPixelFormat = PixelFormat::PF_FloatRGBA;
		VulkanDevice* mDevice = nullptr;
		VulkanSwapChain* mSwapChain = nullptr;
		VulkanSemaphore* mAcquiredSemaphore = nullptr;
		std::mutex mMtx;
		int mAcquiredImageIndex = 0;
		std::vector<VkImage> mBackBufferImages;
		std::vector<VulkanSemaphore*> mSemaphores;
		std::vector<VulkanTextureView*> mTextureViews;
		VkViewport mVkViewPort;
		std::vector <VkFramebuffer> mFrameBuffers;
		//todo 
		VulkanTexture* mDepthBuffer = nullptr;
		VulkanTextureView* mDepthBufferView = nullptr;
		VkRect2D mScissor;
	};
}