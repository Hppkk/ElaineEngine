#pragma once
#include "VirtualTexture/ElaineVirtualTextureTypes.h"

namespace Elaine
{
	/**
	 * VirtualTextureSpace represents a single virtual texture address space.
	 * Each space has its own page table and indirection texture.
	 * Multiple virtual texture instances can share a space if they have compatible formats.
	 */
	class ElaineCoreExport VirtualTextureSpace
	{
	public:
		VirtualTextureSpace(uint8 InSpaceID, const VTSpaceDesc& InDesc);
		~VirtualTextureSpace();

		/** Initialize GPU resources (indirection texture, etc.) */
		void Initialize();

		/** Release GPU resources */
		void Shutdown();

		//-----------------------------------------------
		// Accessors
		//-----------------------------------------------

		uint8 GetSpaceID() const { return mSpaceID; }
		const VTSpaceDesc& GetDesc() const { return mDesc; }
		uint8 GetNumMipLevels() const { return mNumMipLevels; }

		//-----------------------------------------------
		// Page Table Operations
		//-----------------------------------------------

		/** Look up a page table entry for the given tile coordinate */
		const PageTableEntry* GetPageTableEntry(const VTTileCoord& Coord) const;

		/** Look up a page table entry, with fallback to lower-resolution mip if not resident */
		const PageTableEntry* GetPageTableEntryWithFallback(const VTTileCoord& Coord, uint8& OutFallbackMipLevel) const;

		/** Set page table entry (called when a tile becomes resident or is evicted) */
		void SetPageTableEntry(const VTTileCoord& Coord, const PageTableEntry& Entry);

		/** Mark a tile as used this frame (for LRU tracking) */
		void TouchTile(const VTTileCoord& Coord, uint32 FrameNumber);

		/** Get all entries that need indirection texture updates since the last GPU upload */
		void GetDirtyEntries(std::vector<std::pair<VTTileCoord, IndirectionEntry>>& OutDirtyEntries) const;

		/** Clear dirty flag after indirection texture upload */
		void ClearDirtyFlags();

		/** Get all resident tiles sorted by last-used frame (least recently used first) */
		void GetLRUSortedTiles(std::vector<VTTileCoord>& OutTiles) const;

		/** Remove a page table entry (used during eviction) */
		void RemovePageTableEntry(const VTTileCoord& Coord);

		//-----------------------------------------------
		// Indirection Texture
		//-----------------------------------------------

		/** Get the GPU indirection texture */
		RHITexture* GetIndirectionTexture() const { return mIndirectionTexture; }

		/** Get indirection texture dimensions for a given mip level */
		uint32 GetIndirectionTextureSizeX(uint8 MipLevel) const { return mDesc.GetTileCountX(MipLevel); }
		uint32 GetIndirectionTextureSizeY(uint8 MipLevel) const { return mDesc.GetTileCountY(MipLevel); }

		/** Build the indirection texture data for upload to GPU */
		void BuildIndirectionTextureData(std::vector<IndirectionEntry>& OutData, uint8 MipLevel) const;

		//-----------------------------------------------
		// World-space mapping (for RVT)
		//-----------------------------------------------

		/** Convert world position to virtual UV [0,1] */
		void WorldToVirtualUV(float WorldX, float WorldY, float& OutU, float& OutV) const;

		/** Convert virtual UV to tile coordinate at a given mip level */
		VTTileCoord VirtualUVToTileCoord(float U, float V, uint8 MipLevel) const;

		//-----------------------------------------------
		// Statistics
		//-----------------------------------------------

		uint32 GetResidentTileCount() const { return mResidentTileCount; }
		uint32 GetTotalTileCount() const;

	private:
		/** Compute a linear index for page table storage */
		uint32 ComputePageIndex(const VTTileCoord& Coord) const;

		/** Get the offset into the flattened page table for a given mip level */
		uint32 GetMipLevelOffset(uint8 MipLevel) const;

	private:
		uint8 mSpaceID = 0;
		VTSpaceDesc mDesc;
		uint8 mNumMipLevels = 0;

		// Flattened page table: indexed by ComputePageIndex()
		// Stores all mip levels contiguously
		std::vector<PageTableEntry> mPageTable;

		// Mip level offsets into the flattened page table
		std::vector<uint32> mMipLevelOffsets;

		// Set of dirty entries that need indirection texture update
		std::unordered_set<uint32> mDirtyPages;  // stores page indices

		// Statistics
		uint32 mResidentTileCount = 0;

		// GPU indirection texture (one texture with mip chain)
		// Mip 0 of indirection tex = mip 0 of VT (most detailed)
		RHITexture* mIndirectionTexture = nullptr;
	};

} // namespace Elaine
