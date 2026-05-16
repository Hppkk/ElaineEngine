#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "ElaineLogSystem.h"
#include <algorithm>
#include <cassert>

namespace Elaine
{
	VirtualTextureSpace::VirtualTextureSpace(uint8 InSpaceID, const VTSpaceDesc& InDesc)
		: mSpaceID(InSpaceID)
		, mDesc(InDesc)
		, mNumMipLevels(InDesc.ComputeNumMipLevels())
	{
		// Compute offsets for each mip level in the flattened page table
		mMipLevelOffsets.resize(mNumMipLevels);
		uint32 TotalPages = 0;
		for (uint8 Mip = 0; Mip < mNumMipLevels; ++Mip)
		{
			mMipLevelOffsets[Mip] = TotalPages;
			TotalPages += mDesc.GetTileCount(Mip);
		}

		// Allocate page table
		mPageTable.resize(TotalPages);
	}

	VirtualTextureSpace::~VirtualTextureSpace()
	{
		Shutdown();
	}

	void VirtualTextureSpace::Initialize()
	{
		// The indirection texture will be created later when we have RHI context
		// For now just validate the setup
		LogSystem::instance()->Log(LogLevel::Info,
			"VirtualTextureSpace [%s] initialized: SpaceID=%u, Size=%ux%u, Mips=%u, Layers=%u, TotalPages=%u",
			mDesc.Name.c_str(), mSpaceID,
			mDesc.VirtualSizeX, mDesc.VirtualSizeY,
			mNumMipLevels, mDesc.NumLayers,
			(uint32)mPageTable.size());
	}

	void VirtualTextureSpace::Shutdown()
	{
		mPageTable.clear();
		mMipLevelOffsets.clear();
		mDirtyPages.clear();
		mResidentTileCount = 0;
		mIndirectionTexture = nullptr;
	}

	//-----------------------------------------------
	// Page Table Operations
	//-----------------------------------------------

	const PageTableEntry* VirtualTextureSpace::GetPageTableEntry(const VTTileCoord& Coord) const
	{
		uint32 Index = ComputePageIndex(Coord);
		if (Index >= mPageTable.size())
			return nullptr;
		return &mPageTable[Index];
	}

	const PageTableEntry* VirtualTextureSpace::GetPageTableEntryWithFallback(
		const VTTileCoord& Coord, uint8& OutFallbackMipLevel) const
	{
		// Start from the requested mip level and fall back to coarser mips
		VTTileCoord FallbackCoord = Coord;
		
		for (uint8 Mip = Coord.MipLevel; Mip < mNumMipLevels; ++Mip)
		{
			FallbackCoord.MipLevel = Mip;
			// Scale coordinates down for coarser mip levels
			if (Mip > Coord.MipLevel)
			{
				uint32 MipDelta = Mip - Coord.MipLevel;
				FallbackCoord.X = Coord.X >> MipDelta;
				FallbackCoord.Y = Coord.Y >> MipDelta;
			}

			uint32 Index = ComputePageIndex(FallbackCoord);
			if (Index < mPageTable.size() && mPageTable[Index].IsResident())
			{
				OutFallbackMipLevel = Mip;
				return &mPageTable[Index];
			}
		}

		OutFallbackMipLevel = mNumMipLevels;
		return nullptr;
	}

	void VirtualTextureSpace::SetPageTableEntry(const VTTileCoord& Coord, const PageTableEntry& Entry)
	{
		uint32 Index = ComputePageIndex(Coord);
		if (Index >= mPageTable.size())
			return;

		// Track resident count changes
		bool WasResident = mPageTable[Index].IsResident();
		bool NowResident = Entry.IsResident();

		if (!WasResident && NowResident)
			++mResidentTileCount;
		else if (WasResident && !NowResident)
			--mResidentTileCount;

		mPageTable[Index] = Entry;

		// Mark as dirty for indirection texture update
		mDirtyPages.insert(Index);
	}

	void VirtualTextureSpace::TouchTile(const VTTileCoord& Coord, uint32 FrameNumber)
	{
		uint32 Index = ComputePageIndex(Coord);
		if (Index < mPageTable.size())
		{
			mPageTable[Index].FrameLastUsed = FrameNumber;
		}
	}

