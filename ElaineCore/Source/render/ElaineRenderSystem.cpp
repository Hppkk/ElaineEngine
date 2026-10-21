#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineRenderCommandQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::Initialize(const RHI_PARAM_DESC& InDesc)
	{
		mWindowHandle = InDesc.NativeHandle;
		InitEngineRHI(InDesc);
		mImmedCommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		mRenderCommandQueue = new RenderCommandQueue();
	}

	RHIBufferHandle RenderSystem::CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize)
	{
        CheckRHICreationThread();
        auto* Context = GetDynamicRHI()->GetDefaultCommandContext();
        auto* List = Context->GetRHICommandListMgr()->CreateCommandList();
        auto Buffer = Context->RHICreateBuffer(static_cast<uint32>(InSize), InUsage | BufferUsageFlags::Static, 0, ERHIAccess::Unknown, nullptr);
        if (InData && Buffer) Context->UploadBuffer(Buffer.get(), InData, static_cast<uint32>(InSize), InResourceState);
        List->SubmitUploads();
        return Buffer;
	}

	RenderSystem::~RenderSystem()
	{
		//GetDynamicRHI()->DestroyCommandContext(mImmedCommandCtx);
		mImmedCommandCtx = nullptr;
		DestroyEngineRHI();
		SAFE_DELETE(mRenderCommandQueue);
	}

}