#include "VirtualTexture/ElaineVTStreamingManager.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskScheduler.h"
#include "ElaineLogSystem.h"
#include "File/ElaineFileSystem.h"
#include <fstream>
#include <algorithm>

namespace Elaine
{
	VTStreamingManager::VTStreamingManager()
	{
	}

	VTStreamingManager::~VTStreamingManager()
	{
		Shutdown();
	}

	void VTStreamingManager::Initialize()
	{
		if (mInitialized)
			return;

		mShuttingDown = false;
		mInitialized = true;

		LogSystem::instance()->Log(LogLevel::Info,
			"VTStreamingManager: Initialized (maxBandwidth=%llu bytes/frame)",
			(unsigned long long)mMaxBandwidthPerFrame);
	}

	void VTStreamingManager::Shutdown()
	{
		if (!mInitialized)
			return;

		mShuttingDown = true;

		// Cancel all pending requests
		CancelAll();

		// Wait for active loads to finish
		// In a real implementation, we'd join/wait on all TaskGraph tasks
		while (mActiveLoadCount.load() > 0)
		{
			// Spin wait (in production, use proper synchronization)
		}

		// Clear completed tiles
		{
			std::lock_guard<std::mutex> Lock(mCompletedMutex);
			mCompletedTiles.clear();
		}

		mInitialized = false;

		LogSystem::instance()->Log(LogLevel::Info,
			"VTStreamingManager: Shutdown complete. Total bytes loaded: %llu",
			(unsigned long long)mTotalBytesLoaded.load());
	}

	//-----------------------------------------------
	// Request Submission
	//-----------------------------------------------

	void VTStreamingManager::SubmitRequest(const VTTileRequest& Request, const std::string& EvtFilePath)
	{
		if (mShuttingDown)
			return;

		std::lock_guard<std::mutex> Lock(mPendingMutex);
		RequestEntry Entry;
		Entry.Request = Request;
		Entry.EvtFilePath = EvtFilePath;
		mPendingQueue.push(Entry);
	}

	void VTStreamingManager::SubmitRequests(const std::vector<VTTileRequest>& Requests, const std::string& EvtFilePath)
	{
		if (mShuttingDown || Requests.empty())
			return;

		std::lock_guard<std::mutex> Lock(mPendingMutex);
		for (const auto& Req : Requests)
		{
			RequestEntry Entry;
			Entry.Request = Req;
			Entry.EvtFilePath = EvtFilePath;
			mPendingQueue.push(Entry);
		}
	}

	void VTStreamingManager::CancelRequest(const VTTileCoord& Coord)
	{
		std::lock_guard<std::mutex> Lock(mCancelMutex);
		mCancelledTiles.insert(Coord.Pack());
	}

	void VTStreamingManager::CancelAll()
	{
		// Clear the pending queue
		{
			std::lock_guard<std::mutex> Lock(mPendingMutex);
			while (!mPendingQueue.empty())
				mPendingQueue.pop();
		}

		// Clear cancel set
		{
			std::lock_guard<std::mutex> Lock(mCancelMutex);
			mCancelledTiles.clear();
		}
	}

	//-----------------------------------------------
	// Per-frame Processing
	//-----------------------------------------------

