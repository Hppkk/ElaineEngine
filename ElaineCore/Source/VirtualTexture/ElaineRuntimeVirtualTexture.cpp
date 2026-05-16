#include "ElainePrecompiledHeader.h"
#include "VirtualTexture/ElaineRuntimeVirtualTexture.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "VirtualTexture/ElainePhysicalTilePool.h"
#include "render/common/ElaineRHI.h"

#include <cmath>
#include <algorithm>

namespace Elaine
{
	//=========================================================================
	// Construction / Destruction
	//=========================================================================

	RuntimeVirtualTexture::RuntimeVirtualTexture(const VTSpaceDesc& InDesc, SceneManager* InSceneManager)
		: mDesc(InDesc)
		, mSceneManager(InSceneManager)
	{
		// Force RVT type
		mDesc.Type = EVirtualTextureType::RVT;

		// Compute mip levels if not specified
		if (mDesc.NumMipLevels == 0)
			mDesc.NumMipLevels = mDesc.ComputeNumMipLevels();
	}

	RuntimeVirtualTexture::~RuntimeVirtualTexture()
	{
		if (mInitialized)
			Shutdown();
	}

	//=========================================================================
	// Initialization
	//=========================================================================

	void RuntimeVirtualTexture::Initialize()
	{
		if (mInitialized) return;

		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return;

		// Register this RVT with the VT system
		mSpaceID = VTSystem->AllocateSpace(mDesc);
		VTSystem->RegisterVirtualTexture(this);

		mInitialized = true;
	}

	void RuntimeVirtualTexture::Shutdown()
	{
		if (!mInitialized) return;

		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (VTSystem)
		{
			VTSystem->UnregisterVirtualTexture(this);
			VTSystem->FreeSpace(mSpaceID);
		}

		mPendingRenders.clear();
		mDirtyTiles.clear();
		mInitialized = false;
	}

	//=========================================================================
	// IVirtualTexture Implementation
	//=========================================================================

	void RuntimeVirtualTexture::Update(uint32 FrameNumber)
	{
		// The main update logic:
		// 1. Check if any dirty tiles are also resident → mark for re-render
		// 2. Process any new tile requests from feedback
		// 3. Budget: limit pending renders per frame

		// Sort pending renders by priority
		std::sort(mPendingRenders.begin(), mPendingRenders.end());

		// Trim to budget
		if (mPendingRenders.size() > mMaxTileRendersPerFrame)
		{
			mPendingRenders.resize(mMaxTileRendersPerFrame);
		}
	}

	bool RuntimeVirtualTexture::IsTileResident(const VTTileCoord& Coord) const
	{
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return false;

		VirtualTextureSpace* Space = VTSystem->GetSpace(mSpaceID);
		if (!Space) return false;

		const PageTableEntry* Entry = Space->GetPageTableEntry(Coord);
		return Entry && Entry->IsResident();
	}

	RHITexture* RuntimeVirtualTexture::GetIndirectionTexture() const
	{
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return nullptr;

		VirtualTextureSpace* Space = VTSystem->GetSpace(mSpaceID);
		return Space ? Space->GetIndirectionTexture() : nullptr;
	}

	RHITexture* RuntimeVirtualTexture::GetPhysicalAtlasTexture(uint8 LayerIndex) const
	{
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem || LayerIndex >= VTConstants::MaxLayers) return nullptr;

