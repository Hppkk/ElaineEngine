#pragma once
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include "VirtualTexture/ElainePhysicalTilePool.h"
#include <queue>
#include <mutex>
#include <atomic>

namespace Elaine
{
	/**
	 * VTStreamingManager handles asynchronous tile loading for SVT (Streaming Virtual Textures).
	 * 
	 * Architecture:
	 * 1. Main thread submits tile requests via SubmitRequest()
	 * 2. Requests are prioritized in a concurrent priority queue
	 * 3. Worker tasks (via TaskGraph) perform:
	 *    a. File I/O: Read tile data from .evt file
	 *    b. Decompress: Decode compressed tile data
	 *    c. Stage: Write decoded data to staging buffer
	 * 4. Main thread calls ProcessCompletedTiles() to:
	 *    a. Upload staged tiles to Physical Tile Pool
	 *    b. Update page table entries
	 *    c. Mark tiles as resident
	 * 
	 * Thread safety:
	 * - SubmitRequest() is thread-safe
	 * - ProcessCompletedTiles() must be called from main/render thread only
	 */
	class ElaineCoreExport VTStreamingManager
	{
	public:
		VTStreamingManager();
		~VTStreamingManager();

		/** Initialize the streaming system */
		void Initialize();

		/** Shutdown and cancel all pending operations */
		void Shutdown();

		//-----------------------------------------------
		// Request Submission
		//-----------------------------------------------

		/**
		 * Submit a tile load request. Thread-safe.
		 * @param Request  The tile request with priority
		 * @param EvtFilePath  Path to the .evt file containing this tile
		 */
		void SubmitRequest(const VTTileRequest& Request, const std::string& EvtFilePath);

		/**
		 * Submit multiple requests at once. Thread-safe.
		 */
		void SubmitRequests(const std::vector<VTTileRequest>& Requests, const std::string& EvtFilePath);

		/**
		 * Cancel a pending request if it hasn't started loading yet.
		 */
		void CancelRequest(const VTTileCoord& Coord);

		/**
		 * Cancel all pending requests.
		 */
		void CancelAll();

		//-----------------------------------------------
		// Per-frame Processing
		//-----------------------------------------------

		/**
		 * Process completed tile loads. Must be called from main thread.
		 * Uploads tile data to the physical pool and updates page tables.
		 * @param Pool  The physical tile pool to upload to
		 * @param MaxUploadsPerFrame  Maximum number of tiles to upload this frame
		 * @return Number of tiles uploaded
		 */
		uint32 ProcessCompletedTiles(PhysicalTilePool* Pool, uint32 MaxUploadsPerFrame = VTConstants::MaxTileUploadsPerFrame);

		/**
		 * Kick off loading tasks for pending requests.
		 * Should be called each frame to start new load operations.
		 * @param MaxConcurrentLoads  Maximum number of concurrent load tasks
		 */
		void DispatchLoadTasks(uint32 MaxConcurrentLoads = 8);

		//-----------------------------------------------
		// Query
		//-----------------------------------------------

		/** Number of requests waiting to be loaded */
		uint32 GetPendingRequestCount() const;

		/** Number of tiles currently being loaded */
		uint32 GetActiveLoadCount() const { return mActiveLoadCount.load(); }

		/** Number of completed tiles waiting to be uploaded */
		uint32 GetCompletedTileCount() const;

		/** Total bytes loaded this session */
		uint64 GetTotalBytesLoaded() const { return mTotalBytesLoaded.load(); }

		/** Get bandwidth usage in bytes per second (rolling average) */
		float GetBandwidthBytesPerSec() const { return mBandwidthBytesPerSec; }

		//-----------------------------------------------
		// Configuration
		//-----------------------------------------------

		/** Set maximum bandwidth for streaming (bytes per frame) */
		void SetMaxBandwidthPerFrame(uint64 MaxBytes) { mMaxBandwidthPerFrame = MaxBytes; }

	private:
		/** Internal load task function (runs on worker thread) */
		void LoadTileTask(const VTTileRequest& Request, const std::string& EvtFilePath);

		/** Read tile data from an .evt file */
		bool ReadTileFromFile(const VTTileCoord& Coord, const std::string& EvtFilePath,
			VTTileData& OutTileData);

	private:
		// Thread-safe pending request queue (priority queue)
		struct RequestEntry
		{
			VTTileRequest Request;
			std::string EvtFilePath;

			bool operator>(const RequestEntry& Other) const
			{
				return Request.Priority > Other.Request.Priority; // min-heap by priority
			}
		};

		std::priority_queue<RequestEntry, std::vector<RequestEntry>,
			std::greater<RequestEntry>> mPendingQueue;
		mutable std::mutex mPendingMutex;

		// Completed tiles ready for upload (thread-safe)
		struct CompletedTile
		{
			VTTileData TileData;
			VTTileRequest OriginalRequest;
		};
		std::vector<CompletedTile> mCompletedTiles;
		mutable std::mutex mCompletedMutex;

		// Active load tracking
		std::atomic<uint32> mActiveLoadCount{ 0 };

		// Cancellation set
		std::unordered_set<uint32> mCancelledTiles; // packed coords
		std::mutex mCancelMutex;

		// Statistics
		std::atomic<uint64> mTotalBytesLoaded{ 0 };
		float mBandwidthBytesPerSec = 0.0f;
		uint64 mMaxBandwidthPerFrame = 16 * 1024 * 1024; // 16 MB default

		// State
		bool mInitialized = false;
		bool mShuttingDown = false;
	};

} // namespace Elaine
