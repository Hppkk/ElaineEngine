#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"

namespace VulkanRHI
{
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
		void InternalMoveSurface(VulkanDevice& InDevice, VulkanCommandContext& Context, VulkanAllocation& DestAllocation);
		void DestroySurface();

		const RHITextureDesc& GetDesc() const { return mDesc; }
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
		VulkanCpuReadbackBuffer*	mCpuReadbackBuffer;
		RHITextureDesc				mDesc;
		// View with all mips/layers
		VulkanTextureView			mDefaultView;
		// View with all mips/layers, but if it's a Depth/Stencil, only the Depth view
		VulkanTextureView*			mPartialView;
	};


}