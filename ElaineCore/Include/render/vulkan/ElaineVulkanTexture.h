#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
	class VulkanSwapChain;
	class ElaineCoreExport VulkanTextureView
	{
	public:
		VulkanTextureView()
			: mView(VK_NULL_HANDLE)
			, mImage(VK_NULL_HANDLE)
			, mViewId(0)
		{
		}

		void Create(VulkanDevice& Device, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags AspectFlags, PixelFormat InFormat, VkFormat Format, uint32 FirstMip, uint32 NumMips, uint32 ArraySliceIndex, uint32 NumArraySlices, bool bUseIdentitySwizzle = false, VkImageUsageFlags ImageUsageFlags = 0);
		void Destroy(VulkanDevice& Device);

		VkImageView mView;
		VkImage mImage;
		uint32 mViewId;
	};

	class ElaineCoreExport VulkanTexture :public RHITexture, VulkanEvictable
	{
	public:
		VulkanTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc);
		// 用于包装已有的 VkImage（如 Swapchain 图像）
		VulkanTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc, VkImage InExistingImage);
		~VulkanTexture();
		static void GenerateImageCreateInfo(VulkanImageCreateInfo& OutInfo, VulkanDevice* InDevice, const RHITextureDesc& InDesc, VkFormat* OutStorageFormat,
			VkFormat* OutViewFormat, bool bForceLinearTexture = false);
		VkImage getHandle() const { return mHandle; }
		VkImageViewType GetViewType() const
		{
			return TextureDimensionToVkImageViewType(GetDesc().mDimension);
		}
		// Full includes Depth+Stencil
		VkImageAspectFlags GetFullAspectMask() const
		{
			return mFullAspectMask;
		}

		uint32 GetMemorySize() const
		{
			return mMemRequirements.size;
		}
		void InvalidateViews(VulkanDevice& InDevice);

		bool CanMove() const override { return false; }
		bool CanEvict() const override { return false; }
		void Evict(VulkanDevice& Device, VulkanCommandContext& Context) override; ///evict to system memory
		void Move(VulkanDevice& Device, VulkanCommandContext& Context, VulkanAllocation& NewAllocation) override; //move to a full new allocation
		VulkanTexture* GetEvictableTexture() override { return this; }
		VkImageLayout GetImageLayout() const { return mImageLayout; }
		void SetImageLayout(VkImageLayout InVkImageLayout) { mImageLayout = InVkImageLayout; }
		void InternalMoveSurface(VulkanDevice& InDevice, VulkanCommandContext& Context, VulkanAllocation& DestAllocation);
		void DestroySurface();
		VkFormat GetVkFormat() const { return mViewFormat; }
		const VulkanTextureView& GetTextureView() const { return mDefaultView; }

		// 获取 Win32 共享内存 HANDLE（用于 Vulkan <-> DX11 纹理共享）
		void* GetSharedMemoryHandle() const override;
		VkExternalMemoryHandleTypeFlagBits GetExternalHandleType() const { return mExternalHandleType; }

		// CPU Readback 接口（staging buffer 方案）
		void InitReadbackResources() override;
		void CopyToReadbackBuffer() override;
		bool ReadbackPixels(void* OutData, uint32& OutRowPitch) override;
		bool IsReadbackReady() const override;
		void DestroyReadbackResources();

		// 查询驱动支持的外部内存 Handle 类型
		static VkExternalMemoryHandleTypeFlagBits QuerySupportedExternalHandleType(
			VulkanDevice* InDevice,
			VkFormat Format,
			VkImageType ImageType,
			VkImageTiling Tiling,
			VkImageUsageFlags Usage,
			VkImageCreateFlags CreateFlags);

		bool IsSwapchainImage() const { return mbIsSwapchainImage; }
		void SetIsSwapchainImage(bool bIsSwapchain) { mbIsSwapchainImage = bIsSwapchain; }

		bool IsProxy() const { return mbIsProxy; }
		void SetProxy(bool bProxy, VulkanSwapChain* Chain) 
		{ 
			mbIsProxy = bProxy; 
			mOwningSwapchain = Chain; 
		}
		VulkanSwapChain* GetOwningSwapchain() const { return mOwningSwapchain; }
	private:
		VkImage						mHandle = VK_NULL_HANDLE;
		VulkanDevice*				mDevice;
		VkFormat					mViewFormat;
		VkFormat					mStorageFormat;
		VkMemoryPropertyFlags		mMemProps;
		VkMemoryRequirements		mMemRequirements;
		VkImageTiling				mTiling;
		VulkanAllocation			mAllocation;
		VkImageAspectFlags			mFullAspectMask;
		VkImageAspectFlags			mPartialAspectMask;
		VkImageLayout				mImageLayout;
		VulkanCpuReadbackBuffer*	mCpuReadbackBuffer;
		// View with all mips/layers
		VulkanTextureView			mDefaultView;
		// View with all mips/layers, but if it's a Depth/Stencil, only the Depth view
		VulkanTextureView*			mPartialView;
		// 是否拥有 VkImage 所有权（Swapchain 图像为 false）
		bool						mbOwnsImage = true;
		bool						mbIsSwapchainImage = false;
		mutable HANDLE				mSharedHandle = nullptr;
		VkExternalMemoryHandleTypeFlagBits mExternalHandleType = (VkExternalMemoryHandleTypeFlagBits)0;
		bool						mbIsProxy = false;
		VulkanSwapChain*			mOwningSwapchain = nullptr;
	};


}
