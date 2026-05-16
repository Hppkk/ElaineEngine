#pragma once
#include "render/vulkan/ElaineVulkanTexture.h"
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include <vector>

namespace VulkanRHI
{
	/**
	 * VulkanSparseTexture extends VulkanTexture with Vulkan Sparse Resource support.
	 * 
	 * A sparse image in Vulkan allows partial residency: not all mip levels/tiles
	 * need to have memory bound at all times. This is the hardware foundation for
	 * virtual textures.
	 * 
	 * Key Vulkan concepts used:
	 * - VK_IMAGE_CREATE_SPARSE_BINDING_BIT: Image supports sparse memory binding
	 * - VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT: Image can be partially resident
	 * - vkQueueBindSparse: Bind/unbind memory to sparse image regions
	 * - VkSparseImageMemoryBind: Describes a bind operation for a specific mip region
	 */
	class ElaineCoreExport VulkanSparseTexture : public RHITexture
	{
	public:
		VulkanSparseTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc);
		~VulkanSparseTexture();

		//-----------------------------------------------
		// Initialization
		//-----------------------------------------------

		/** Create the sparse VkImage and query sparse properties */
		bool Initialize();

		/** Destroy the sparse image and all bound memory */
		void Destroy();

		//-----------------------------------------------
		// Sparse Memory Binding
		//-----------------------------------------------

		/**
		 * Bind memory to a specific tile region of the sparse image.
		 * @param MipLevel  The mip level to bind
		 * @param TileX     Tile X coordinate within the mip level
		 * @param TileY     Tile Y coordinate within the mip level
		 * @return true if binding succeeded
		 */
		bool BindTile(uint8 MipLevel, uint16 TileX, uint16 TileY);

		/**
		 * Unbind memory from a specific tile region.
		 * The freed memory is returned to the pool for reuse.
		 */
		bool UnbindTile(uint8 MipLevel, uint16 TileX, uint16 TileY);

		/**
		 * Submit all pending bind/unbind operations to the sparse binding queue.
		 * Call this once per frame after all bind/unbind calls.
		 * @param SignalSemaphore Optional semaphore to signal when binding is complete
		 */
		bool SubmitSparseBindings(VkSemaphore SignalSemaphore = VK_NULL_HANDLE,
								  VkSemaphore WaitSemaphore = VK_NULL_HANDLE);

		/**
		 * Bind the mip tail (lowest mip levels that are too small for sparse tiles).
		 * Must be done once during initialization.
		 */
		bool BindMipTail();

		//-----------------------------------------------
		// Query
		//-----------------------------------------------

		/** Get the VkImage handle */
		VkImage GetHandle() const { return mImage; }

		/** Get the image view for shader binding */
		VkImageView GetImageView() const { return mImageView; }

		/** Get the sparse tile size in pixels (as reported by the driver) */
		void GetSparseGranularity(uint32& OutWidth, uint32& OutHeight, uint32& OutDepth) const;

		/** Get the number of sparse tiles in X/Y at a given mip level */
		uint32 GetSparseTileCountX(uint8 MipLevel) const;
		uint32 GetSparseTileCountY(uint8 MipLevel) const;

		/** Check if a specific tile has memory bound */
		bool IsTileBound(uint8 MipLevel, uint16 TileX, uint16 TileY) const;

		/** Get the first mip level that is part of the mip tail */
		uint8 GetMipTailFirstLevel() const { return mMipTailFirstLevel; }

		/** Check if sparse residency is supported */
		bool IsSparseResidencySupported() const { return mSparseResidencySupported; }

		/** Get the VulkanDevice */
		VulkanDevice* GetDevice() const { return mDevice; }

	private:
		/** Allocate a page of device memory for sparse binding */
		VkDeviceMemory AllocatePage(VkDeviceSize Size);

		/** Free a page of device memory */
		void FreePage(VkDeviceMemory Memory);

		/** Compute linear index for tile tracking */
		uint32 ComputeTileIndex(uint8 MipLevel, uint16 TileX, uint16 TileY) const;

	private:
		VulkanDevice* mDevice = nullptr;

		// Vulkan sparse image resources
		VkImage mImage = VK_NULL_HANDLE;
		VkImageView mImageView = VK_NULL_HANDLE;

		// Sparse properties
		VkSparseImageFormatProperties mSparseFormat;
		VkSparseImageMemoryRequirements mSparseMemRequirements;
		VkExtent3D mSparseGranularity;  // Tile size in pixels as reported by driver

		// Memory requirements
		VkMemoryRequirements mMemRequirements;
		uint32 mMemoryTypeIndex = 0;
		VkDeviceSize mPageSize = 0;  // Size of one sparse page

		// Mip tail info
		uint8 mMipTailFirstLevel = 0;
		VkDeviceSize mMipTailOffset = 0;
		VkDeviceSize mMipTailSize = 0;
		VkDeviceMemory mMipTailMemory = VK_NULL_HANDLE;

		// Per-mip tile count
		struct MipLevelInfo
		{
			uint32 TilesX = 0;
			uint32 TilesY = 0;
			uint32 TileIndexOffset = 0;  // Offset into mBoundTiles array
		};
		std::vector<MipLevelInfo> mMipLevels;

		// Tracks which tiles have memory bound (true = bound)
		std::vector<bool> mBoundTiles;

		// Memory pages allocated for sparse binding
		struct SparsePage
		{
			VkDeviceMemory Memory = VK_NULL_HANDLE;
			uint8 MipLevel = 0;
			uint16 TileX = 0;
			uint16 TileY = 0;
		};
		std::vector<SparsePage> mAllocatedPages;

		// Pending bind/unbind operations for next SubmitSparseBindings call
		std::vector<VkSparseImageMemoryBind> mPendingBinds;
		std::vector<VkSparseImageOpaqueMemoryBind> mPendingOpaqueBinds;

		// Feature support
		bool mSparseResidencySupported = false;
	};

	//=============================================================================
	// Sparse Texture Utility Functions
	//=============================================================================

	/**
	 * Check if the device supports sparse texture residency for 2D images.
	 */
	ElaineCoreExport bool VulkanSupportsSparseTexture2D(VulkanDevice* Device);

	/**
	 * Query the sparse image format properties for a given format.
	 */
	ElaineCoreExport bool VulkanQuerySparseImageFormatProperties(
		VulkanDevice* Device,
		VkFormat Format,
		VkImageType ImageType,
		VkSampleCountFlagBits Samples,
		VkImageUsageFlags Usage,
		VkImageTiling Tiling,
		std::vector<VkSparseImageFormatProperties>& OutProperties);

} // namespace VulkanRHI