	void VirtualTextureSpace::GetDirtyEntries(
		std::vector<std::pair<VTTileCoord, IndirectionEntry>>& OutDirtyEntries) const
	{
		OutDirtyEntries.reserve(mDirtyPages.size());

		for (uint32 PageIndex : mDirtyPages)
		{
			const PageTableEntry& Entry = mPageTable[PageIndex];

			// Reverse-compute the tile coordinate from the page index
			// Find which mip level this index belongs to
			uint8 Mip = 0;
			for (uint8 m = 0; m < mNumMipLevels; ++m)
			{
				uint32 MipStart = mMipLevelOffsets[m];
				uint32 MipEnd = (m + 1 < mNumMipLevels) ? mMipLevelOffsets[m + 1] : (uint32)mPageTable.size();
				if (PageIndex >= MipStart && PageIndex < MipEnd)
				{
					Mip = m;
					break;
				}
			}

			uint32 LocalIndex = PageIndex - mMipLevelOffsets[Mip];
			uint32 TilesX = mDesc.GetTileCountX(Mip);

			VTTileCoord Coord;
			Coord.SpaceID = mSpaceID;
			Coord.MipLevel = Mip;
			Coord.X = uint16(LocalIndex % TilesX);
			Coord.Y = uint16(LocalIndex / TilesX);

			IndirectionEntry IndEntry;
			if (Entry.IsResident())
			{
				IndEntry = IndirectionEntry::Create(Entry.PhysicalLocation, 0, true);
			}
			else
			{
				// Find fallback entry for non-resident pages
				uint8 FallbackMip = 0;
				const PageTableEntry* Fallback = GetPageTableEntryWithFallback(Coord, FallbackMip);
				if (Fallback && Fallback->IsResident())
				{
					uint8 MipBias = FallbackMip - Mip;
					IndEntry = IndirectionEntry::Create(Fallback->PhysicalLocation, MipBias, false);
				}
				else
				{
					IndEntry = IndirectionEntry::Invalid();
				}
			}

			OutDirtyEntries.push_back({ Coord, IndEntry });
		}
	}

	void VirtualTextureSpace::ClearDirtyFlags()
	{
		mDirtyPages.clear();
	}

	void VirtualTextureSpace::GetLRUSortedTiles(std::vector<VTTileCoord>& OutTiles) const
	{
		// Collect all resident tiles
		for (uint8 Mip = 0; Mip < mNumMipLevels; ++Mip)
		{
			uint32 TilesX = mDesc.GetTileCountX(Mip);
			uint32 TilesY = mDesc.GetTileCountY(Mip);
			uint32 Offset = mMipLevelOffsets[Mip];

			for (uint32 y = 0; y < TilesY; ++y)
			{
				for (uint32 x = 0; x < TilesX; ++x)
				{
					uint32 Index = Offset + y * TilesX + x;
					if (mPageTable[Index].IsResident())
					{
						VTTileCoord Coord;
						Coord.SpaceID = mSpaceID;
						Coord.MipLevel = Mip;
						Coord.X = uint16(x);
						Coord.Y = uint16(y);
						OutTiles.push_back(Coord);
					}
				}
			}
		}

		// Sort by FrameLastUsed (least recently used first)
		std::sort(OutTiles.begin(), OutTiles.end(),
			[this](const VTTileCoord& A, const VTTileCoord& B)
			{
				uint32 IndexA = ComputePageIndex(A);
				uint32 IndexB = ComputePageIndex(B);
				return mPageTable[IndexA].FrameLastUsed < mPageTable[IndexB].FrameLastUsed;
			});
	}

	void VirtualTextureSpace::RemovePageTableEntry(const VTTileCoord& Coord)
	{
		uint32 Index = ComputePageIndex(Coord);
		if (Index >= mPageTable.size())
			return;

		if (mPageTable[Index].IsResident())
			--mResidentTileCount;

		mPageTable[Index] = PageTableEntry();  // Reset to default (NotLoaded)
		mDirtyPages.insert(Index);
	}

	//-----------------------------------------------
	// Indirection Texture
	//-----------------------------------------------

