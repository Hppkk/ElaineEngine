#include "VirtualTexture/ElainePhysicalTilePool.h"
#include "ElaineLogSystem.h"
#include <cassert>

namespace Elaine
{
	PhysicalTilePool::PhysicalTilePool(uint8 InPoolIndex, const PhysicalTilePoolDesc& InDesc)
		: mPoolIndex(InPoolIndex)
		, mDesc(InDesc)
		, mCapacity(InDesc.GetCapacity())
	{
		mAtlasTextures.fill(nullptr);

		// Initialize free list with all tile slots (in reverse order for LIFO efficiency)
		mFreeSlots.reserve(mCapacity);
		for (uint32 i = mCapacity; i > 0; --i)
		{
			mFreeSlots.push_back(i - 1);
		}
	}

	PhysicalTilePool::~PhysicalTilePool()
	{
		Shutdown();
	}

	void PhysicalTilePool::Initialize()
	{
		uint32 AtlasPixels = GetAtlasSizePixels();

		LogSystem::instance()->Log(LogLevel::Info,
			"PhysicalTilePool[%u]: Initializing atlas %ux%u pixels (%ux%u tiles), %u layers, capacity=%u",
			mPoolIndex, AtlasPixels, AtlasPixels,
			mDesc.AtlasSizeInTiles, mDesc.AtlasSizeInTiles,
			mDesc.NumLayers, mCapacity);

		// Create atlas textures for each layer
		// Note: Actual RHI texture creation will use the engine's RHI command context
		// This is a placeholder - the real implementation will submit RHI commands
		for (uint8 Layer = 0; Layer < mDesc.NumLayers; ++Layer)
		{
			RHITextureDesc AtlasDesc;
			AtlasDesc.mExtent = Vector2((float)AtlasPixels, (float)AtlasPixels);
			AtlasDesc.mFormat = mDesc.LayerFormats[Layer];
			AtlasDesc.mNumMips = 1;  // Atlas doesn't need mip chain (tiles contain their own mip data)
			AtlasDesc.mFlags = TextureCreateFlags::ShaderResource;
			AtlasDesc.mDimension = TextureDimension::Texture2D;

			// TODO: Create RHI texture through command context
			// mAtlasTextures[Layer] = RHICreateTexture(AtlasDesc);

			LogSystem::instance()->Log(LogLevel::Info,
				"PhysicalTilePool[%u]: Created atlas texture for layer %u (format=%d)",
				mPoolIndex, Layer, (int)mDesc.LayerFormats[Layer]);
		}

		// Create staging buffer for tile uploads
		// Size enough for one tile with border across all layers
		uint32 TileBytes = VTConstants::TileSizeWithBorder * VTConstants::TileSizeWithBorder * 4; // 4 bytes per pixel (RGBA8)
		uint32 StagingSize = TileBytes * mDesc.NumLayers * VTConstants::MaxTileUploadsPerFrame;

		// TODO: Create staging buffer
		// mStagingBuffer = RHICreateBuffer(StagingSize, BufferUsageFlags::SourceCopy | BufferUsageFlags::Dynamic);

		LogSystem::instance()->Log(LogLevel::Info,
			"PhysicalTilePool[%u]: Initialization complete", mPoolIndex);
	}

	void PhysicalTilePool::Shutdown()
	{
		// Release GPU resources
		for (uint8 Layer = 0; Layer < VTConstants::MaxLayers; ++Layer)
		{
			mAtlasTextures[Layer] = nullptr;
		}
		mStagingBuffer = nullptr;

		// Clear allocation state
		mFreeSlots.clear();
		mLRUList.clear();
		mLRUMap.clear();

		LogSystem::instance()->Log(LogLevel::Info,
			"PhysicalTilePool[%u]: Shutdown complete", mPoolIndex);
	}

	//-----------------------------------------------
	// Tile Allocation
	//-----------------------------------------------

	PhysicalTileLocation PhysicalTilePool::Allocate()
	{
		if (mFreeSlots.empty())
		{
			// Pool is full
			PhysicalTileLocation Invalid;
			return Invalid;
		}

		// Pop from free list (LIFO)
		uint32 LinearIndex = mFreeSlots.back();
		mFreeSlots.pop_back();

		PhysicalTileLocation Location;
		LinearToTileIndex(LinearIndex, Location.AtlasX, Location.AtlasY);
		Location.PoolIndex = mPoolIndex;

		return Location;
	}

