#include "render/vulkan/ElaineVulkanSparseTexture.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "ElaineLogSystem.h"
#include <cassert>

namespace VulkanRHI
{
	//=============================================================================
	// VulkanSparseTexture
	//=============================================================================

	VulkanSparseTexture::VulkanSparseTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc)
		: RHITexture(InDesc)
		, mDevice(InDevice)
	{
		memset(&mSparseFormat, 0, sizeof(mSparseFormat));
		memset(&mSparseMemRequirements, 0, sizeof(mSparseMemRequirements));
		memset(&mSparseGranularity, 0, sizeof(mSparseGranularity));
		memset(&mMemRequirements, 0, sizeof(mMemRequirements));
	}

	VulkanSparseTexture::~VulkanSparseTexture()
	{
		Destroy();
	}

	bool VulkanSparseTexture::Initialize()
	{
		VkDevice Device = mDevice->GetDevice();

		// Check sparse residency support
		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(mDevice->GetPhyDevice()->GetHandle(), &Features);

		if (!Features.sparseBinding || !Features.sparseResidencyImage2D)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: Device does not support sparse residency for 2D images.");
			return false;
		}
		mSparseResidencySupported = true;

		// Determine VkFormat from PixelFormat
		// Using R8G8B8A8_UNORM as default; actual conversion should use engine's format mapping
		VkFormat VkFmt = VK_FORMAT_R8G8B8A8_UNORM;
		// TODO: Convert from mDesc.mFormat using engine's format conversion utilities

		// Create sparse VkImage
		VkImageCreateInfo ImageCI = {};
		ImageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCI.imageType = VK_IMAGE_TYPE_2D;
		ImageCI.format = VkFmt;
		ImageCI.extent.width = (uint32_t)mDesc.mExtent.x;
		ImageCI.extent.height = (uint32_t)mDesc.mExtent.y;
		ImageCI.extent.depth = 1;
		ImageCI.mipLevels = mDesc.mNumMips;
		ImageCI.arrayLayers = 1;
		ImageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		ImageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		ImageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		ImageCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		// Sparse flags
		ImageCI.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;

		VkResult Result = vkCreateImage(Device, &ImageCI, VULKAN_CPU_ALLOCATOR, &mImage);
		if (Result != VK_SUCCESS)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: vkCreateImage failed with result %d", (int)Result);
			return false;
		}

		// Query memory requirements
		vkGetImageMemoryRequirements(Device, mImage, &mMemRequirements);

		// Find suitable memory type
		VkPhysicalDeviceMemoryProperties MemProps;
		vkGetPhysicalDeviceMemoryProperties(mDevice->GetPhyDevice()->GetHandle(), &MemProps);

		mMemoryTypeIndex = UINT32_MAX;
		for (uint32 i = 0; i < MemProps.memoryTypeCount; ++i)
		{
			if ((mMemRequirements.memoryTypeBits & (1 << i)) &&
				(MemProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
			{
				mMemoryTypeIndex = i;
				break;
			}
		}

		if (mMemoryTypeIndex == UINT32_MAX)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: Could not find suitable memory type.");
			Destroy();
			return false;
		}

		// Query sparse image memory requirements
		uint32 SparseReqCount = 0;
		vkGetImageSparseMemoryRequirements(Device, mImage, &SparseReqCount, nullptr);

		std::vector<VkSparseImageMemoryRequirements> SparseReqs(SparseReqCount);
		vkGetImageSparseMemoryRequirements(Device, mImage, &SparseReqCount, SparseReqs.data());

		// Find the color aspect sparse requirements
		for (const auto& Req : SparseReqs)
		{
			if (Req.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)
			{
				mSparseMemRequirements = Req;
				mSparseFormat = Req.formatProperties;
				mSparseGranularity = Req.formatProperties.imageGranularity;
				mMipTailFirstLevel = Req.imageMipTailFirstLod;
				mMipTailOffset = Req.imageMipTailOffset;
				mMipTailSize = Req.imageMipTailSize;
				break;
			}
		}

		mPageSize = mMemRequirements.alignment; // Sparse page size = alignment

		LogSystem::instance()->Log(LogLevel::Info,
			"VulkanSparseTexture: Created %ux%u, %u mips, granularity=%ux%ux%u, mipTailFirst=%u, pageSize=%llu",
			(uint32_t)mDesc.mExtent.x, (uint32_t)mDesc.mExtent.y,
			mDesc.mNumMips,
			mSparseGranularity.width, mSparseGranularity.height, mSparseGranularity.depth,
			mMipTailFirstLevel, (unsigned long long)mPageSize);

		// Compute per-mip tile counts
		mMipLevels.resize(mDesc.mNumMips);
		uint32 TotalTiles = 0;
		for (uint8 Mip = 0; Mip < mDesc.mNumMips; ++Mip)
		{
			if (Mip < mMipTailFirstLevel)
			{
				uint32 MipWidth = std::max(1u, (uint32_t)mDesc.mExtent.x >> Mip);
				uint32 MipHeight = std::max(1u, (uint32_t)mDesc.mExtent.y >> Mip);

				mMipLevels[Mip].TilesX = (MipWidth + mSparseGranularity.width - 1) / mSparseGranularity.width;
				mMipLevels[Mip].TilesY = (MipHeight + mSparseGranularity.height - 1) / mSparseGranularity.height;
				mMipLevels[Mip].TileIndexOffset = TotalTiles;
				TotalTiles += mMipLevels[Mip].TilesX * mMipLevels[Mip].TilesY;
			}
			else
			{
				// Part of mip tail, handled as opaque binding
				mMipLevels[Mip].TilesX = 0;
				mMipLevels[Mip].TilesY = 0;
				mMipLevels[Mip].TileIndexOffset = TotalTiles;
			}
		}

		mBoundTiles.resize(TotalTiles, false);

		// Create image view
		VkImageViewCreateInfo ViewCI = {};
		ViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewCI.image = mImage;
		ViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ViewCI.format = VkFmt;
		ViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ViewCI.subresourceRange.baseMipLevel = 0;
		ViewCI.subresourceRange.levelCount = mDesc.mNumMips;
		ViewCI.subresourceRange.baseArrayLayer = 0;
		ViewCI.subresourceRange.layerCount = 1;
		ViewCI.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
							  VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

		Result = vkCreateImageView(Device, &ViewCI, VULKAN_CPU_ALLOCATOR, &mImageView);
		if (Result != VK_SUCCESS)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: vkCreateImageView failed with result %d", (int)Result);
			Destroy();
			return false;
		}

		// Bind mip tail immediately (small mips that are always resident)
		if (!BindMipTail())
		{
			LogSystem::instance()->Log(LogLevel::Warning,
				"VulkanSparseTexture: MipTail binding failed or not needed.");
		}

		return true;
	}

	void VulkanSparseTexture::Destroy()
	{
		if (!mDevice)
			return;

		VkDevice Device = mDevice->GetDevice();

		// Free all allocated pages
		for (auto& Page : mAllocatedPages)
		{
			if (Page.Memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(Device, Page.Memory, VULKAN_CPU_ALLOCATOR);
				Page.Memory = VK_NULL_HANDLE;
			}
		}
		mAllocatedPages.clear();

		// Free mip tail memory
		if (mMipTailMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(Device, mMipTailMemory, VULKAN_CPU_ALLOCATOR);
			mMipTailMemory = VK_NULL_HANDLE;
		}

		// Destroy image view
		if (mImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(Device, mImageView, VULKAN_CPU_ALLOCATOR);
			mImageView = VK_NULL_HANDLE;
		}

		// Destroy image
		if (mImage != VK_NULL_HANDLE)
		{
			vkDestroyImage(Device, mImage, VULKAN_CPU_ALLOCATOR);
			mImage = VK_NULL_HANDLE;
		}

		mBoundTiles.clear();
		mMipLevels.clear();
		mPendingBinds.clear();
		mPendingOpaqueBinds.clear();
	}

	//-----------------------------------------------
	// Sparse Memory Binding
	//-----------------------------------------------

	bool VulkanSparseTexture::BindTile(uint8 MipLevel, uint16 TileX, uint16 TileY)
	{
		if (MipLevel >= mMipTailFirstLevel)
		{
			// Part of mip tail, handled by BindMipTail
			return false;
		}

		uint32 TileIdx = ComputeTileIndex(MipLevel, TileX, TileY);
		if (TileIdx >= mBoundTiles.size())
			return false;

		if (mBoundTiles[TileIdx])
			return true; // Already bound

		// Allocate a page of device memory
		VkDeviceMemory PageMemory = AllocatePage(mPageSize);
		if (PageMemory == VK_NULL_HANDLE)
			return false;

		// Create sparse image memory bind
		VkSparseImageMemoryBind Bind = {};
		Bind.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Bind.subresource.mipLevel = MipLevel;
		Bind.subresource.arrayLayer = 0;
		Bind.offset.x = TileX * mSparseGranularity.width;
		Bind.offset.y = TileY * mSparseGranularity.height;
		Bind.offset.z = 0;
		Bind.extent = mSparseGranularity;

		// Clamp extent to image bounds at this mip level
		uint32 MipWidth = std::max(1u, (uint32_t)mDesc.mExtent.x >> MipLevel);
		uint32 MipHeight = std::max(1u, (uint32_t)mDesc.mExtent.y >> MipLevel);
		if (Bind.offset.x + Bind.extent.width > MipWidth)
			Bind.extent.width = MipWidth - Bind.offset.x;
		if (Bind.offset.y + Bind.extent.height > MipHeight)
			Bind.extent.height = MipHeight - Bind.offset.y;

		Bind.memory = PageMemory;
		Bind.memoryOffset = 0;
		Bind.flags = 0;

		mPendingBinds.push_back(Bind);

		// Track the allocation
		SparsePage Page;
		Page.Memory = PageMemory;
		Page.MipLevel = MipLevel;
		Page.TileX = TileX;
		Page.TileY = TileY;
		mAllocatedPages.push_back(Page);

		mBoundTiles[TileIdx] = true;

		return true;
	}

	bool VulkanSparseTexture::UnbindTile(uint8 MipLevel, uint16 TileX, uint16 TileY)
	{
		if (MipLevel >= mMipTailFirstLevel)
			return false;

		uint32 TileIdx = ComputeTileIndex(MipLevel, TileX, TileY);
		if (TileIdx >= mBoundTiles.size())
			return false;

		if (!mBoundTiles[TileIdx])
			return true; // Already unbound

		// Find the allocated page
		VkDeviceMemory PageToFree = VK_NULL_HANDLE;
		for (auto It = mAllocatedPages.begin(); It != mAllocatedPages.end(); ++It)
		{
			if (It->MipLevel == MipLevel && It->TileX == TileX && It->TileY == TileY)
			{
				PageToFree = It->Memory;
				mAllocatedPages.erase(It);
				break;
			}
		}

		// Create unbind operation (memory = VK_NULL_HANDLE means unbind)
		VkSparseImageMemoryBind Bind = {};
		Bind.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Bind.subresource.mipLevel = MipLevel;
		Bind.subresource.arrayLayer = 0;
		Bind.offset.x = TileX * mSparseGranularity.width;
		Bind.offset.y = TileY * mSparseGranularity.height;
		Bind.offset.z = 0;
		Bind.extent = mSparseGranularity;
		Bind.memory = VK_NULL_HANDLE; // Unbind
		Bind.memoryOffset = 0;
		Bind.flags = 0;

		mPendingBinds.push_back(Bind);

		// Free the memory page
		if (PageToFree != VK_NULL_HANDLE)
		{
			FreePage(PageToFree);
		}

		mBoundTiles[TileIdx] = false;

		return true;
	}

	bool VulkanSparseTexture::SubmitSparseBindings(VkSemaphore SignalSemaphore, VkSemaphore WaitSemaphore)
	{
		if (mPendingBinds.empty() && mPendingOpaqueBinds.empty())
			return true; // Nothing to do

		VkDevice Device = mDevice->GetDevice();

		// Image memory binds
		VkSparseImageMemoryBindInfo ImageBindInfo = {};
		ImageBindInfo.image = mImage;
		ImageBindInfo.bindCount = (uint32_t)mPendingBinds.size();
		ImageBindInfo.pBinds = mPendingBinds.data();

		// Opaque memory binds (for mip tail)
		VkSparseImageOpaqueMemoryBindInfo OpaqueBindInfo = {};
		OpaqueBindInfo.image = mImage;
		OpaqueBindInfo.bindCount = (uint32_t)mPendingOpaqueBinds.size();
		OpaqueBindInfo.pBinds = mPendingOpaqueBinds.data();

		VkBindSparseInfo BindInfo = {};
		BindInfo.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;

		if (!mPendingBinds.empty())
		{
			BindInfo.imageBindCount = 1;
			BindInfo.pImageBinds = &ImageBindInfo;
		}

		if (!mPendingOpaqueBinds.empty())
		{
			BindInfo.imageOpaqueBindCount = 1;
			BindInfo.pImageOpaqueBinds = &OpaqueBindInfo;
		}

		if (WaitSemaphore != VK_NULL_HANDLE)
		{
			BindInfo.waitSemaphoreCount = 1;
			BindInfo.pWaitSemaphores = &WaitSemaphore;
		}

		if (SignalSemaphore != VK_NULL_HANDLE)
		{
			BindInfo.signalSemaphoreCount = 1;
			BindInfo.pSignalSemaphores = &SignalSemaphore;
		}

		// Submit to the graphics queue (which supports sparse binding)
		VulkanQueue* Queue = mDevice->GetGraphicQueue();
		VkResult Result = vkQueueBindSparse(Queue->GetHandle(), 1, &BindInfo, VK_NULL_HANDLE);

		if (Result != VK_SUCCESS)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: vkQueueBindSparse failed with result %d", (int)Result);
			return false;
		}

		mPendingBinds.clear();
		mPendingOpaqueBinds.clear();

		return true;
	}

	bool VulkanSparseTexture::BindMipTail()
	{
		if (mMipTailSize == 0 || mMipTailFirstLevel >= mDesc.mNumMips)
			return true; // No mip tail

		VkDevice Device = mDevice->GetDevice();

		// Allocate memory for the entire mip tail
		mMipTailMemory = AllocatePage(mMipTailSize);
		if (mMipTailMemory == VK_NULL_HANDLE)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: Failed to allocate mip tail memory (%llu bytes)",
				(unsigned long long)mMipTailSize);
			return false;
		}

		// Create opaque bind for mip tail
		VkSparseMemoryBind OpaqueBind = {};
		OpaqueBind.resourceOffset = mMipTailOffset;
		OpaqueBind.size = mMipTailSize;
		OpaqueBind.memory = mMipTailMemory;
		OpaqueBind.memoryOffset = 0;
		OpaqueBind.flags = 0;

		VkSparseImageOpaqueMemoryBind OpaqueImageBind = {};
		// Reinterpret as opaque bind (mip tail is bound as opaque)
		VkSparseImageOpaqueMemoryBindInfo OpaqueInfo = {};
		OpaqueInfo.image = mImage;
		OpaqueInfo.bindCount = 1;

		// Store as pending opaque bind
		VkSparseImageOpaqueMemoryBind PendingOpaque = {};
		PendingOpaque.resourceOffset = mMipTailOffset;
		PendingOpaque.size = mMipTailSize;
		PendingOpaque.memory = mMipTailMemory;
		PendingOpaque.memoryOffset = 0;
		PendingOpaque.flags = 0;
		mPendingOpaqueBinds.push_back(PendingOpaque);

		LogSystem::instance()->Log(LogLevel::Info,
			"VulkanSparseTexture: Bound mip tail (offset=%llu, size=%llu, firstMip=%u)",
			(unsigned long long)mMipTailOffset,
			(unsigned long long)mMipTailSize,
			mMipTailFirstLevel);

		return true;
	}

	//-----------------------------------------------
	// Query
	//-----------------------------------------------

	void VulkanSparseTexture::GetSparseGranularity(uint32& OutWidth, uint32& OutHeight, uint32& OutDepth) const
	{
		OutWidth = mSparseGranularity.width;
		OutHeight = mSparseGranularity.height;
		OutDepth = mSparseGranularity.depth;
	}

	uint32 VulkanSparseTexture::GetSparseTileCountX(uint8 MipLevel) const
	{
		if (MipLevel >= mMipLevels.size())
			return 0;
		return mMipLevels[MipLevel].TilesX;
	}

	uint32 VulkanSparseTexture::GetSparseTileCountY(uint8 MipLevel) const
	{
		if (MipLevel >= mMipLevels.size())
			return 0;
		return mMipLevels[MipLevel].TilesY;
	}

	bool VulkanSparseTexture::IsTileBound(uint8 MipLevel, uint16 TileX, uint16 TileY) const
	{
		uint32 Idx = ComputeTileIndex(MipLevel, TileX, TileY);
		if (Idx >= mBoundTiles.size())
			return false;
		return mBoundTiles[Idx];
	}

	//-----------------------------------------------
	// Private Helpers
	//-----------------------------------------------

	VkDeviceMemory VulkanSparseTexture::AllocatePage(VkDeviceSize Size)
	{
		VkMemoryAllocateInfo AllocInfo = {};
		AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		AllocInfo.allocationSize = Size;
		AllocInfo.memoryTypeIndex = mMemoryTypeIndex;

		VkDeviceMemory Memory = VK_NULL_HANDLE;
		VkResult Result = vkAllocateMemory(mDevice->GetDevice(), &AllocInfo, VULKAN_CPU_ALLOCATOR, &Memory);
		if (Result != VK_SUCCESS)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VulkanSparseTexture: vkAllocateMemory failed for sparse page (%llu bytes), result=%d",
				(unsigned long long)Size, (int)Result);
			return VK_NULL_HANDLE;
		}
		return Memory;
	}

	void VulkanSparseTexture::FreePage(VkDeviceMemory Memory)
	{
		if (Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(mDevice->GetDevice(), Memory, VULKAN_CPU_ALLOCATOR);
		}
	}

	uint32 VulkanSparseTexture::ComputeTileIndex(uint8 MipLevel, uint16 TileX, uint16 TileY) const
	{
		if (MipLevel >= mMipLevels.size() || MipLevel >= mMipTailFirstLevel)
			return (uint32)mBoundTiles.size(); // Out of bounds

		const MipLevelInfo& Info = mMipLevels[MipLevel];
		if (TileX >= Info.TilesX || TileY >= Info.TilesY)
			return (uint32)mBoundTiles.size(); // Out of bounds

		return Info.TileIndexOffset + TileY * Info.TilesX + TileX;
	}

	//=============================================================================
	// Utility Functions
	//=============================================================================

	bool VulkanSupportsSparseTexture2D(VulkanDevice* Device)
	{
		if (!Device || !Device->GetPhyDevice())
			return false;

		VkPhysicalDeviceFeatures Features;
		vkGetPhysicalDeviceFeatures(Device->GetPhyDevice()->GetHandle(), &Features);

		return Features.sparseBinding && Features.sparseResidencyImage2D;
	}

	bool VulkanQuerySparseImageFormatProperties(
		VulkanDevice* Device,
		VkFormat Format,
		VkImageType ImageType,
		VkSampleCountFlagBits Samples,
		VkImageUsageFlags Usage,
		VkImageTiling Tiling,
		std::vector<VkSparseImageFormatProperties>& OutProperties)
	{
		if (!Device || !Device->GetPhyDevice())
			return false;

		uint32 PropertyCount = 0;
		vkGetPhysicalDeviceSparseImageFormatProperties(
			Device->GetPhyDevice()->GetHandle(),
			Format, ImageType, Samples, Usage, Tiling,
			&PropertyCount, nullptr);

		if (PropertyCount == 0)
			return false;

		OutProperties.resize(PropertyCount);
		vkGetPhysicalDeviceSparseImageFormatProperties(
			Device->GetPhyDevice()->GetHandle(),
			Format, ImageType, Samples, Usage, Tiling,
			&PropertyCount, OutProperties.data());

		return !OutProperties.empty();
	}

} // namespace VulkanRHI
