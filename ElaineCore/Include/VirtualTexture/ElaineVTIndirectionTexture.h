#pragma once
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include <vector>

namespace Elaine
{
	class RHICommandContext;
	class RHICommandList;
	class RHITexture;
	class RHIBuffer;
	class VirtualTextureSpace;

	/**
	 * VTIndirectionTexture manages the GPU-side indirection (page table) texture
	 * for a single VirtualTextureSpace.
	 *
	 * The indirection texture is a mipmapped R16G16B16A16_UINT texture where each texel
	 * maps a virtual page coordinate to the physical atlas tile location:
	 *   .r = PhysicalTileX
	 *   .g = PhysicalTileY
	 *   .b = packed [PoolIndex:8 | MipBias:8]
	 *   .a = flags (0 = not resident, 1 = resident)
	 *
	 * Update flow per frame:
	 * 1. CollectDirtyRegions()  - Gather changed page table entries from VirtualTextureSpace
	 * 2. WriteStagingBuffer()   - Write changed entries to a CPU-visible staging buffer
	 * 3. UploadToGPU()         - Issue copy commands from staging buffer to GPU texture
	 *
	 * Supports both:
	 * - Incremental update: Only uploads changed regions (optimal for steady state)
	 * - Full refresh: Uploads entire indirection texture (used on init or after many changes)
	 */
	class ElaineCoreExport VTIndirectionTexture
	{
	public:
		VTIndirectionTexture();
		~VTIndirectionTexture();

		/**
		 * Initialize the indirection texture for a given space.
		 * Creates the GPU texture and staging buffer.
		 * @param InSpace   The owning virtual texture space
		 * @param InCmdCtx  RHI command context for GPU resource creation
		 */
		void Initialize(VirtualTextureSpace* InSpace, RHICommandContext* InCmdCtx);

		/**
		 * Release all GPU resources.
		 */
		void Shutdown();

		/**
		 * Per-frame update: collect dirty entries, write staging buffer, upload.
		 * Call this from the render thread after page table updates.
		 * @param InCmdList  Active command list for issuing copy commands
		 * @return Number of entries updated
		 */
		uint32 Update(RHICommandList* InCmdList);

		/**
		 * Force a full refresh of the entire indirection texture.
		 * Useful after initialization or when too many entries are dirty.
		 * @param InCmdList  Active command list for issuing copy commands
		 */
		void ForceFullRefresh(RHICommandList* InCmdList);

		/**
		 * Get the GPU indirection texture for shader binding.
		 */
		RHITexture* GetGPUTexture() const { return mGPUTexture; }

		/**
		 * Get the staging buffer for CPU writes.
		 */
		RHIBuffer* GetStagingBuffer() const { return mStagingBuffer; }

		/**
		 * Check if a full refresh is pending.
		 */
		bool IsFullRefreshPending() const { return mNeedsFullRefresh; }

		/**
		 * Get number of entries updated in the last Update() call.
		 */
		uint32 GetLastUpdateCount() const { return mLastUpdateCount; }

		/**
		 * Request a full refresh on next Update() call.
		 */
		void RequestFullRefresh() { mNeedsFullRefresh = true; }

	private:
		/**
		 * Represents a rectangular region of the indirection texture that needs updating.
		 * Used to batch multiple individual texel updates into larger copy operations.
		 */
		struct DirtyRegion
		{
			uint8  MipLevel;
			uint16 X, Y;         // Texel coordinates within the mip level
			uint16 Width, Height; // Region dimensions (usually 1x1 for single tile updates)
			uint32 StagingOffset; // Byte offset into the staging buffer
		};

		/**
		 * Collect dirty entries from the VirtualTextureSpace and merge them
		 * into rectangular DirtyRegions for efficient GPU upload.
		 */
		void CollectDirtyRegions();

		/**
		 * Write the dirty entries to the staging buffer.
		 * @return Number of entries written
		 */
		uint32 WriteStagingBuffer();

		/**
		 * Write the entire indirection texture data to the staging buffer.
		 * @return Number of entries written
		 */
		uint32 WriteFullStagingBuffer();

		/**
		 * Issue GPU copy commands from staging buffer to GPU texture.
		 * @param InCmdList  Active command list
		 */
		void UploadToGPU(RHICommandList* InCmdList);

		/**
		 * Issue GPU copy commands for a full refresh.
		 * @param InCmdList  Active command list
		 */
		void UploadFullToGPU(RHICommandList* InCmdList);

		/**
		 * Compute the byte offset in the staging buffer for a specific mip level.
		 */
		uint32 GetMipStagingOffset(uint8 MipLevel) const;

		/**
		 * Compute the total staging buffer size needed for all mip levels.
		 */
		uint32 ComputeTotalStagingSize() const;

		/**
		 * Create the IndirectionEntry for a fallback (lower-mip) mapping.
		 */
		IndirectionEntry BuildFallbackEntry(const VTTileCoord& Coord) const;

	private:
		VirtualTextureSpace* mSpace = nullptr;
		RHICommandContext* mCmdCtx = nullptr;

		// GPU texture: R16G16B16A16_UINT, mipmapped
		RHITexture* mGPUTexture = nullptr;

		// CPU-visible staging buffer for uploading data
		RHIBuffer* mStagingBuffer = nullptr;

		// CPU-side mirror of the staging buffer data
		std::vector<uint8> mStagingData;

		// Dirty regions collected this frame
		std::vector<DirtyRegion> mDirtyRegions;

		// Per-mip level staging offsets (byte offsets into mStagingData)
		std::vector<uint32> mMipStagingOffsets;

		// State
		bool mInitialized = false;
		bool mNeedsFullRefresh = true; // First frame always needs full refresh

		// Stats
		uint32 mLastUpdateCount = 0;

		// Threshold: if more than this fraction of entries are dirty, do full refresh
		static constexpr float FullRefreshThreshold = 0.25f;
	};

} // namespace Elaine
