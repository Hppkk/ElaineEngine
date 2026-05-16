#include "VirtualTexture/ElaineVTFeedbackAnalyzer.h"
#include "ElaineLogSystem.h"
#include "ElaineTimer.h"
#include <algorithm>
#include <unordered_map>

namespace Elaine
{
	VTFeedbackAnalyzer::VTFeedbackAnalyzer()
	{
	}

	VTFeedbackAnalyzer::~VTFeedbackAnalyzer()
	{
		Shutdown();
	}

	void VTFeedbackAnalyzer::Initialize(uint32 ViewportWidth, uint32 ViewportHeight)
	{
		mFeedbackWidth = std::max(1u, ViewportWidth / VTConstants::FeedbackDownscaleFactor);
		mFeedbackHeight = std::max(1u, ViewportHeight / VTConstants::FeedbackDownscaleFactor);

		LogSystem::instance()->Log(LogLevel::Info,
			"VTFeedbackAnalyzer: Initializing feedback buffer %ux%u (viewport %ux%u, downscale %u)",
			mFeedbackWidth, mFeedbackHeight, ViewportWidth, ViewportHeight,
			VTConstants::FeedbackDownscaleFactor);

		// Create feedback render target (R32_UINT format)
		RHITextureDesc FeedbackDesc;
		FeedbackDesc.mExtent = Vector2((float)mFeedbackWidth, (float)mFeedbackHeight);
		FeedbackDesc.mFormat = PF_R32_UINT;
		FeedbackDesc.mNumMips = 1;
		FeedbackDesc.mFlags = TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource;
		FeedbackDesc.mDimension = TextureDimension::Texture2D;

		// TODO: Create feedback RT via RHI command context
		// mFeedbackRT = RHICreateTexture(FeedbackDesc);

		// Create readback staging texture (CPU-readable)
		RHITextureDesc ReadbackDesc = FeedbackDesc;
		ReadbackDesc.mFlags = TextureCreateFlags::CPUReadback;

		// TODO: Create readback texture via RHI command context
		// mReadbackTexture = RHICreateTexture(ReadbackDesc);
		// mReadbackTexturePrevFrame = RHICreateTexture(ReadbackDesc);

		LogSystem::instance()->Log(LogLevel::Info,
			"VTFeedbackAnalyzer: Initialization complete.");
	}

	void VTFeedbackAnalyzer::Shutdown()
	{
		mFeedbackRT = nullptr;
		mReadbackTexture = nullptr;
		mReadbackTexturePrevFrame = nullptr;
		mFeedbackWidth = 0;
		mFeedbackHeight = 0;
		mReadbackReady = false;
	}

	void VTFeedbackAnalyzer::ResizeIfNeeded(uint32 ViewportWidth, uint32 ViewportHeight)
	{
		uint32 NewWidth = std::max(1u, ViewportWidth / VTConstants::FeedbackDownscaleFactor);
		uint32 NewHeight = std::max(1u, ViewportHeight / VTConstants::FeedbackDownscaleFactor);

		if (NewWidth != mFeedbackWidth || NewHeight != mFeedbackHeight)
		{
			Shutdown();
			Initialize(ViewportWidth, ViewportHeight);
		}
	}

	void VTFeedbackAnalyzer::BeginFrame(uint32 FrameNumber)
	{
		mCurrentFrame = FrameNumber;

		// Swap readback buffers (double buffering)
		// The readback from the previous frame should now be available
		std::swap(mReadbackTexture, mReadbackTexturePrevFrame);

		// Initiate copy from the feedback RT (rendered last frame) to readback buffer
		if (mFeedbackRT && mReadbackTexture)
		{
			// TODO: Record copy command: feedback RT -> readback texture
			// CommandContext->CopyTexture(mFeedbackRT, mReadbackTexture);
			mReadbackReady = true;
		}
	}

	void VTFeedbackAnalyzer::AnalyzeFeedbackBuffer(std::vector<VTTileRequest>& OutRequests)
	{
		if (!mReadbackReady || !mReadbackTexturePrevFrame)
			return;

		// Read back the pixel data
		// TODO: Map the readback texture and get pixel data
		// const uint32* PixelData = (const uint32*)MapReadbackTexture(mReadbackTexturePrevFrame);
		const uint32* PixelData = nullptr; // Placeholder

		if (PixelData)
		{
			ProcessReadbackData(PixelData, mFeedbackWidth, mFeedbackHeight, OutRequests);
			// TODO: Unmap readback texture
			// UnmapReadbackTexture(mReadbackTexturePrevFrame);
		}

		mReadbackReady = false;
	}

	void VTFeedbackAnalyzer::ProcessReadbackData(
		const uint32* PixelData, uint32 Width, uint32 Height,
		std::vector<VTTileRequest>& OutRequests)
	{
		// Timer for profiling
		// auto StartTime = Timer::GetTimeMs();

		uint32 TotalPixels = Width * Height;

		// Count how many pixels reference each unique tile
		// Key: packed tile coord, Value: pixel count
		std::unordered_map<uint32, uint32> TilePixelCounts;
		TilePixelCounts.reserve(1024);

		mPageFaultCount = 0;

		for (uint32 i = 0; i < TotalPixels; ++i)
		{
			uint32 PackedCoord = PixelData[i];

			// Skip invalid/empty pixels (0xFFFFFFFF = no VT at this pixel)
			if (PackedCoord == VTConstants::InvalidPageEntry)
				continue;

			TilePixelCounts[PackedCoord]++;
		}

		mUniqueRequestCount = (uint32)TilePixelCounts.size();

		// Convert to tile requests with priority
		OutRequests.reserve(TilePixelCounts.size());

		for (const auto& Pair : TilePixelCounts)
		{
			VTTileCoord Coord = VTTileCoord::Unpack(Pair.first);

			if (!Coord.IsValid())
				continue;

			// Check if tile is already resident in the page table
			VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
			VirtualTextureSpace* Space = VTSystem->GetSpace(Coord.SpaceID);
			if (Space)
			{
				const PageTableEntry* Entry = Space->GetPageTableEntry(Coord);
				if (Entry && Entry->IsResident())
				{
					// Tile is resident, just touch it for LRU
					Space->TouchTile(Coord, mCurrentFrame);
					continue;
				}

				// Page fault - tile needs to be loaded
				++mPageFaultCount;
			}

			VTTileRequest Request;
			Request.Coord = Coord;
			Request.ScreenCoverage = (float)Pair.second / (float)TotalPixels;
			Request.Priority = ComputeTilePriority(Coord, Pair.second, TotalPixels);
			Request.FrameRequested = mCurrentFrame;

			OutRequests.push_back(Request);
		}

		// Sort by priority
		std::sort(OutRequests.begin(), OutRequests.end());

		// auto EndTime = Timer::GetTimeMs();
		// mAnalysisTimeMs = EndTime - StartTime;
	}

	ETilePriority VTFeedbackAnalyzer::ComputeTilePriority(
		const VTTileCoord& Coord, uint32 PixelCount, uint32 TotalPixels) const
	{
		float Coverage = (float)PixelCount / (float)TotalPixels;

		// High screen coverage = critical priority
		if (Coverage > 0.01f)  // More than 1% of screen
			return ETilePriority::Critical;
		else if (Coverage > 0.001f) // More than 0.1%
			return ETilePriority::High;
		else if (Coverage > 0.0001f) // More than 0.01%
			return ETilePriority::Medium;
		else
			return ETilePriority::Low;
	}

} // namespace Elaine
