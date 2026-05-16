#pragma once
#include "Common/ElaineCorePrerequirements.h"
#include "Common/ElaineStdRequirements.h"
#include "render/common/ElaineRHIProtocol.h"
#include "Common/ElaineSingleton.h"

namespace Elaine
{
	//=============================================================================
	// Virtual Texture Constants
	//=============================================================================
	namespace VTConstants
	{
		/** Size of a single tile in pixels (excluding border) */
		static constexpr uint32 TileSize = 128;

		/** Border pixels on each side for filtering (prevents seams) */
		static constexpr uint32 TileBorderSize = 4;

		/** Total tile size including borders on both sides */
		static constexpr uint32 TileSizeWithBorder = TileSize + TileBorderSize * 2;

		/** Physical texture atlas size in tiles per dimension */
		static constexpr uint32 PhysicalAtlasSizeInTiles = 32;

		/** Physical texture atlas size in pixels per dimension */
		static constexpr uint32 PhysicalAtlasSize = PhysicalAtlasSizeInTiles * TileSizeWithBorder;

		/** Maximum number of mip levels for a virtual texture */
		static constexpr uint32 MaxMipLevels = 12;

		/** Maximum number of virtual texture spaces */
		static constexpr uint32 MaxSpaces = 16;

		/** Maximum number of physical tile pools */
		static constexpr uint32 MaxPhysicalPools = 8;

		/** Maximum number of layers per virtual texture (Albedo, Normal, etc.) */
		static constexpr uint32 MaxLayers = 4;

		/** Maximum number of tile requests to process per frame */
		static constexpr uint32 MaxTileRequestsPerFrame = 128;

		/** Maximum number of tile uploads per frame */
		static constexpr uint32 MaxTileUploadsPerFrame = 64;

		/** Feedback buffer downscale factor */
		static constexpr uint32 FeedbackDownscaleFactor = 8;

		/** Invalid page table entry value */
		static constexpr uint32 InvalidPageEntry = 0xFFFFFFFF;

		/** Invalid physical tile index */
		static constexpr uint16 InvalidPhysicalTileIndex = 0xFFFF;
	}

	//=============================================================================
	// Virtual Texture Enums
	//=============================================================================

	/** Type of virtual texture */
	enum class EVirtualTextureType : uint8
	{
		/** Streaming Virtual Texture - tiles loaded from disk */
		SVT,
		/** Runtime Virtual Texture - tiles rendered on demand */
		RVT,
	};

	/** Layer types that a virtual texture can contain */
	enum class EVTLayerType : uint8
	{
		BaseColor = 0,
		Normal = 1,
		Roughness_Metallic_AO = 2,  // packed R=Roughness, G=Metallic, B=AO
		Emissive = 3,
		Count
	};

	/** State of a tile in the page table */
	enum class ETileState : uint8
	{
		/** Tile not loaded, not requested */
		NotLoaded = 0,
		/** Tile load requested, waiting in queue */
		Pending,
		/** Tile is currently being loaded (I/O or decode in progress) */
		Loading,
		/** Tile is being uploaded to GPU */
		Uploading,
		/** Tile is resident in physical memory and mapped */
		Resident,
		/** Tile has been evicted from physical memory */
		Evicted,
	};

	/** Priority for tile loading */
	enum class ETilePriority : uint8
	{
		Critical = 0,   // On-screen, current mip level
		High = 1,       // On-screen, one mip level higher detail
		Medium = 2,     // On-screen, pre-fetch
		Low = 3,        // Off-screen, predicted
		Background = 4, // Background streaming
	};

	//=============================================================================
	// Forward Declarations
	//=============================================================================
	class VirtualTextureSystem;
	class VirtualTextureSpace;
	class VirtualTexturePageTable;
	class PhysicalTilePool;
	class VTFeedbackAnalyzer;
	class VTStreamingManager;
	class VTIndirectionTexture;
	class RuntimeVirtualTexture;

