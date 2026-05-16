#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "ElaineLogSystem.h"
#include <algorithm>
#include <cassert>

namespace Elaine
{
	//=============================================================================
	// VirtualTextureSystem Implementation
	//=============================================================================

	VirtualTextureSystem::VirtualTextureSystem()
	{
		mSpaces.fill(nullptr);
		mPhysicalPools.fill(nullptr);
	}

	VirtualTextureSystem::~VirtualTextureSystem()
	{
		Shutdown();
	}

	void VirtualTextureSystem::Initialize()
	{
		if (mInitialized)
			return;

		LOG_INFO( "VirtualTextureSystem: Initializing...");

		// Create default streaming manager (will be implemented in task-6)
		// mStreamingManager = new VTStreamingManager();

		mInitialized = true;

		LOG_INFO( "VirtualTextureSystem: Initialized successfully.");
	}

	void VirtualTextureSystem::Shutdown()
	{
		if (!mInitialized)
			return;

		LOG_INFO( "VirtualTextureSystem: Shutting down...");

		// Unregister all VTs
		mRegisteredVTs.clear();

		// Destroy spaces
		for (uint8 i = 0; i < VTConstants::MaxSpaces; ++i)
		{
			if (mSpaces[i])
			{
				mSpaces[i]->Shutdown();
				delete mSpaces[i];
				mSpaces[i] = nullptr;
			}
		}
		mNumAllocatedSpaces = 0;

		// Destroy physical pools (will be implemented in task-3)
		for (uint8 i = 0; i < VTConstants::MaxPhysicalPools; ++i)
		{
			if (mPhysicalPools[i])
			{
				// delete mPhysicalPools[i];
				mPhysicalPools[i] = nullptr;
			}
		}
		mNumPhysicalPools = 0;

		// Destroy streaming manager
		if (mStreamingManager)
		{
			// delete mStreamingManager;
			mStreamingManager = nullptr;
		}

		// Release feedback resources
		mFeedbackRT = nullptr;
		mFeedbackReadbackTexture = nullptr;

		mPendingRequests.clear();
		mStats = VTStatistics();

		mInitialized = false;

		LOG_INFO( "VirtualTextureSystem: Shutdown complete.");
	}

	void VirtualTextureSystem::Update(uint32 FrameNumber)
	{
		if (!mInitialized)
			return;

		mCurrentFrame = FrameNumber;
		mStats = VTStatistics(); // Reset per-frame stats

		// Step 1: Analyze feedback buffer from previous frame
		AnalyzeFeedback();

		// Step 2: Process tile requests (prioritize and submit to streaming/rendering)
		ProcessTileRequests();

		// Step 3: Update page tables with newly loaded tiles
		UpdatePageTables();

		// Step 4: Upload indirection texture updates to GPU
		UpdateIndirectionTextures();

		// Step 5: Evict unused tiles if pools are full
		EvictUnusedTiles();

		// Update per-VT state
		for (IVirtualTexture* VT : mRegisteredVTs)
		{
			if (VT)
				VT->Update(FrameNumber);
		}

		// Accumulate statistics
		for (uint8 i = 0; i < VTConstants::MaxSpaces; ++i)
		{
			if (mSpaces[i])
			{
				mStats.TotalVirtualPages += mSpaces[i]->GetTotalTileCount();
				mStats.ResidentPages += mSpaces[i]->GetResidentTileCount();
			}
		}
	}

	//-----------------------------------------------
	// Space Management
	//-----------------------------------------------

	uint8 VirtualTextureSystem::AllocateSpace(const VTSpaceDesc& Desc)
	{
		if (mNumAllocatedSpaces >= VTConstants::MaxSpaces)
		{
			LOG_ERROR(
				"VirtualTextureSystem: Cannot allocate space '{}', maximum ({}) reached.",
				Desc.Name.c_str(), VTConstants::MaxSpaces);
			return 0xFF;
		}

		// Find first free slot
		uint8 SpaceID = 0xFF;
		for (uint8 i = 0; i < VTConstants::MaxSpaces; ++i)
		{
			if (mSpaces[i] == nullptr)
			{
				SpaceID = i;
				break;
			}
		}

		if (SpaceID == 0xFF)
			return 0xFF;

		VirtualTextureSpace* Space = new VirtualTextureSpace(SpaceID, Desc);
		Space->Initialize();
		mSpaces[SpaceID] = Space;
		++mNumAllocatedSpaces;

		LOG_INFO(
			"VirtualTextureSystem: Allocated space '{}' with ID={}",
			Desc.Name.c_str(), SpaceID);

		return SpaceID;
	}

