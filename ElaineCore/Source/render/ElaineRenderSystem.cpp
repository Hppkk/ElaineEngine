#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineRenderCommandQueue.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::Initialize(const RHI_PARAM_DESC& InDesc)
	{
		mWindowHandle = InDesc.WindowHandle;
		InitEngineRHI(InDesc);
		mImmedCommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		mRenderCommandQueue = new RenderCommandQueue();
	}

	RHIBuffer* RenderSystem::CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize)
	{
		return mImmedCommandCtx->RHICreateVertexBuffer(InSize, InUsage, InResourceState, InData);
	}

	RenderSystem::~RenderSystem()
	{
		//GetDynamicRHI()->DestroyCommandContext(mImmedCommandCtx);
		mImmedCommandCtx = nullptr;
		DestroyEngineRHI();
		SAFE_DELETE(mRenderCommandQueue);
	}

}