	//=============================================================================
	// Core Data Structures
	//=============================================================================

	/** 
	 * Identifies a specific tile in the virtual texture hierarchy.
	 * Encodes both the mip level and the 2D coordinate within that mip.
	 */
	struct VTTileCoord
	{
		uint16 X = 0;       // Tile X coordinate within the mip level
		uint16 Y = 0;       // Tile Y coordinate within the mip level
		uint8  MipLevel = 0; // Mip level (0 = highest detail)
		uint8  SpaceID = 0;  // Virtual texture space this tile belongs to

		bool operator==(const VTTileCoord& Other) const
		{
			return X == Other.X && Y == Other.Y && MipLevel == Other.MipLevel && SpaceID == Other.SpaceID;
		}

		bool operator!=(const VTTileCoord& Other) const
		{
			return !(*this == Other);
		}

		bool IsValid() const
		{
			return SpaceID < VTConstants::MaxSpaces && MipLevel < VTConstants::MaxMipLevels;
		}

		/** Pack into a single 32-bit value for use in hash maps and feedback buffers */
		uint32 Pack() const
		{
			// Layout: [SpaceID:4][MipLevel:4][Y:12][X:12]
			return (uint32(SpaceID & 0xF) << 28)
				| (uint32(MipLevel & 0xF) << 24)
				| (uint32(Y & 0xFFF) << 12)
				| uint32(X & 0xFFF);
		}

		/** Unpack from a 32-bit packed value */
		static VTTileCoord Unpack(uint32 Packed)
		{
			VTTileCoord Result;
			Result.SpaceID  = uint8((Packed >> 28) & 0xF);
			Result.MipLevel = uint8((Packed >> 24) & 0xF);
			Result.Y        = uint16((Packed >> 12) & 0xFFF);
			Result.X        = uint16(Packed & 0xFFF);
			return Result;
		}
	};

	/** Hash function for VTTileCoord */
	struct VTTileCoordHash
	{
		size_t operator()(const VTTileCoord& Coord) const
		{
			return std::hash<uint32>()(Coord.Pack());
		}
	};

	/**
	 * Represents a tile's position in the physical texture atlas.
	 */
	struct PhysicalTileLocation
	{
		uint16 AtlasX = VTConstants::InvalidPhysicalTileIndex;  // Tile X in the atlas grid
		uint16 AtlasY = VTConstants::InvalidPhysicalTileIndex;  // Tile Y in the atlas grid
		uint8  PoolIndex = 0;                                     // Which physical pool

		bool IsValid() const
		{
			return AtlasX != VTConstants::InvalidPhysicalTileIndex 
				&& AtlasY != VTConstants::InvalidPhysicalTileIndex;
		}

		/** Get the pixel offset in the physical atlas texture */
		uint32 GetPixelX() const { return AtlasX * VTConstants::TileSizeWithBorder; }
		uint32 GetPixelY() const { return AtlasY * VTConstants::TileSizeWithBorder; }

		bool operator==(const PhysicalTileLocation& Other) const
		{
			return AtlasX == Other.AtlasX && AtlasY == Other.AtlasY && PoolIndex == Other.PoolIndex;
		}
	};

	/**
	 * An entry in the page table, mapping a virtual tile to its physical location.
	 * Also stored in the indirection texture for GPU lookup.
	 */
	struct PageTableEntry
	{
		PhysicalTileLocation PhysicalLocation;
		ETileState State = ETileState::NotLoaded;
		uint32 FrameLastUsed = 0;    // Frame number when this tile was last accessed
		uint32 FrameLoaded = 0;      // Frame number when this tile was loaded

		bool IsResident() const { return State == ETileState::Resident; }
		bool IsPending() const { return State == ETileState::Pending || State == ETileState::Loading || State == ETileState::Uploading; }
	};

