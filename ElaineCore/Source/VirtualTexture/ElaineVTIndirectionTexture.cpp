#include "VirtualTexture/ElaineVTIndirectionTexture.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineLogSystem.h"
#include <algorithm>
#include <cstring>

namespace Elaine
{
	// Size of one IndirectionEntry in bytes (R16G16B16A16_UINT = 8 bytes)
	static constexpr uint32 IndirectionEntryBytes = sizeof(IndirectionEntry);

	VTIndirectionTexture::VTIndirectionTexture()
	{
	}

	VTIndirectionTexture::~VTIndirectionTexture()
	{
		Shutdown();
	}

	void VTIndirectionTexture::Initialize(VirtualTextureSpace* InSpace, RHICommandContext* InCmdCtx)
	{
		if (mInitialized)
			return;

		mSpace = InSpace;
		mCmdCtx = InCmdCtx;

		if (!mSpace || !mCmdCtx)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VTIndirectionTexture: Cannot initialize with null space or command context");
			return;
		}

		const VTSpaceDesc& Desc = mSpace->GetDesc();
		uint8 NumMips = mSpace->GetNumMipLevels();

		// Compute per-mip staging offsets
		mMipStagingOffsets.resize(NumMips);
		uint32 TotalSize = 0;
		for (uint8 Mip = 0; Mip < NumMips; ++Mip)
		{
			mMipStagingOffsets[Mip] = TotalSize;
			uint32 TilesX = Desc.GetTileCountX(Mip);
			uint32 TilesY = Desc.GetTileCountY(Mip);
			TotalSize += TilesX * TilesY * IndirectionEntryBytes;
		}

		// Allocate CPU-side staging data
		mStagingData.resize(TotalSize, 0xFF); // Initialize to invalid entries

		// Create the GPU indirection texture
		// R16G16B16A16_UINT, with mip chain
		// Mip 0 size = number of tiles at VT mip 0
		uint32 Mip0Width = Desc.GetTileCountX(0);
		uint32 Mip0Height = Desc.GetTileCountY(0);

		RHITextureCreateDesc TexDesc;
		TexDesc.Width = Mip0Width;
		TexDesc.Height = Mip0Height;
		TexDesc.Depth = 1;
		TexDesc.ArraySize = 1;
		TexDesc.NumMips = NumMips;
		TexDesc.NumSamples = 1;
		TexDesc.Format = PF_R16G16B16A16_UINT;
		TexDesc.Flags = TextureCreateFlags::ShaderResource | TextureCreateFlags::CPUWritable;
		TexDesc.Dimension = ETextureDimension::Texture2D;

		mGPUTexture = mCmdCtx->RHICreateTexture(TexDesc);
		if (!mGPUTexture)
		{
			LogSystem::instance()->Log(LogLevel::Error,
				"VTIndirectionTexture: Failed to create GPU texture for space %u (%ux%u, %u mips)",
				mSpace->GetSpaceID(), Mip0Width, Mip0Height, NumMips);
			return;
		}

		// Create staging buffer for CPU->GPU upload
		uint32 StagingSize = ComputeTotalStagingSize();
		mStagingBuffer = mCmdCtx->RHICreateBuffer(
			BufferUsageFlags::TransferSrc | BufferUsageFlags::CPUVisible,
			ERHIAccess::CopySrc,
			nullptr,
			StagingSize);

		mNeedsFullRefresh = true;
		mInitialized = true;

