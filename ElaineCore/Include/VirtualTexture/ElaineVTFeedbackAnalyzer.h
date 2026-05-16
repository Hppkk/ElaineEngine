#pragma once
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"

namespace Elaine
{
	class RHICommandContext;

	/**
	 * VTFeedbackAnalyzer manages the feedback buffer system for virtual textures.
	 * 
	 * Architecture:
	 * - Frame N: Feedback Pass renders scene at low resolution, writing per-pixel
	 *   VT tile requests to a Feedback RT (R32_UINT format).
	 * - Frame N+1: CPU reads back the Feedback RT from frame N, analyzes it,
	 *   deduplicates tile requests, and generates a priority-sorted request list.
	 * 
	 * The feedback pass requires a special shader that:
	 * - Computes the virtual UV for each pixel
	 * - Determines the required mip level based on UV derivatives
	 * - Outputs the packed VTTileCoord to the feedback RT
	 */
	class ElaineCoreExport VTFeedbackAnalyzer
	{
	public:
		VTFeedbackAnalyzer();
		~VTFeedbackAnalyzer();

		/** Initialize feedback resources for a given viewport size */
		void Initialize(uint32 ViewportWidth, uint32 ViewportHeight);

		/** Release resources */
		void Shutdown();

		/** Resize feedback buffer if viewport changed */
		void ResizeIfNeeded(uint32 ViewportWidth, uint32 ViewportHeight);

		//-----------------------------------------------
		// Per-frame Operations
		//-----------------------------------------------

		/**
		 * Begin the feedback analysis for this frame.
		 * Initiates readback of the previous frame's feedback RT.
		 */
		void BeginFrame(uint32 FrameNumber);

		/**
		 * Analyze the readback data and generate tile requests.
		 * This is CPU-side work, called after readback is available.
		 * @param OutRequests  Output list of tile requests, sorted by priority
		 */
		void AnalyzeFeedbackBuffer(std::vector<VTTileRequest>& OutRequests);

		/**
		 * Get the feedback render target for the feedback rendering pass.
		 * This is the RT that the feedback shader writes to.
		 */
		RHITexture* GetFeedbackRenderTarget() const { return mFeedbackRT; }

		/** Get feedback buffer dimensions */
		uint32 GetFeedbackWidth() const { return mFeedbackWidth; }
		uint32 GetFeedbackHeight() const { return mFeedbackHeight; }

		//-----------------------------------------------
		// Statistics
		//-----------------------------------------------

		uint32 GetUniqueTileRequestCount() const { return mUniqueRequestCount; }
		uint32 GetPageFaultCount() const { return mPageFaultCount; }
		float GetAnalysisTimeMs() const { return mAnalysisTimeMs; }

	private:
		/** Process readback data from the feedback RT */
		void ProcessReadbackData(const uint32* PixelData, uint32 Width, uint32 Height,
			std::vector<VTTileRequest>& OutRequests);

		/** Compute tile priority based on screen coverage and other factors */
		ETilePriority ComputeTilePriority(const VTTileCoord& Coord, uint32 PixelCount,
			uint32 TotalPixels) const;

	private:
		// Feedback render target (low-res, R32_UINT)
		RHITexture* mFeedbackRT = nullptr;

		// Readback staging texture (CPU-visible copy of feedback RT)
		RHITexture* mReadbackTexture = nullptr;

		// Double-buffered: feedback RT from previous frame for readback
		RHITexture* mReadbackTexturePrevFrame = nullptr;

		// Feedback buffer dimensions (viewport / FeedbackDownscaleFactor)
		uint32 mFeedbackWidth = 0;
		uint32 mFeedbackHeight = 0;

		// Current frame number
		uint32 mCurrentFrame = 0;

		// Whether readback data is available for analysis
		bool mReadbackReady = false;

		// Statistics
		uint32 mUniqueRequestCount = 0;
		uint32 mPageFaultCount = 0;
		float mAnalysisTimeMs = 0.0f;
	};

	//=============================================================================
	// Feedback Shader Constants
	//=============================================================================

	/**
	 * Uniform buffer data passed to the feedback rendering shader.
	 * The shader uses this to compute which VT tile each pixel needs.
	 */
	struct VTFeedbackUniforms
	{
		// Virtual texture space parameters (per-space)
		float VirtualTextureSizeX;   // Virtual texture width at mip 0
		float VirtualTextureSizeY;   // Virtual texture height at mip 0
		float InvVirtualTextureSizeX;// 1.0 / VirtualTextureSizeX
		float InvVirtualTextureSizeY;// 1.0 / VirtualTextureSizeY

		float TileSize;              // Tile size in pixels (128)
		float InvTileSize;           // 1.0 / TileSize
		float MaxMipLevel;           // Maximum mip level
		uint32 SpaceID;             // VT space ID

		// For RVT: world-space to UV mapping
		float WorldBoundsMinX;
		float WorldBoundsMinY;
		float WorldBoundsRangeInvX;  // 1.0 / (MaxX - MinX)
		float WorldBoundsRangeInvY;  // 1.0 / (MaxY - MinY)

		// Screen info for mip level computation
		float ScreenWidth;
		float ScreenHeight;
		float FeedbackDownscale;     // FeedbackDownscaleFactor
		float Padding;
	};

} // namespace Elaine