	/**
	 * Data written to the indirection texture.
	 * Each texel of the indirection texture maps a virtual page to its physical atlas location.
	 * Format: R16G16B16A16_UINT
	 *   R = PhysicalTileX
	 *   G = PhysicalTileY  
	 *   B = packed: [PoolIndex:4][MipBias:4]  
	 *   A = flags (IsResident, etc.)
	 */
	struct IndirectionEntry
	{
		uint16 PhysicalTileX = VTConstants::InvalidPhysicalTileIndex;
		uint16 PhysicalTileY = VTConstants::InvalidPhysicalTileIndex;
		uint16 PackedPoolMip = 0;  // [PoolIndex:8][MipBias:8]
		uint16 Flags = 0;

		static IndirectionEntry Create(const PhysicalTileLocation& Location, uint8 MipBias, bool bResident)
		{
			IndirectionEntry Entry;
			Entry.PhysicalTileX = Location.AtlasX;
			Entry.PhysicalTileY = Location.AtlasY;
			Entry.PackedPoolMip = (uint16(Location.PoolIndex) << 8) | uint16(MipBias);
			Entry.Flags = bResident ? 1 : 0;
			return Entry;
		}

		static IndirectionEntry Invalid()
		{
			return IndirectionEntry();
		}
	};

	/**
	 * Describes a tile request from the feedback analysis.
	 */
	struct VTTileRequest
	{
		VTTileCoord Coord;
		ETilePriority Priority = ETilePriority::Medium;
		float ScreenCoverage = 0.0f;  // How much screen area this tile covers (for priority sorting)
		uint32 FrameRequested = 0;

		bool operator<(const VTTileRequest& Other) const
		{
			// Higher priority (lower enum value) first, then by screen coverage
			if (Priority != Other.Priority)
				return Priority < Other.Priority;
			return ScreenCoverage > Other.ScreenCoverage;
		}
	};

	/**
	 * Tile data loaded from disk, ready to be uploaded to GPU.
	 */
	struct VTTileData
	{
		VTTileCoord Coord;
		std::vector<std::vector<uint8>> LayerData;  // Per-layer pixel data
		uint32 TilePixelSize = VTConstants::TileSizeWithBorder;
		PixelFormat Format = PF_R8G8B8A8;
		bool bCompressed = false;
	};

	//=============================================================================
	// Virtual Texture Space Descriptor
	//=============================================================================

	/**
	 * Describes the layout and properties of a virtual texture space.
	 * A space defines the virtual address range for a set of related virtual textures.
	 */
	struct VTSpaceDesc
	{
		/** Human-readable name for debugging */
		std::string Name;

		/** Virtual texture size at mip 0 (must be power of 2) */
		uint32 VirtualSizeX = 4096;
		uint32 VirtualSizeY = 4096;

		/** Number of mip levels (computed from virtual size and tile size if 0) */
		uint8 NumMipLevels = 0;

		/** Number of layers (Albedo, Normal, etc.) */
		uint8 NumLayers = 1;

		/** Pixel format for each layer */
		PixelFormat LayerFormats[VTConstants::MaxLayers] = { PF_R8G8B8A8, PF_R8G8B8A8, PF_R8G8B8A8, PF_R8G8B8A8 };

		/** Type of virtual texture */
		EVirtualTextureType Type = EVirtualTextureType::SVT;

		/** For RVT: World-space bounds */
		float WorldBoundsMinX = 0.0f;
		float WorldBoundsMinY = 0.0f;
		float WorldBoundsMaxX = 1000.0f;
		float WorldBoundsMaxY = 1000.0f;

		/** Compute number of mip levels based on virtual size */
		uint8 ComputeNumMipLevels() const
		{
			if (NumMipLevels > 0) return NumMipLevels;
			uint32 MaxDim = (VirtualSizeX > VirtualSizeY) ? VirtualSizeX : VirtualSizeY;
			uint32 TilesAtMip0 = MaxDim / VTConstants::TileSize;
			uint8 Mips = 0;
			while ((TilesAtMip0 >> Mips) > 0 && Mips < VTConstants::MaxMipLevels)
				++Mips;
			return Mips > 0 ? Mips : 1;
		}