	uint32 VTStreamingManager::ProcessCompletedTiles(PhysicalTilePool* Pool, uint32 MaxUploadsPerFrame)
	{
		if (!Pool)
			return 0;

		std::vector<CompletedTile> TilesToProcess;

		// Grab completed tiles
		{
			std::lock_guard<std::mutex> Lock(mCompletedMutex);
			uint32 Count = std::min((uint32)mCompletedTiles.size(), MaxUploadsPerFrame);
			if (Count == 0)
				return 0;

			TilesToProcess.assign(
				mCompletedTiles.begin(),
				mCompletedTiles.begin() + Count);
			mCompletedTiles.erase(
				mCompletedTiles.begin(),
				mCompletedTiles.begin() + Count);
		}

		uint32 Uploaded = 0;

		for (auto& Completed : TilesToProcess)
		{
			// Check if this tile was cancelled
			{
				std::lock_guard<std::mutex> Lock(mCancelMutex);
				uint32 PackedCoord = Completed.TileData.Coord.Pack();
				if (mCancelledTiles.count(PackedCoord) > 0)
				{
					mCancelledTiles.erase(PackedCoord);
					continue; // Skip cancelled tiles
				}
			}

			// Allocate a physical tile slot
			PhysicalTileLocation Location = Pool->Allocate();
			if (!Location.IsValid())
			{
				// Pool is full, need eviction
				VTTileCoord EvictedCoord;
				if (Pool->EvictLRU(EvictedCoord))
				{
					// Update page table for evicted tile
					VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
					VirtualTextureSpace* Space = VTSystem->GetSpace(EvictedCoord.SpaceID);
					if (Space)
					{
						Space->RemovePageTableEntry(EvictedCoord);
					}

					// Try allocating again
					Location = Pool->Allocate();
				}
			}

			if (!Location.IsValid())
			{
				// Still can't allocate - skip this tile, put it back
				LogSystem::instance()->Log(LogLevel::Warning,
					"VTStreamingManager: Failed to allocate physical tile for (%u,%u) mip %u",
					Completed.TileData.Coord.X, Completed.TileData.Coord.Y,
					Completed.TileData.Coord.MipLevel);
				continue;
			}

			// Upload tile data to physical atlas
			Pool->UploadTileData(Location, Completed.TileData);

			// Touch the tile in LRU
			Pool->TouchTile(Location, VirtualTextureSystem::instance()->GetCurrentFrame());

			// Update page table entry
			VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
			VirtualTextureSpace* Space = VTSystem->GetSpace(Completed.TileData.Coord.SpaceID);
			if (Space)
			{
				PageTableEntry Entry;
				Entry.PhysicalLocation = Location;
				Entry.State = ETileState::Resident;
				Entry.FrameLoaded = VTSystem->GetCurrentFrame();
				Entry.FrameLastUsed = VTSystem->GetCurrentFrame();
				Space->SetPageTableEntry(Completed.TileData.Coord, Entry);
			}

			++Uploaded;
		}

		return Uploaded;
	}

	void VTStreamingManager::DispatchLoadTasks(uint32 MaxConcurrentLoads)
	{
		if (mShuttingDown)
			return;

		uint32 CurrentActive = mActiveLoadCount.load();
		if (CurrentActive >= MaxConcurrentLoads)
			return;

		uint32 SlotsAvailable = MaxConcurrentLoads - CurrentActive;
		uint32 Dispatched = 0;

		std::lock_guard<std::mutex> Lock(mPendingMutex);

		while (!mPendingQueue.empty() && Dispatched < SlotsAvailable)
		{
			RequestEntry Entry = mPendingQueue.top();
			mPendingQueue.pop();

			// Check if cancelled
			{
				std::lock_guard<std::mutex> CancelLock(mCancelMutex);
				if (mCancelledTiles.count(Entry.Request.Coord.Pack()) > 0)
				{
					mCancelledTiles.erase(Entry.Request.Coord.Pack());
					continue; // Skip cancelled
				}
			}

			// Dispatch async load task via TaskGraph
			mActiveLoadCount.fetch_add(1);

			// Capture by value for the async task
			VTTileRequest RequestCopy = Entry.Request;
			std::string FilePathCopy = Entry.EvtFilePath;

			auto LoadTask = std::make_shared<TaskGraph::GraphTask>(
				[this, RequestCopy, FilePathCopy]()
				{
					this->LoadTileTask(RequestCopy, FilePathCopy);
				},
				Elaine::NamedThread::AnyThread,
				"VT_TileLoad"
			);

			// Submit to task scheduler
			TaskGraph::TaskScheduler::instance()->SubmitTask(LoadTask);

			++Dispatched;
		}
	}

	//-----------------------------------------------
	// Query
	//-----------------------------------------------

	uint32 VTStreamingManager::GetPendingRequestCount() const
	{
		std::lock_guard<std::mutex> Lock(mPendingMutex);
		return (uint32)mPendingQueue.size();
	}

	uint32 VTStreamingManager::GetCompletedTileCount() const
	{
		std::lock_guard<std::mutex> Lock(mCompletedMutex);
		return (uint32)mCompletedTiles.size();
	}

	//-----------------------------------------------
	// Internal: Async Load Task
	//-----------------------------------------------