	void VirtualTextureSystem::FreeSpace(uint8 SpaceID)
	{
		if (SpaceID >= VTConstants::MaxSpaces || mSpaces[SpaceID] == nullptr)
			return;

		mSpaces[SpaceID]->Shutdown();
		delete mSpaces[SpaceID];
		mSpaces[SpaceID] = nullptr;
		--mNumAllocatedSpaces;

		LOG_INFO(
			"VirtualTextureSystem: Freed space ID={}", SpaceID);
	}

	VirtualTextureSpace* VirtualTextureSystem::GetSpace(uint8 SpaceID) const
	{
		if (SpaceID >= VTConstants::MaxSpaces)
			return nullptr;
		return mSpaces[SpaceID];
	}

	//-----------------------------------------------
	// Virtual Texture Registration
	//-----------------------------------------------

	void VirtualTextureSystem::RegisterVirtualTexture(IVirtualTexture* VT)
	{
		if (!VT) return;

		auto It = std::find(mRegisteredVTs.begin(), mRegisteredVTs.end(), VT);
		if (It == mRegisteredVTs.end())
		{
			mRegisteredVTs.push_back(VT);
			LOG_INFO(
				"VirtualTextureSystem: Registered VT (SpaceID={}, Type={})",
				VT->GetSpaceID(),
				VT->GetType() == EVirtualTextureType::SVT ? "SVT" : "RVT");
		}
	}

	void VirtualTextureSystem::UnregisterVirtualTexture(IVirtualTexture* VT)
	{
		if (!VT) return;

		auto It = std::find(mRegisteredVTs.begin(), mRegisteredVTs.end(), VT);
		if (It != mRegisteredVTs.end())
		{
			mRegisteredVTs.erase(It);
			LOG_INFO(
				"VirtualTextureSystem: Unregistered VT (SpaceID={})", VT->GetSpaceID());
		}
	}

	//-----------------------------------------------
	// Physical Pool Management
	//-----------------------------------------------

	uint8 VirtualTextureSystem::CreatePhysicalPool(const PhysicalTilePoolDesc& Desc)
	{
		if (mNumPhysicalPools >= VTConstants::MaxPhysicalPools)
		{
			LOG_ERROR(
				"VirtualTextureSystem: Cannot create physical pool, maximum ({}) reached.",
				VTConstants::MaxPhysicalPools);
			return 0xFF;
		}

		uint8 PoolIndex = mNumPhysicalPools;
		// PhysicalTilePool will be created in task-3
		// mPhysicalPools[PoolIndex] = new PhysicalTilePool(PoolIndex, Desc);
		++mNumPhysicalPools;

		LOG_INFO(
			"VirtualTextureSystem: Created physical pool {} (capacity={} tiles, {} layers)",
			PoolIndex, Desc.GetCapacity(), Desc.NumLayers);

		return PoolIndex;
	}

	PhysicalTilePool* VirtualTextureSystem::GetPhysicalPool(uint8 PoolIndex) const
	{
		if (PoolIndex >= VTConstants::MaxPhysicalPools)
			return nullptr;
		return mPhysicalPools[PoolIndex];
	}

	//-----------------------------------------------
	// Tile Operations
	//-----------------------------------------------

	void VirtualTextureSystem::RequestTile(const VTTileRequest& Request)
	{
		if (!Request.Coord.IsValid())
			return;

		// Check if tile is already resident or pending
		VirtualTextureSpace* Space = GetSpace(Request.Coord.SpaceID);
		if (!Space)
			return;

		const PageTableEntry* Entry = Space->GetPageTableEntry(Request.Coord);
		if (Entry && (Entry->IsResident() || Entry->IsPending()))
			return;

		// Add to pending requests with frame stamp
		VTTileRequest StampedRequest = Request;
		StampedRequest.FrameRequested = mCurrentFrame;
		mPendingRequests.push_back(StampedRequest);
	}

	bool VirtualTextureSystem::ForceLoadTile(const VTTileCoord& Coord)
	{
		// Synchronous load - to be implemented with streaming manager
		// For now, mark as requested with critical priority
		VTTileRequest Request;
		Request.Coord = Coord;
		Request.Priority = ETilePriority::Critical;
		Request.ScreenCoverage = 1.0f;
		RequestTile(Request);
		return false; // Not yet loaded synchronously
	}

	void VirtualTextureSystem::InvalidateTile(const VTTileCoord& Coord)
	{
		VirtualTextureSpace* Space = GetSpace(Coord.SpaceID);
		if (Space)
		{
			Space->RemovePageTableEntry(Coord);
		}
	}