		/** Get number of tiles in X at a given mip level */
		uint32 GetTileCountX(uint8 MipLevel) const
		{
			uint32 WidthAtMip = VirtualSizeX >> MipLevel;
			if (WidthAtMip < VTConstants::TileSize) WidthAtMip = VTConstants::TileSize;
			return (WidthAtMip + VTConstants::TileSize - 1) / VTConstants::TileSize;
		}

		/** Get number of tiles in Y at a given mip level */
		uint32 GetTileCountY(uint8 MipLevel) const
		{
			uint32 HeightAtMip = VirtualSizeY >> MipLevel;
			if (HeightAtMip < VTConstants::TileSize) HeightAtMip = VTConstants::TileSize;
			return (HeightAtMip + VTConstants::TileSize - 1) / VTConstants::TileSize;
		}

		/** Get total number of tiles at a given mip level */
		uint32 GetTileCount(uint8 MipLevel) const
		{
			return GetTileCountX(MipLevel) * GetTileCountY(MipLevel);
		}
	};

	//=============================================================================
	// Physical Tile Pool Descriptor
	//=============================================================================

	struct PhysicalTilePoolDesc
	{
		/** Size of the atlas in tiles per dimension */
		uint32 AtlasSizeInTiles = VTConstants::PhysicalAtlasSizeInTiles;

		/** Number of layers in this pool */
		uint8 NumLayers = 1;

		/** Format for each layer */
		PixelFormat LayerFormats[VTConstants::MaxLayers] = { PF_R8G8B8A8 };

		/** Total capacity in tiles */
		uint32 GetCapacity() const { return AtlasSizeInTiles * AtlasSizeInTiles; }
	};

	//=============================================================================
	// Feedback Buffer Data Format
	//=============================================================================

	/**
	 * Data format for the feedback buffer.
	 * Each pixel in the feedback render target stores which virtual tile
	 * is needed at that screen location.
	 * 
	 * Format: R32_UINT
	 * Packed as VTTileCoord::Pack()
	 */
	struct VTFeedbackPixel
	{
		uint32 PackedTileCoord;

		VTTileCoord Unpack() const
		{
			return VTTileCoord::Unpack(PackedTileCoord);
		}
	};

	//=============================================================================
	// Virtual Texture Statistics
	//=============================================================================

	struct VTStatistics
	{
		// Page table stats
		uint32 TotalVirtualPages = 0;
		uint32 ResidentPages = 0;
		uint32 PendingPages = 0;

		// Physical pool stats
		uint32 PhysicalPoolCapacity = 0;
		uint32 PhysicalPoolUsed = 0;
		float  PhysicalPoolUtilization = 0.0f;

		// Streaming stats
		uint32 TileRequestsThisFrame = 0;
		uint32 TileUploadsThisFrame = 0;
		uint64 BytesStreamedThisFrame = 0;
		uint64 TotalBytesStreamed = 0;

		// Eviction stats
		uint32 TilesEvictedThisFrame = 0;

		// Feedback stats
		uint32 UniqueTilesRequested = 0;
		uint32 PageFaultsThisFrame = 0;

		// Timing
		float FeedbackAnalysisTimeMs = 0.0f;
		float TileUploadTimeMs = 0.0f;
		float IndirectionUpdateTimeMs = 0.0f;
	};

	//=============================================================================
	// IVirtualTexture Interface
	//=============================================================================

	/**
	 * Interface for a virtual texture instance.
	 * Both SVT (streaming) and RVT (runtime) implement this.
	 */
	class ElaineCoreExport IVirtualTexture
	{
	public:
		virtual ~IVirtualTexture() = default;