	void VTStreamingManager::LoadTileTask(const VTTileRequest& Request, const std::string& EvtFilePath)
	{
		if (mShuttingDown)
		{
			mActiveLoadCount.fetch_sub(1);
			return;
		}

		// Check cancellation
		{
			std::lock_guard<std::mutex> Lock(mCancelMutex);
			if (mCancelledTiles.count(Request.Coord.Pack()) > 0)
			{
				mActiveLoadCount.fetch_sub(1);
				return;
			}
		}

		// Read tile from file
		VTTileData TileData;
		bool Success = ReadTileFromFile(Request.Coord, EvtFilePath, TileData);

		if (Success)
		{
			// Add to completed queue
			CompletedTile Completed;
			Completed.TileData = std::move(TileData);
			Completed.OriginalRequest = Request;

			std::lock_guard<std::mutex> Lock(mCompletedMutex);
			mCompletedTiles.push_back(std::move(Completed));
		}
		else
		{
			LogSystem::instance()->Log(LogLevel::Warning,
				"VTStreamingManager: Failed to load tile (%u,%u) mip %u from '%s'",
				Request.Coord.X, Request.Coord.Y, Request.Coord.MipLevel,
				EvtFilePath.c_str());
		}

		mActiveLoadCount.fetch_sub(1);
	}

	bool VTStreamingManager::ReadTileFromFile(
		const VTTileCoord& Coord, const std::string& EvtFilePath,
		VTTileData& OutTileData)
	{
		// Open the .evt file
		std::ifstream File(EvtFilePath, std::ios::binary);
		if (!File.is_open())
			return false;

		// Read and validate header
		EVTFileHeader Header;
		File.read(reinterpret_cast<char*>(&Header), sizeof(Header));
		if (!Header.IsValid())
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VTStreamingManager: Invalid .evt file header in '%s'",
				EvtFilePath.c_str());
			return false;
		}

		// Read tile index table to find this tile
		File.seekg(Header.TileIndexTableOffset);

		// Search for the requested tile
		// Build packed coord without space ID (file doesn't store space)
		uint32 SearchPacked = (uint32(Coord.MipLevel & 0xF) << 24)
			| (uint32(Coord.Y & 0xFFF) << 12)
			| uint32(Coord.X & 0xFFF);

		EVTTileIndexEntry FoundEntry;
		bool Found = false;

		for (uint32 i = 0; i < Header.TotalTileCount; ++i)
		{
			EVTTileIndexEntry IndexEntry;
			File.read(reinterpret_cast<char*>(&IndexEntry), sizeof(IndexEntry));

			if (IndexEntry.PackedCoord == SearchPacked)
			{
				FoundEntry = IndexEntry;
				Found = true;
				break;
			}
		}

		if (!Found)
		{
			return false;
		}

		// Read tile data
		OutTileData.Coord = Coord;
		OutTileData.TilePixelSize = VTConstants::TileSizeWithBorder;
		OutTileData.LayerData.resize(Header.NumLayers);

		uint64 DataStart = Header.TileDataOffset + FoundEntry.DataOffset;
		File.seekg(DataStart);

		uint32 SizeToRead = (FoundEntry.CompressedSize > 0)
			? FoundEntry.CompressedSize
			: FoundEntry.UncompressedSize;

		for (uint8 Layer = 0; Layer < Header.NumLayers; ++Layer)
		{
			OutTileData.LayerData[Layer].resize(SizeToRead);
			File.read(reinterpret_cast<char*>(OutTileData.LayerData[Layer].data()), SizeToRead);

			if (!File.good())
			{
				LogSystem::instance()->Log(LogLevel::Error,
					"VTStreamingManager: Read error for tile (%u,%u) mip %u, layer %u",
					Coord.X, Coord.Y, Coord.MipLevel, Layer);
				return false;
			}

			// Decompress if needed
			if (FoundEntry.CompressedSize > 0)
			{
				// TODO: Decompress tile data (LZ4 or similar)
				// std::vector<uint8> Decompressed(FoundEntry.UncompressedSize);
				// LZ4_decompress(LayerData, Decompressed);
				// OutTileData.LayerData[Layer] = std::move(Decompressed);
				OutTileData.bCompressed = true;
			}
		}

		// Track bytes loaded
		mTotalBytesLoaded.fetch_add(SizeToRead * Header.NumLayers);

		return true;
	}

} // namespace Elaine