		LogSystem::instance()->Log(LogLevel::Info,
			"VTIndirectionTexture: Initialized for space %u (%ux%u, %u mips, staging=%u bytes)",
			mSpace->GetSpaceID(), Mip0Width, Mip0Height, NumMips, StagingSize);
	}

	void VTIndirectionTexture::Shutdown()
	{
		if (!mInitialized)
			return;

		// GPU resources are managed by the RHI and will be cleaned up
		// through reference counting or explicit release
		mGPUTexture = nullptr;
		mStagingBuffer = nullptr;
		mStagingData.clear();
		mDirtyRegions.clear();
		mMipStagingOffsets.clear();

		mInitialized = false;
		mSpace = nullptr;
		mCmdCtx = nullptr;

		LogSystem::instance()->Log(LogLevel::Info,
			"VTIndirectionTexture: Shutdown complete");
	}

	//=============================================================================
	// Per-frame Update
	//=============================================================================

	uint32 VTIndirectionTexture::Update(RHICommandList* InCmdList)
	{
		if (!mInitialized || !InCmdList)
			return 0;

		// Check if full refresh is needed
		if (mNeedsFullRefresh)
		{
			ForceFullRefresh(InCmdList);
			return mLastUpdateCount;
		}

		// Step 1: Collect dirty regions from VirtualTextureSpace
		CollectDirtyRegions();

		if (mDirtyRegions.empty())
		{
			mLastUpdateCount = 0;
			return 0;
		}

		// Check if too many entries are dirty - switch to full refresh
		uint32 TotalEntries = 0;
		const VTSpaceDesc& Desc = mSpace->GetDesc();
		uint8 NumMips = mSpace->GetNumMipLevels();
		for (uint8 Mip = 0; Mip < NumMips; ++Mip)
		{
			TotalEntries += Desc.GetTileCountX(Mip) * Desc.GetTileCountY(Mip);
		}

		if ((float)mDirtyRegions.size() / (float)TotalEntries > FullRefreshThreshold)
		{
			ForceFullRefresh(InCmdList);
			return mLastUpdateCount;
		}

		// Step 2: Write dirty entries to staging buffer
		uint32 EntriesWritten = WriteStagingBuffer();

		// Step 3: Upload to GPU
		UploadToGPU(InCmdList);

		// Clear dirty flags in the space
		mSpace->ClearDirtyFlags();

		mLastUpdateCount = EntriesWritten;
		return EntriesWritten;
	}

	void VTIndirectionTexture::ForceFullRefresh(RHICommandList* InCmdList)
	{
		if (!mInitialized || !InCmdList)
			return;

		// Write all entries to staging buffer
		uint32 EntriesWritten = WriteFullStagingBuffer();

		// Upload entire texture
		UploadFullToGPU(InCmdList);

		// Clear dirty flags
		mSpace->ClearDirtyFlags();

		mNeedsFullRefresh = false;
		mLastUpdateCount = EntriesWritten;

		LogSystem::instance()->Log(LogLevel::Info,
			"VTIndirectionTexture: Full refresh complete (%u entries) for space %u",
			EntriesWritten, mSpace->GetSpaceID());
	}

	//=============================================================================
	// Internal: Collect Dirty Regions
	//=============================================================================

	void VTIndirectionTexture::CollectDirtyRegions()
	{
		mDirtyRegions.clear();

		// Get dirty entries from the VirtualTextureSpace
		std::vector<std::pair<VTTileCoord, IndirectionEntry>> DirtyEntries;
		mSpace->GetDirtyEntries(DirtyEntries);

		if (DirtyEntries.empty())
			return;

		// For each dirty entry, create a DirtyRegion
		// Currently creates 1x1 regions per entry; future optimization could merge adjacent regions
		for (const auto& Pair : DirtyEntries)
		{
			const VTTileCoord& Coord = Pair.first;
			const IndirectionEntry& Entry = Pair.second;

			DirtyRegion Region;
			Region.MipLevel = Coord.MipLevel;
			Region.X = Coord.X;
			Region.Y = Coord.Y;
			Region.Width = 1;
			Region.Height = 1;

			// Compute staging buffer offset for this entry
			uint32 MipOffset = GetMipStagingOffset(Coord.MipLevel);
			uint32 MipWidth = mSpace->GetDesc().GetTileCountX(Coord.MipLevel);
			Region.StagingOffset = MipOffset + (Coord.Y * MipWidth + Coord.X) * IndirectionEntryBytes;

			mDirtyRegions.push_back(Region);

			// Write the entry into our CPU staging data
			std::memcpy(&mStagingData[Region.StagingOffset], &Entry, IndirectionEntryBytes);
		}
	}

	//=============================================================================
	// Internal: Write Staging Buffer
	//=============================================================================

	uint32 VTIndirectionTexture::WriteStagingBuffer()
	{
		if (mDirtyRegions.empty() || !mStagingBuffer)
			return 0;

		// Map the staging buffer and write dirty regions
		// The staging data has already been written in CollectDirtyRegions()
		// Now we need to upload the dirty portions to the RHI staging buffer

		// For efficiency, we upload the entire staging data
		// A more optimal implementation would only upload dirty byte ranges
		if (mCmdCtx)
		{
			void* MappedData = mCmdCtx->RHIMapBuffer(mStagingBuffer, 0, mStagingData.size());
			if (MappedData)
			{
				// Only copy the dirty regions
				for (const auto& Region : mDirtyRegions)
				{
					uint32 ByteSize = Region.Width * Region.Height * IndirectionEntryBytes;
					std::memcpy(
						static_cast<uint8*>(MappedData) + Region.StagingOffset,
						&mStagingData[Region.StagingOffset],
						ByteSize);
				}
				mCmdCtx->RHIUnmapBuffer(mStagingBuffer);
			}
		}

		return static_cast<uint32>(mDirtyRegions.size());
	}

	uint32 VTIndirectionTexture::WriteFullStagingBuffer()
	{
		if (!mStagingBuffer)
			return 0;

		const VTSpaceDesc& Desc = mSpace->GetDesc();
		uint8 NumMips = mSpace->GetNumMipLevels();
		uint32 TotalEntries = 0;

		// Build the full indirection texture data for all mip levels
		for (uint8 Mip = 0; Mip < NumMips; ++Mip)
		{
			uint32 TilesX = Desc.GetTileCountX(Mip);
			uint32 TilesY = Desc.GetTileCountY(Mip);
			uint32 MipOffset = GetMipStagingOffset(Mip);

			for (uint32 Y = 0; Y < TilesY; ++Y)
			{
				for (uint32 X = 0; X < TilesX; ++X)
				{
					VTTileCoord Coord;
					Coord.X = static_cast<uint16>(X);
					Coord.Y = static_cast<uint16>(Y);
					Coord.MipLevel = Mip;
					Coord.SpaceID = mSpace->GetSpaceID();

					IndirectionEntry Entry;

					// Check if this tile is resident
					const PageTableEntry* PageEntry = mSpace->GetPageTableEntry(Coord);
					if (PageEntry && PageEntry->IsResident())
					{
						Entry = IndirectionEntry::Create(PageEntry->PhysicalLocation, 0, true);
					}
					else
					{
						// Try fallback to lower-resolution mip
						Entry = BuildFallbackEntry(Coord);
					}

					uint32 Offset = MipOffset + (Y * TilesX + X) * IndirectionEntryBytes;
					std::memcpy(&mStagingData[Offset], &Entry, IndirectionEntryBytes);
					++TotalEntries;
				}
			}
		}

		// Upload entire staging data to the RHI staging buffer
		if (mCmdCtx)
		{
			void* MappedData = mCmdCtx->RHIMapBuffer(mStagingBuffer, 0, mStagingData.size());
			if (MappedData)
			{
				std::memcpy(MappedData, mStagingData.data(), mStagingData.size());
				mCmdCtx->RHIUnmapBuffer(mStagingBuffer);
			}
		}

		return TotalEntries;
	}

	//=============================================================================
	// Internal: Upload to GPU
	//=============================================================================

	void VTIndirectionTexture::UploadToGPU(RHICommandList* InCmdList)
	{
		if (!InCmdList || !mGPUTexture || !mStagingBuffer || mDirtyRegions.empty())
			return;

		// Transition GPU texture to copy-dest layout
		RHITransitionInfo Transition;
		Transition.Resource = mGPUTexture;
		Transition.StateBefore = ERHIAccess::SRVGraphics;
		Transition.StateAfter = ERHIAccess::CopyDest;
		InCmdList->Transition(Transition);

		// Issue buffer-to-image copy for each dirty region
		for (const auto& Region : mDirtyRegions)
		{
			uint32 MipWidth = mSpace->GetDesc().GetTileCountX(Region.MipLevel);

			RHICopyBufferToTextureInfo CopyInfo;
			CopyInfo.BufferOffset = Region.StagingOffset;
			CopyInfo.BufferRowLength = MipWidth * IndirectionEntryBytes;
			CopyInfo.BufferImageHeight = 0; // Tightly packed
			CopyInfo.TextureOffset.x = Region.X;
			CopyInfo.TextureOffset.y = Region.Y;
			CopyInfo.TextureOffset.z = 0;
			CopyInfo.TextureSize.x = Region.Width;
			CopyInfo.TextureSize.y = Region.Height;
			CopyInfo.TextureSize.z = 1;
			CopyInfo.MipLevel = Region.MipLevel;
			CopyInfo.ArrayLayer = 0;

			InCmdList->CopyBufferToTexture(mStagingBuffer, mGPUTexture, CopyInfo);
		}

		// Transition GPU texture back to shader read
		Transition.StateBefore = ERHIAccess::CopyDest;
		Transition.StateAfter = ERHIAccess::SRVGraphics;
		InCmdList->Transition(Transition);

		mDirtyRegions.clear();
	}

	void VTIndirectionTexture::UploadFullToGPU(RHICommandList* InCmdList)
	{
		if (!InCmdList || !mGPUTexture || !mStagingBuffer)
			return;

		const VTSpaceDesc& Desc = mSpace->GetDesc();
		uint8 NumMips = mSpace->GetNumMipLevels();

		// Transition GPU texture to copy-dest layout
		RHITransitionInfo Transition;
		Transition.Resource = mGPUTexture;
		Transition.StateBefore = ERHIAccess::SRVGraphics;
		Transition.StateAfter = ERHIAccess::CopyDest;
		InCmdList->Transition(Transition);

		// Issue one copy per mip level
		for (uint8 Mip = 0; Mip < NumMips; ++Mip)
		{
			uint32 TilesX = Desc.GetTileCountX(Mip);
			uint32 TilesY = Desc.GetTileCountY(Mip);
			uint32 MipOffset = GetMipStagingOffset(Mip);

			RHICopyBufferToTextureInfo CopyInfo;
			CopyInfo.BufferOffset = MipOffset;
			CopyInfo.BufferRowLength = TilesX * IndirectionEntryBytes;
			CopyInfo.BufferImageHeight = 0;
			CopyInfo.TextureOffset.x = 0;
			CopyInfo.TextureOffset.y = 0;
			CopyInfo.TextureOffset.z = 0;
			CopyInfo.TextureSize.x = TilesX;
			CopyInfo.TextureSize.y = TilesY;
			CopyInfo.TextureSize.z = 1;
			CopyInfo.MipLevel = Mip;
			CopyInfo.ArrayLayer = 0;

			InCmdList->CopyBufferToTexture(mStagingBuffer, mGPUTexture, CopyInfo);
		}

		// Transition back to shader read
		Transition.StateBefore = ERHIAccess::CopyDest;
		Transition.StateAfter = ERHIAccess::SRVGraphics;
		InCmdList->Transition(Transition);
	}

	//=============================================================================
	// Internal: Helpers
	//=============================================================================

	uint32 VTIndirectionTexture::GetMipStagingOffset(uint8 MipLevel) const
	{
		if (MipLevel < mMipStagingOffsets.size())
			return mMipStagingOffsets[MipLevel];
		return 0;
	}

	uint32 VTIndirectionTexture::ComputeTotalStagingSize() const
	{
		if (!mSpace) return 0;

		const VTSpaceDesc& Desc = mSpace->GetDesc();
		uint8 NumMips = mSpace->GetNumMipLevels();
		uint32 TotalSize = 0;

		for (uint8 Mip = 0; Mip < NumMips; ++Mip)
		{
			uint32 TilesX = Desc.GetTileCountX(Mip);
			uint32 TilesY = Desc.GetTileCountY(Mip);
			TotalSize += TilesX * TilesY * IndirectionEntryBytes;
		}

		return TotalSize;
	}

	IndirectionEntry VTIndirectionTexture::BuildFallbackEntry(const VTTileCoord& Coord) const
	{
		if (!mSpace)
			return IndirectionEntry::Invalid();

		// Walk up the mip chain looking for a resident parent tile
		uint8 NumMips = mSpace->GetNumMipLevels();

		for (uint8 FallbackMip = Coord.MipLevel + 1; FallbackMip < NumMips; ++FallbackMip)
		{
			// Compute which tile at the fallback mip level covers this tile
			uint8 MipDelta = FallbackMip - Coord.MipLevel;
			uint16 ParentX = Coord.X >> MipDelta;
			uint16 ParentY = Coord.Y >> MipDelta;

			VTTileCoord ParentCoord;
			ParentCoord.X = ParentX;
			ParentCoord.Y = ParentY;
			ParentCoord.MipLevel = FallbackMip;
			ParentCoord.SpaceID = Coord.SpaceID;

			const PageTableEntry* ParentEntry = mSpace->GetPageTableEntry(ParentCoord);
			if (ParentEntry && ParentEntry->IsResident())
			{
				// Found a resident parent - create entry with mip bias
				return IndirectionEntry::Create(
					ParentEntry->PhysicalLocation,
					MipDelta,  // MipBias: how many levels we fell back
					false      // Not the requested mip, so mark as fallback
				);
			}
		}

		// No fallback found - return invalid
		return IndirectionEntry::Invalid();
	}

} // namespace Elaine