		/** Get the virtual texture space descriptor */
		virtual const VTSpaceDesc& GetSpaceDesc() const = 0;

		/** Get the space ID assigned by the system */
		virtual uint8 GetSpaceID() const = 0;

		/** Get the type (SVT or RVT) */
		virtual EVirtualTextureType GetType() const = 0;

		/** Called each frame to update the virtual texture */
		virtual void Update(uint32 FrameNumber) = 0;

		/** Check if a specific tile is resident */
		virtual bool IsTileResident(const VTTileCoord& Coord) const = 0;

		/** Get the indirection texture for GPU sampling */
		virtual RHITexture* GetIndirectionTexture() const = 0;

		/** Get the physical atlas texture for a given layer */
		virtual RHITexture* GetPhysicalAtlasTexture(uint8 LayerIndex) const = 0;

		/** Get statistics */
		virtual VTStatistics GetStatistics() const = 0;
	};

	//=============================================================================
	// Virtual Texture System - Singleton Manager
	//=============================================================================

	/**
	 * Central manager for all virtual texture operations.
	 * Manages spaces, physical pools, streaming, and the per-frame update cycle.
	 * 
	 * Frame update flow:
	 * 1. AnalyzeFeedback() - Read back and analyze feedback buffer from previous frame
	 * 2. ProcessTileRequests() - Prioritize and submit tile load/render requests
	 * 3. UpdatePageTable() - Update page table with newly loaded tiles
	 * 4. UpdateIndirectionTextures() - Upload indirection data to GPU
	 * 5. RenderFeedback() - Render feedback pass for this frame (feeds into next frame)
	 */
	class ElaineCoreExport VirtualTextureSystem : public Singleton<VirtualTextureSystem>
	{
	public:
		VirtualTextureSystem();
		~VirtualTextureSystem();

		/** Initialize the VT system with RHI resources */
		void Initialize();

		/** Shutdown and release all resources */
		void Shutdown();

		/** Called once per frame from the render pipeline */
		void Update(uint32 FrameNumber);

		//-----------------------------------------------
		// Space Management
		//-----------------------------------------------

		/** Register a new virtual texture space. Returns the space ID. */
		uint8 AllocateSpace(const VTSpaceDesc& Desc);

		/** Unregister a virtual texture space */
		void FreeSpace(uint8 SpaceID);

		/** Get a space by ID */
		VirtualTextureSpace* GetSpace(uint8 SpaceID) const;

		//-----------------------------------------------
		// Virtual Texture Registration
		//-----------------------------------------------

		/** Register a virtual texture instance */
		void RegisterVirtualTexture(IVirtualTexture* VT);

		/** Unregister a virtual texture instance */
		void UnregisterVirtualTexture(IVirtualTexture* VT);

		//-----------------------------------------------
		// Physical Pool Management
		//-----------------------------------------------

		/** Create a physical tile pool */
		uint8 CreatePhysicalPool(const PhysicalTilePoolDesc& Desc);

		/** Get a physical pool by index */
		PhysicalTilePool* GetPhysicalPool(uint8 PoolIndex) const;

		//-----------------------------------------------
		// Tile Operations
		//-----------------------------------------------

		/** Submit a tile request (called by feedback analyzer or manual request) */
		void RequestTile(const VTTileRequest& Request);

		/** Force load a tile synchronously (use sparingly) */
		bool ForceLoadTile(const VTTileCoord& Coord);

		/** Invalidate a tile (force re-load or re-render) */
		void InvalidateTile(const VTTileCoord& Coord);

		/** Invalidate all tiles in a region (useful for RVT when content changes) */
		void InvalidateRegion(uint8 SpaceID, uint16 MinX, uint16 MinY, uint16 MaxX, uint16 MaxY, uint8 MipLevel);

		//-----------------------------------------------
		// Feedback
		//-----------------------------------------------