	void VirtualTextureSystem::InvalidateRegion(
		uint8 SpaceID, uint16 MinX, uint16 MinY, uint16 MaxX, uint16 MaxY, uint8 MipLevel)
	{
		VirtualTextureSpace* Space = GetSpace(SpaceID);
		if (!Space)
			return;

		for (uint16 y = MinY; y <= MaxY; ++y)
		{
			for (uint16 x = MinX; x <= MaxX; ++x)
			{
				VTTileCoord Coord;
				Coord.SpaceID = SpaceID;
				Coord.MipLevel = MipLevel;
				Coord.X = x;
				Coord.Y = y;
				Space->RemovePageTableEntry(Coord);
			}
		}
	}

	//-----------------------------------------------
	// Feedback
	//-----------------------------------------------

	RHITexture* VirtualTextureSystem::GetFeedbackRenderTarget() const
	{
		return mFeedbackRT;
	}

	void VirtualTextureSystem::GetFeedbackBufferSize(uint32& OutWidth, uint32& OutHeight) const
	{
		OutWidth = mFeedbackWidth;
		OutHeight = mFeedbackHeight;
	}

	//-----------------------------------------------
	// Query
	//-----------------------------------------------

	VTStatistics VirtualTextureSystem::GetStatistics() const
	{
		return mStats;
	}

	//-----------------------------------------------
	// Internal Frame Update Steps
	//-----------------------------------------------

	void VirtualTextureSystem::AnalyzeFeedback()
	{
		// Will be implemented in task-5 (Feedback Buffer & GPU Analysis Pass)
		// 1. Readback the feedback render target from the previous frame
		// 2. Parse each pixel to extract VTTileCoord
		// 3. Deduplicate and generate tile requests
		// 4. Mark tiles as touched in the page table
	}

	void VirtualTextureSystem::ProcessTileRequests()
	{
		if (mPendingRequests.empty())
			return;

		// Sort by priority
		std::sort(mPendingRequests.begin(), mPendingRequests.end());

		// Limit requests per frame
		uint32 RequestsToProcess = std::min(
			(uint32)mPendingRequests.size(),
			VTConstants::MaxTileRequestsPerFrame);

		mStats.TileRequestsThisFrame = RequestsToProcess;

		// Submit to streaming manager (task-6) or RVT renderer (task-12)
		for (uint32 i = 0; i < RequestsToProcess; ++i)
		{
			const VTTileRequest& Request = mPendingRequests[i];
			VirtualTextureSpace* Space = GetSpace(Request.Coord.SpaceID);
			if (!Space)
				continue;

			// Mark tile as pending in page table
			PageTableEntry PendingEntry;
			PendingEntry.State = ETileState::Pending;
			PendingEntry.FrameLastUsed = mCurrentFrame;
			Space->SetPageTableEntry(Request.Coord, PendingEntry);

			// TODO: Submit to streaming manager for SVT
			// TODO: Submit to RVT renderer for RVT
		}

		// Remove processed requests, keep unprocessed ones for next frame
		if (RequestsToProcess < mPendingRequests.size())
		{
			mPendingRequests.erase(
				mPendingRequests.begin(),
				mPendingRequests.begin() + RequestsToProcess);
		}
		else
		{
			mPendingRequests.clear();
		}
	}

	void VirtualTextureSystem::UpdatePageTables()
	{
		// Will be enhanced when streaming manager delivers loaded tiles (task-6)
		// For each completed tile load:
		// 1. Allocate physical tile from pool
		// 2. Upload tile data to physical atlas
		// 3. Update page table entry to Resident with physical location
	}

	void VirtualTextureSystem::UpdateIndirectionTextures()
	{
		// Will be fully implemented in task-7 (Indirection Texture GPU Update)
		// For each space with dirty pages:
		// 1. Collect dirty entries
		// 2. Build updated indirection data
		// 3. Upload to GPU indirection texture

		for (uint8 i = 0; i < VTConstants::MaxSpaces; ++i)
		{
			if (mSpaces[i])
			{
				std::vector<std::pair<VTTileCoord, IndirectionEntry>> DirtyEntries;
				mSpaces[i]->GetDirtyEntries(DirtyEntries);

				if (!DirtyEntries.empty())
				{
					// TODO: Upload dirty entries to GPU indirection texture
					// This will be a staging buffer upload in task-7
				}

				mSpaces[i]->ClearDirtyFlags();
			}
		}
	}

	void VirtualTextureSystem::EvictUnusedTiles()
	{
		// Will be enhanced with PhysicalTilePool in task-3
		// For each physical pool that is full:
		// 1. Collect LRU sorted tiles across all spaces using this pool
		// 2. Evict least recently used tiles until enough space is available
		// 3. Update page table entries for evicted tiles
		// 4. Update indirection texture

		mStats.TilesEvictedThisFrame = 0;
	}

} // namespace Elaine