		// RVT shares the physical pool with SVT (pool 0 by default)
		PhysicalTilePool* Pool = VTSystem->GetPhysicalPool(0);
		return Pool ? Pool->GetAtlasTexture(LayerIndex) : nullptr;
	}

	VTStatistics RuntimeVirtualTexture::GetStatistics() const
	{
		VTStatistics Stats;
		Stats.TotalVirtualPages = mDesc.GetTileCount(0); // mip 0 count
		Stats.TileUploadsThisFrame = (uint32)mPendingRenders.size();
		return Stats;
	}

	//=========================================================================
	// World-Space Mapping
	//=========================================================================

	void RuntimeVirtualTexture::GetWorldBounds(float& OutMinX, float& OutMinY,
		float& OutMaxX, float& OutMaxY) const
	{
		OutMinX = mDesc.WorldBoundsMinX;
		OutMinY = mDesc.WorldBoundsMinY;
		OutMaxX = mDesc.WorldBoundsMaxX;
		OutMaxY = mDesc.WorldBoundsMaxY;
	}

	void RuntimeVirtualTexture::SetWorldBounds(float MinX, float MinY, float MaxX, float MaxY)
	{
		mDesc.WorldBoundsMinX = MinX;
		mDesc.WorldBoundsMinY = MinY;
		mDesc.WorldBoundsMaxX = MaxX;
		mDesc.WorldBoundsMaxY = MaxY;

		// Changing bounds invalidates everything
		InvalidateAll();
	}

	void RuntimeVirtualTexture::WorldToVirtualUV(float WorldX, float WorldY,
		float& OutU, float& OutV) const
	{
		float RangeX = mDesc.WorldBoundsMaxX - mDesc.WorldBoundsMinX;
		float RangeY = mDesc.WorldBoundsMaxY - mDesc.WorldBoundsMinY;

		OutU = (RangeX > 0.0f) ? (WorldX - mDesc.WorldBoundsMinX) / RangeX : 0.0f;
		OutV = (RangeY > 0.0f) ? (WorldY - mDesc.WorldBoundsMinY) / RangeY : 0.0f;

		OutU = std::clamp(OutU, 0.0f, 1.0f);
		OutV = std::clamp(OutV, 0.0f, 1.0f);
	}

	void RuntimeVirtualTexture::GetTileWorldBounds(const VTTileCoord& Coord,
		float& OutMinX, float& OutMinY, float& OutMaxX, float& OutMaxY) const
	{
		float WorldRangeX = mDesc.WorldBoundsMaxX - mDesc.WorldBoundsMinX;
		float WorldRangeY = mDesc.WorldBoundsMaxY - mDesc.WorldBoundsMinY;

		uint32 TileCountX = mDesc.GetTileCountX(Coord.MipLevel);
		uint32 TileCountY = mDesc.GetTileCountY(Coord.MipLevel);

		float TileWorldSizeX = WorldRangeX / (float)TileCountX;
		float TileWorldSizeY = WorldRangeY / (float)TileCountY;

		// Include border pixels: extend the world bounds slightly
		// Border is TileBorderSize / TileSize fraction of a tile
		float BorderFractionX = (float)VTConstants::TileBorderSize / (float)VTConstants::TileSize;
		float BorderFractionY = (float)VTConstants::TileBorderSize / (float)VTConstants::TileSize;
		float BorderWorldX = TileWorldSizeX * BorderFractionX;
		float BorderWorldY = TileWorldSizeY * BorderFractionY;

		OutMinX = mDesc.WorldBoundsMinX + (float)Coord.X * TileWorldSizeX - BorderWorldX;
		OutMinY = mDesc.WorldBoundsMinY + (float)Coord.Y * TileWorldSizeY - BorderWorldY;
		OutMaxX = OutMinX + TileWorldSizeX + 2.0f * BorderWorldX;
		OutMaxY = OutMinY + TileWorldSizeY + 2.0f * BorderWorldY;
	}

	void RuntimeVirtualTexture::ComputeTileProjection(const VTTileCoord& Coord,
		float* OutViewMatrix4x4, float* OutProjMatrix4x4) const
	{
		float MinX, MinY, MaxX, MaxY;
		GetTileWorldBounds(Coord, MinX, MinY, MaxX, MaxY);

		float CenterX = (MinX + MaxX) * 0.5f;
		float CenterY = (MinY + MaxY) * 0.5f;
		float HalfWidth = (MaxX - MinX) * 0.5f;
		float HalfHeight = (MaxY - MinY) * 0.5f;

		// View matrix: top-down camera looking along -Y (or -Z depending on convention)
		// Here we use: Camera at (CenterX, Height, CenterY) looking down
		// This creates a simple identity-like view for top-down rendering
		// Using a high Y position to capture everything
		float CameraHeight = 10000.0f;

		// View matrix (column-major, row 0-3):
		// Looking down the -Y axis, with Z as the "up" direction in screen space
		//
		//  1  0  0  -CenterX
		//  0  0  1  -CameraHeight
		//  0 -1  0   CenterY
		//  0  0  0   1
		float View[16] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			-CenterX, CenterY, -CameraHeight, 1.0f
		};
		memcpy(OutViewMatrix4x4, View, sizeof(View));

		// Orthographic projection matrix
		// Maps world coords to [-1, 1] clip space
		float NearZ = 0.1f;
		float FarZ = CameraHeight * 2.0f;

		float Proj[16] = {
			1.0f / HalfWidth, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f / HalfHeight, 0.0f, 0.0f,
			0.0f, 0.0f, -2.0f / (FarZ - NearZ), 0.0f,
			0.0f, 0.0f, -(FarZ + NearZ) / (FarZ - NearZ), 1.0f
		};
		memcpy(OutProjMatrix4x4, Proj, sizeof(Proj));
	}

	//=========================================================================
	// Invalidation
	//=========================================================================

	void RuntimeVirtualTexture::InvalidateWorldRegion(float MinX, float MinY,
		float MaxX, float MaxY)
	{
		// For each mip level, find overlapping tiles and mark dirty
		for (uint8 Mip = 0; Mip < mDesc.NumMipLevels; ++Mip)
		{
			uint16 TileMinX, TileMinY, TileMaxX, TileMaxY;
			WorldRegionToTileRange(MinX, MinY, MaxX, MaxY, Mip,
				TileMinX, TileMinY, TileMaxX, TileMaxY);

			for (uint16 TY = TileMinY; TY <= TileMaxY; ++TY)
			{
				for (uint16 TX = TileMinX; TX <= TileMaxX; ++TX)
				{
					VTTileCoord Coord;
					Coord.X = TX;
					Coord.Y = TY;
					Coord.MipLevel = Mip;
					Coord.SpaceID = mSpaceID;

					mDirtyTiles.insert(Coord.Pack());

					// Also notify the VT system to invalidate
					VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
					if (VTSystem)
					{
						VTSystem->InvalidateTile(Coord);
					}
				}
			}
		}
	}

	void RuntimeVirtualTexture::InvalidateAll()
	{
		mDirtyTiles.clear();

		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return;

		for (uint8 Mip = 0; Mip < mDesc.NumMipLevels; ++Mip)
		{
			uint32 TilesX = mDesc.GetTileCountX(Mip);
			uint32 TilesY = mDesc.GetTileCountY(Mip);
			VTSystem->InvalidateRegion(mSpaceID, 0, 0,
				(uint16)(TilesX - 1), (uint16)(TilesY - 1), Mip);
		}
	}

	void RuntimeVirtualTexture::ClearPendingRenderRequests()
	{
		mPendingRenders.clear();
	}

	void RuntimeVirtualTexture::OnTileRenderComplete(const VTTileCoord& Coord,
		const PhysicalTileLocation& Location)
	{
		// Update page table
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return;

		VirtualTextureSpace* Space = VTSystem->GetSpace(mSpaceID);
		if (!Space) return;

		PageTableEntry Entry;
		Entry.PhysicalLocation = Location;
		Entry.State = ETileState::Resident;
		Entry.FrameLoaded = VTSystem->GetCurrentFrame();
		Entry.FrameLastUsed = Entry.FrameLoaded;
		Space->SetPageTableEntry(Coord, Entry);

		// Remove from dirty set
		mDirtyTiles.erase(Coord.Pack());

		++mTotalTilesRendered;
	}

	//=========================================================================
	// Tile Request Processing
	//=========================================================================

	void RuntimeVirtualTexture::ProcessTileRequest(const VTTileRequest& Request)
	{
		// Check if we already have a pending render for this tile
		for (const auto& Pending : mPendingRenders)
		{
			if (Pending.Coord == Request.Coord)
				return; // Already queued
		}

		// Allocate a physical tile slot
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem) return;

		PhysicalTilePool* Pool = VTSystem->GetPhysicalPool(0);
		if (!Pool) return;

		PhysicalTileLocation PhysLoc = Pool->AllocateTile();
		if (!PhysLoc.IsValid())
		{
			// Pool is full, need eviction
			// The VT system handles eviction in its main update loop
			return;
		}

		// Create render request
		RVTTileRenderRequest RenderReq;
		RenderReq.Coord = Request.Coord;
		RenderReq.PhysTarget = PhysLoc;
		RenderReq.FrameRequested = Request.FrameRequested;
		RenderReq.Priority = Request.Priority;

		// Compute world bounds for this tile
		GetTileWorldBounds(Request.Coord,
			RenderReq.WorldMinX, RenderReq.WorldMinY,
			RenderReq.WorldMaxX, RenderReq.WorldMaxY);

		mPendingRenders.push_back(RenderReq);
	}

	//=========================================================================
	// Private Helpers
	//=========================================================================

	void RuntimeVirtualTexture::WorldRegionToTileRange(
		float MinX, float MinY, float MaxX, float MaxY,
		uint8 MipLevel, uint16& OutTileMinX, uint16& OutTileMinY,
		uint16& OutTileMaxX, uint16& OutTileMaxY) const
	{
		float WorldRangeX = mDesc.WorldBoundsMaxX - mDesc.WorldBoundsMinX;
		float WorldRangeY = mDesc.WorldBoundsMaxY - mDesc.WorldBoundsMinY;

		if (WorldRangeX <= 0.0f || WorldRangeY <= 0.0f)
		{
			OutTileMinX = OutTileMinY = OutTileMaxX = OutTileMaxY = 0;
			return;
		}

		uint32 TileCountX = mDesc.GetTileCountX(MipLevel);
		uint32 TileCountY = mDesc.GetTileCountY(MipLevel);

		// Normalize to [0, 1] UV space
		float U0 = (MinX - mDesc.WorldBoundsMinX) / WorldRangeX;
		float V0 = (MinY - mDesc.WorldBoundsMinY) / WorldRangeY;
		float U1 = (MaxX - mDesc.WorldBoundsMinX) / WorldRangeX;
		float V1 = (MaxY - mDesc.WorldBoundsMinY) / WorldRangeY;

		// Convert to tile coordinates
		int32 TX0 = (int32)std::floor(U0 * TileCountX);
		int32 TY0 = (int32)std::floor(V0 * TileCountY);
		int32 TX1 = (int32)std::floor(U1 * TileCountX);
		int32 TY1 = (int32)std::floor(V1 * TileCountY);

		// Clamp to valid range
		OutTileMinX = (uint16)std::max(0, TX0);
		OutTileMinY = (uint16)std::max(0, TY0);
		OutTileMaxX = (uint16)std::min((int32)TileCountX - 1, TX1);
		OutTileMaxY = (uint16)std::min((int32)TileCountY - 1, TY1);
	}

} // namespace Elaine