		/** Get the feedback render target for the feedback pass */
		RHITexture* GetFeedbackRenderTarget() const;

		/** Get the feedback buffer size */
		void GetFeedbackBufferSize(uint32& OutWidth, uint32& OutHeight) const;

		//-----------------------------------------------
		// Query & Debug
		//-----------------------------------------------

		/** Get overall system statistics */
		VTStatistics GetStatistics() const;

		/** Is the system initialized? */
		bool IsInitialized() const { return mInitialized; }

		/** Enable/disable debug visualization */
		void SetDebugVisualizationEnabled(bool bEnabled) { mDebugVisualization = bEnabled; }
		bool IsDebugVisualizationEnabled() const { return mDebugVisualization; }

		/** Get current frame number */
		uint32 GetCurrentFrame() const { return mCurrentFrame; }

	private:
		/** Internal frame update steps */
		void AnalyzeFeedback();
		void ProcessTileRequests();
		void UpdatePageTables();
		void UpdateIndirectionTextures();
		void EvictUnusedTiles();

	private:
		bool mInitialized = false;
		bool mDebugVisualization = false;
		uint32 mCurrentFrame = 0;

		// Registered spaces
		std::array<VirtualTextureSpace*, VTConstants::MaxSpaces> mSpaces;
		uint8 mNumAllocatedSpaces = 0;

		// Registered virtual textures
		std::vector<IVirtualTexture*> mRegisteredVTs;

		// Physical tile pools
		std::array<PhysicalTilePool*, VTConstants::MaxPhysicalPools> mPhysicalPools;
		uint8 mNumPhysicalPools = 0;

		// Tile request queue (accumulated during feedback analysis)
		std::vector<VTTileRequest> mPendingRequests;

		// Feedback
		RHITexture* mFeedbackRT = nullptr;
		RHITexture* mFeedbackReadbackTexture = nullptr;
		uint32 mFeedbackWidth = 0;
		uint32 mFeedbackHeight = 0;

		// Streaming manager (async tile loading)
		VTStreamingManager* mStreamingManager = nullptr;

		// Statistics
		VTStatistics mStats;
	};

	//=============================================================================
	// EVT File Format Header
	//=============================================================================

	/**
	 * Header for the Elaine Virtual Texture (.evt) file format.
	 * Used by the offline baking tool and streaming loader.
	 */
	struct EVTFileHeader
	{
		static constexpr uint32 MAGIC = 0x45565400;  // "EVT\0"
		static constexpr uint32 VERSION = 1;

		uint32 Magic = MAGIC;
		uint32 Version = VERSION;

		uint32 VirtualSizeX = 0;
		uint32 VirtualSizeY = 0;
		uint8  NumMipLevels = 0;
		uint8  NumLayers = 0;
		uint8  TileSize = VTConstants::TileSize;      // Without border
		uint8  TileBorderSize = VTConstants::TileBorderSize;

		PixelFormat LayerFormats[VTConstants::MaxLayers] = {};

		/** Total number of tiles in the file */
		uint32 TotalTileCount = 0;

		/** Offset to the tile index table (array of EVTTileIndexEntry) */
		uint64 TileIndexTableOffset = 0;

		/** Offset to the tile data blob */
		uint64 TileDataOffset = 0;

		bool IsValid() const
		{
			return Magic == MAGIC && Version == VERSION;
		}
	};

	/**
	 * Entry in the tile index table within an .evt file.
	 * Maps (MipLevel, TileX, TileY) -> offset and size in the data blob.
	 */
	struct EVTTileIndexEntry
	{
		uint32 PackedCoord;     // VTTileCoord::Pack() (without SpaceID)
		uint64 DataOffset;      // Offset from TileDataOffset
		uint32 CompressedSize;  // Size of compressed data (0 if uncompressed)
		uint32 UncompressedSize;// Size of uncompressed data per layer
	};

} // namespace Elaine
