#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"

namespace Elaine
{
	RHICommandContext::RHICommandContext()
	{
		mRHICmdListMgr = new RHICommandListManager(this);
	}

	RHICommandContext::~RHICommandContext()
	{
		SAFE_DELETE(mRHICmdListMgr);
	}
    void RHICommandContext::UploadBuffer(RHIBuffer* Target, const void* Data, uint32 Size, ERHIAccess Access)
    {
        CheckRHICreationThread();
        if (!Target || !Data || Size != Target->GetDesc().Size) throw std::invalid_argument("Invalid buffer upload size");
        auto Staging = RHICreateBuffer(Size, BufferUsageFlags::KeepCPUAccessible, 1, ERHIAccess::CopySrc, const_cast<void*>(Data));
        auto* List = mRHICmdListMgr->GetCurrentCommandList();
        if (!List) throw std::logic_error("Buffer upload requires a recording command list");
        List->CopyBufferRegion(Target, 0, Staging.get(), 0, Size);
        RHIResourceBarrierDesc Barrier{}; Barrier.Buffer = Target;
        Barrier.StateBefore = ERHIAccess::CopyDest; Barrier.StateAfter = Access;
        List->ResourceBarrier(Barrier);
    }
    void RHICommandContext::UploadTexture(RHITexture* Target, const void* Data, size_t Size)
    {
        CheckRHICreationThread();
        if (!Target || !Data) throw std::invalid_argument("Missing texture upload data");
        const auto& Desc = Target->GetDesc();
        if (Desc.mNumMips != 1 || Desc.mNumSamples != 1 || Desc.mFormat <= PF_Unknown || Desc.mFormat >= PF_MAX_)
            throw std::invalid_argument("Unsupported texture upload format or mip layout");
        const auto& Format = GPixelFormats[Desc.mFormat];
        if (Format.BlockSizeX != 1 || Format.BlockSizeY != 1 || !Format.BlockBytes || Desc.mDimension == TextureDimension::Texture3D || Desc.mDimension == TextureDimension::TextureCubeArray)
            throw std::invalid_argument("Unsupported texture upload dimension or block format");
        const uint64 Layers = Desc.mDimension == TextureDimension::TextureCube ? 6 : Desc.mDimension == TextureDimension::Texture2DArray ? Desc.mArraySize : 1;
        const uint64 Expected = uint64(Desc.mExtent.x) * uint64(Desc.mExtent.y) * Layers * Format.BlockBytes;
        if (Size != Expected || Size > UINT32_MAX) throw std::invalid_argument("Texture upload data size mismatch");
        auto Staging = RHICreateBuffer(static_cast<uint32>(Size), BufferUsageFlags::KeepCPUAccessible, 1, ERHIAccess::CopySrc, const_cast<void*>(Data));
        auto* List = mRHICmdListMgr->GetCurrentCommandList();
        if (!List) throw std::logic_error("Texture upload requires a recording command list");
        List->CopyBufferToTexture(Staging.get(), Target);
    }

}