	void VirtualTextureSpace::BuildIndirectionTextureData(
		std::vector<IndirectionEntry>& OutData, uint8 MipLevel) const
	{
		if (MipLevel >= mNumMipLevels)
			return;

		uint32 TilesX = mDesc.GetTileCountX(MipLevel);
		uint32 TilesY = mDesc.GetTileCountY(MipLevel);
		uint32 Offset = mMipLevelOffsets[MipLevel];

		OutData.resize(TilesX * TilesY);

		for (uint32 y = 0; y < TilesY; ++y)
		{
			for (uint32 x = 0; x < TilesX; ++x)
			{
				uint32 Index = Offset + y * TilesX + x;
				const PageTableEntry& Entry = mPageTable[Index];

				if (Entry.IsResident())
				{
					OutData[y * TilesX + x] = IndirectionEntry::Create(
						Entry.PhysicalLocation, 0, true);
				}
				else
				{
					// Try fallback to coarser mip
					VTTileCoord Coord;
					Coord.SpaceID = mSpaceID;
					Coord.MipLevel = MipLevel;
					Coord.X = uint16(x);
					Coord.Y = uint16(y);

					uint8 FallbackMip = 0;
					const PageTableEntry* Fallback = GetPageTableEntryWithFallback(Coord, FallbackMip);
					if (Fallback && Fallback->IsResident())
					{
						uint8 MipBias = FallbackMip - MipLevel;
						OutData[y * TilesX + x] = IndirectionEntry::Create(
							Fallback->PhysicalLocation, MipBias, false);
					}
					else
					{
						OutData[y * TilesX + x] = IndirectionEntry::Invalid();
					}
				}
			}
		}
	}

	//-----------------------------------------------
	// World-space mapping (for RVT)
	//-----------------------------------------------

	void VirtualTextureSpace::WorldToVirtualUV(float WorldX, float WorldY, float& OutU, float& OutV) const
	{
		float RangeX = mDesc.WorldBoundsMaxX - mDesc.WorldBoundsMinX;
		float RangeY = mDesc.WorldBoundsMaxY - mDesc.WorldBoundsMinY;

		OutU = (RangeX > 0.0f) ? (WorldX - mDesc.WorldBoundsMinX) / RangeX : 0.0f;
		OutV = (RangeY > 0.0f) ? (WorldY - mDesc.WorldBoundsMinY) / RangeY : 0.0f;

		// Clamp to [0, 1]
		OutU = (OutU < 0.0f) ? 0.0f : ((OutU > 1.0f) ? 1.0f : OutU);
		OutV = (OutV < 0.0f) ? 0.0f : ((OutV > 1.0f) ? 1.0f : OutV);
	}

	VTTileCoord VirtualTextureSpace::VirtualUVToTileCoord(float U, float V, uint8 MipLevel) const
	{
		uint32 TilesX = mDesc.GetTileCountX(MipLevel);
		uint32 TilesY = mDesc.GetTileCountY(MipLevel);

		VTTileCoord Coord;
		Coord.SpaceID = mSpaceID;
		Coord.MipLevel = MipLevel;
		Coord.X = uint16((uint32)(U * TilesX) % TilesX);
		Coord.Y = uint16((uint32)(V * TilesY) % TilesY);
		return Coord;
	}

	//-----------------------------------------------
	// Statistics
	//-----------------------------------------------

	uint32 VirtualTextureSpace::GetTotalTileCount() const
	{
		return (uint32)mPageTable.size();
	}

	//-----------------------------------------------
	// Private Helpers
	//-----------------------------------------------

	uint32 VirtualTextureSpace::ComputePageIndex(const VTTileCoord& Coord) const
	{
		if (Coord.MipLevel >= mNumMipLevels)
			return (uint32)mPageTable.size(); // Out of bounds sentinel

		uint32 TilesX = mDesc.GetTileCountX(Coord.MipLevel);
		uint32 TilesY = mDesc.GetTileCountY(Coord.MipLevel);

		if (Coord.X >= TilesX || Coord.Y >= TilesY)
			return (uint32)mPageTable.size(); // Out of bounds sentinel

		return mMipLevelOffsets[Coord.MipLevel] + Coord.Y * TilesX + Coord.X;
	}

	uint32 VirtualTextureSpace::GetMipLevelOffset(uint8 MipLevel) const
	{
		if (MipLevel >= mNumMipLevels)
			return (uint32)mPageTable.size();
		return mMipLevelOffsets[MipLevel];
	}

} // namespace Elaine
