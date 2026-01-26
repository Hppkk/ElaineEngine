#pragma once

namespace VulkanRHI
{
	class VulkanDevice;
	class VulkanSemaphore;
	class VulkanFence;
	class VulkanQueue;
	class VulkanTextureView;

	class ElaineCoreExport VulkanSwapChain : public RHISwapchain
	{
	public:
		VulkanSwapChain(VulkanDevice* InDevice, uint32 Width, uint32 Height, bool bIsFullscreen, VulkanSwapChain* InSwapChain, PixelFormat InFormat);
		const VkSwapchainKHR& GetSwapChain() const { return mSwapChainHandle; }
		void Present(VulkanQueue* InGfxQueue, VulkanSemaphore* InSemaphore);
		void Destroy();
		virtual RHITexture* AcquireNextTexture() override;
		virtual RHITexture* AcquireAndGetBackBuffer(void** OutSemaphore = nullptr) override;
		virtual void Resize(uint32 InWidth, uint32 InHeight) override;
		int32 AcquireImageIndex(VulkanSemaphore** OutSemaphore);
		bool AcquireImage(VulkanSemaphore* InSemaphore, uint32& OutImageIndex);
		void GetSwapChainImages(std::vector<VkImage>& InOutImages);
		uint32 GetNumSwapChainImages() const { return mNumSwapChainImages; }
		VkFormat GetVkFormat()const { return mImageFormat; }
		const VkSemaphore& GetIndexVkSemaphore(size_t InIndex);
		int32 GetCurrentImageIndex() const { return mCurrentImageIndex; }
		void CreateTextureView();
		void CreateSwapchain(VulkanSwapChain* InOldSwapchain);
		void DestroySwapchain();
		void RecreateSwapchain();
		virtual void GetSize(uint32& OutWidth, uint32& OutHeight) override;

		VulkanTexture* GetActiveBackBufferTexture();
		RHITexture* GetBackBufferProxy();
	private:
		int32 mSemaphoreIndex = 0;
		uint32 mInternalWidth = 0;
		uint32 mInternalHeight = 0;
		int32 mCurrentImageIndex = 0;
		uint32 mNumSwapChainImages = 0;
		PixelFormat mFormat;
		VulkanDevice* mDevice = nullptr;
		bool mbInternalFullScreen = false;
		VkSwapchainKHR mSwapChainHandle = nullptr;
		VkFormat mImageFormat = VK_FORMAT_UNDEFINED;
		std::vector<VulkanSemaphore*> mImageAcquiredSemaphore;
		std::vector<VulkanFence*> mImageAcquiredFences;
		int mAcquiredImageIndex = 0;
		std::vector<VkImage> mBackBufferImages;
		std::vector<VulkanTexture*> mBackBufferTextures;
		VulkanTexture* mBackBufferProxy = nullptr;
		std::mutex mMtx;

		friend class VulkanQueue;
	};
}