	void PhysicalTilePool::Free(const PhysicalTileLocation& Location)
	{
		if (!Location.IsValid() || Location.PoolIndex != mPoolIndex)
			return;

		uint32 LinearIndex = TileToLinearIndex(Location.AtlasX, Location.AtlasY);

		// Remove from LRU tracking
		auto LRUIt = mLRUMap.find(LinearIndex);
		if (LRUIt != mLRUMap.end())
		{
			mLRUList.erase(LRUIt->second);
			mLRUMap.erase(LRUIt);
		}

		// Return to free list
		mFreeSlots.push_back(LinearIndex);
	}

	//-----------------------------------------------
	// LRU Management
	//-----------------------------------------------

	void PhysicalTilePool::TouchTile(const PhysicalTileLocation& Location, uint32 FrameNumber)
	{
		if (!Location.IsValid() || Location.PoolIndex != mPoolIndex)
			return;

		uint32 LinearIndex = TileToLinearIndex(Location.AtlasX, Location.AtlasY);

		auto LRUIt = mLRUMap.find(LinearIndex);
		if (LRUIt != mLRUMap.end())
		{
			// Move to front (most recently used)
			LRUEntry Entry = *(LRUIt->second);
			Entry.FrameLastUsed = FrameNumber;
			mLRUList.erase(LRUIt->second);
			mLRUList.push_front(Entry);
			mLRUMap[LinearIndex] = mLRUList.begin();
		}
		else
		{
			// New entry - add to front
			LRUEntry Entry;
			Entry.LinearIndex = LinearIndex;
			Entry.FrameLastUsed = FrameNumber;
			mLRUList.push_front(Entry);
			mLRUMap[LinearIndex] = mLRUList.begin();
		}
	}

	bool PhysicalTilePool::GetLeastRecentlyUsedTile(PhysicalTileLocation& OutLocation) const
	{
		if (mLRUList.empty())
			return false;

		// LRU is at the back of the list
		const LRUEntry& Entry = mLRUList.back();
		LinearToTileIndex(Entry.LinearIndex, OutLocation.AtlasX, OutLocation.AtlasY);
		OutLocation.PoolIndex = mPoolIndex;
		return true;
	}

	bool PhysicalTilePool::EvictLRU(VTTileCoord& OutEvictedCoord)
	{
		if (mLRUList.empty())
			return false;

		// Get LRU entry (back of list)
		LRUEntry Entry = mLRUList.back();
		OutEvictedCoord = Entry.VirtualCoord;

		// Build physical location
		PhysicalTileLocation Location;
		LinearToTileIndex(Entry.LinearIndex, Location.AtlasX, Location.AtlasY);
		Location.PoolIndex = mPoolIndex;

		// Free the slot
		Free(Location);

		return true;
	}

	//-----------------------------------------------
	// Tile Data Upload
	//-----------------------------------------------

	void PhysicalTilePool::UploadTileData(
		const PhysicalTileLocation& Location, const VTTileData& TileData)
	{
		if (!Location.IsValid() || Location.PoolIndex != mPoolIndex)
			return;

		uint32 DestPixelX = Location.GetPixelX();
		uint32 DestPixelY = Location.GetPixelY();

		// Upload each layer
		for (uint8 Layer = 0; Layer < mDesc.NumLayers && Layer < (uint8)TileData.LayerData.size(); ++Layer)
		{
			RHITexture* AtlasTex = mAtlasTextures[Layer];
			if (!AtlasTex)
				continue;

			const std::vector<uint8>& LayerPixels = TileData.LayerData[Layer];
			if (LayerPixels.empty())
				continue;

			// TODO: Perform actual GPU upload using staging buffer and copy command
			// 1. Map staging buffer region
			// 2. Copy layer pixel data into staging buffer
			// 3. Record copy command: staging buffer -> atlas texture at (DestPixelX, DestPixelY)
			//
			// Pseudocode:
			// RHICopyTextureInfo CopyInfo;
			// CopyInfo.DestPosition = Vector3(DestPixelX, DestPixelY, 0);
			// CopyInfo.Size = Vector3(TileData.TilePixelSize, TileData.TilePixelSize, 1);
			// CommandContext->CopyBufferToTexture(mStagingBuffer, AtlasTex, CopyInfo);
		}

		// Update LRU entry with the virtual coordinate mapping
		uint32 LinearIndex = TileToLinearIndex(Location.AtlasX, Location.AtlasY);
		auto LRUIt = mLRUMap.find(LinearIndex);
		if (LRUIt != mLRUMap.end())
		{
			LRUIt->second->VirtualCoord = TileData.Coord;
		}
	}

	//-----------------------------------------------
	// GPU Resources
	//-----------------------------------------------

	RHITexture* PhysicalTilePool::GetAtlasTexture(uint8 LayerIndex) const
	{
		if (LayerIndex >= VTConstants::MaxLayers)
			return nullptr;
		return mAtlasTextures[LayerIndex];
	}

} // namespace Elaine
