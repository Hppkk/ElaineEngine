#pragma once
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include <list>

namespace Elaine
{
	/**
	 * PhysicalTilePool manages the GPU-side physical texture atlas.
	 * 
	 * The atlas is a large 2D texture (e.g., 4352x4352 for 32x32 tiles with border)
	 * divided into a grid of tile slots. Each slot holds one TileSizeWithBorder x TileSizeWithBorder
	 * region of pixel data.
	 * 
	 * The pool supports:
	 * - Multiple layers (e.g., Albedo, Normal, Roughness) stored as separate atlas textures
	 * - LRU-based eviction when the pool is full
	 * - Free list management for efficient allocation
	 * - Tile data upload via staging buffer
	 */
	class ElaineCoreExport PhysicalTilePool
	{
	public:
		PhysicalTilePool(uint8 InPoolIndex, const PhysicalTilePoolDesc& InDesc);
		~PhysicalTilePool();

		/** Initialize GPU atlas textures */
		void Initialize();

		/** Release GPU resources */
		void Shutdown();

		//-----------------------------------------------
		// Tile Allocation
		//-----------------------------------------------

		/**
		 * Allocate a physical tile slot.
		 * Returns the atlas location, or invalid if pool is full.
		 */
		PhysicalTileLocation Allocate();

		/**
		 * Free a physical tile slot, returning it to the free list.
		 */
		void Free(const PhysicalTileLocation& Location);

		/**
		 * Check if the pool has available slots.
		 */
		bool HasFreeSlots() const { return !mFreeSlots.empty(); }

		/**
		 * Get the number of free slots remaining.
		 */
		uint32 GetFreeSlotCount() const { return (uint32)mFreeSlots.size(); }

		/**
		 * Get the number of used slots.
		 */
		uint32 GetUsedSlotCount() const { return mCapacity - (uint32)mFreeSlots.size(); }

		/**
		 * Get utilization as a percentage [0.0, 1.0].
		 */
		float GetUtilization() const
		{
			return mCapacity > 0 ? (float)GetUsedSlotCount() / (float)mCapacity : 0.0f;
		}

		//-----------------------------------------------
		// LRU Management
		//-----------------------------------------------

		/**
		 * Touch a tile (move to most-recently-used end of LRU list).
		 */
		void TouchTile(const PhysicalTileLocation& Location, uint32 FrameNumber);

		/**
		 * Get the least recently used tile location.
		 * Used by eviction logic.
		 */
		bool GetLeastRecentlyUsedTile(PhysicalTileLocation& OutLocation) const;

		/**
		 * Evict the least recently used tile, freeing its slot.
		 * Returns the evicted tile's virtual coordinate through OutEvictedCoord.
		 */
		bool EvictLRU(VTTileCoord& OutEvictedCoord);

		//-----------------------------------------------
		// Tile Data Upload
		//-----------------------------------------------

		/**
		 * Upload tile data to the physical atlas at the given location.
		 * Handles all layers simultaneously.
		 * @param Location  The atlas slot to upload to
		 * @param TileData  The tile data containing per-layer pixel data
		 */
		void UploadTileData(const PhysicalTileLocation& Location, const VTTileData& TileData);

		//-----------------------------------------------
		// GPU Resources
		//-----------------------------------------------

		/** Get the atlas texture for a specific layer */
		RHITexture* GetAtlasTexture(uint8 LayerIndex) const;

		/** Get the atlas size in pixels */
		uint32 GetAtlasSizePixels() const { return mDesc.AtlasSizeInTiles * VTConstants::TileSizeWithBorder; }

		/** Get the atlas size in tiles per dimension */
		uint32 GetAtlasSizeInTiles() const { return mDesc.AtlasSizeInTiles; }

		/** Get pool index */
		uint8 GetPoolIndex() const { return mPoolIndex; }

		/** Get the descriptor */
		const PhysicalTilePoolDesc& GetDesc() const { return mDesc; }

		/** Get capacity (total slots) */
		uint32 GetCapacity() const { return mCapacity; }

	private:
		/** Convert 2D tile index to linear index */
		uint32 TileToLinearIndex(uint16 TileX, uint16 TileY) const
		{
			return TileY * mDesc.AtlasSizeInTiles + TileX;
		}

		/** Convert linear index to 2D tile index */
		void LinearToTileIndex(uint32 LinearIndex, uint16& OutTileX, uint16& OutTileY) const
		{
			OutTileX = uint16(LinearIndex % mDesc.AtlasSizeInTiles);
			OutTileY = uint16(LinearIndex / mDesc.AtlasSizeInTiles);
		}

	private:
		uint8 mPoolIndex = 0;
		PhysicalTilePoolDesc mDesc;
		uint32 mCapacity = 0;

		// Free slot indices (LIFO stack for fast allocation)
		std::vector<uint32> mFreeSlots;

		// LRU tracking: maps linear tile index -> LRU list iterator
		struct LRUEntry
		{
			uint32 LinearIndex = 0;
			uint32 FrameLastUsed = 0;
			VTTileCoord VirtualCoord;  // Which virtual tile is mapped here
		};

		// Doubly-linked list for O(1) LRU move-to-front/evict-from-back
		std::list<LRUEntry> mLRUList;

		// Map from linear tile index to LRU list iterator for O(1) lookup
		std::unordered_map<uint32, std::list<LRUEntry>::iterator> mLRUMap;

		// GPU atlas textures (one per layer)
		std::array<RHITexture*, VTConstants::MaxLayers> mAtlasTextures;

		// Staging buffer for tile uploads
		RHIBuffer* mStagingBuffer = nullptr;
	};

} // namespace Elaine
