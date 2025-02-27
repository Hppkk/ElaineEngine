#pragma once

namespace VulkanRHI
{
	class VulkanDevice;
	class VulkanSemaphore;
	class VulkanFence;
	class VulkanQueue;

	class ElaineCoreExport VulkanSwapChain
	{
	public:
		VulkanSwapChain(VulkanDevice* InDevice, uint32 Width, uint32 Height, bool bIsFullscreen, VulkanSwapChain* InSwapChain, PixelFormat InFormat);
		const VkSwapchainKHR& GetSwapChain() const { return mSwapChainHandle; }
		void Present(VulkanQueue* InGfxQueue, VulkanSemaphore* InSemaphore);
		void Destroy();
		int32 AcquireImageIndex(VulkanSemaphore** OutSemaphore);
		void GetSwapChainImages(std::vector<VkImage>& InOutImages);
		uint32 GetNumSwapChainImages() const { return mNumSwapChainImages; }
		VkFormat GetVkFormat()const { return mImageFormat; }
		const VkSemaphore& GetIndexVkSemaphore(size_t InIndex);
		int32 GetCurrentImageIndex() const { return mCurrentImageIndex; }
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
		friend class VulkanQueue;
	};
}