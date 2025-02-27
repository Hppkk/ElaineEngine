#include "ElainePrecompiledHeader.h"
#include "render/ElaineRenderSystem.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	RenderSystem::RenderSystem()
	{

	}

	void RenderSystem::Initilize(const RHI_PARAM_DESC& InDesc)
	{
		mWindowHandle = InDesc.WindowHandle;
		InitEngineRHI(InDesc);
		mImmedCommandCtx = GetDynamicRHI()->CreateCommandContex();
	}

	RHIBuffer* RenderSystem::CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize)
	{
		return mImmedCommandCtx->RHICreateVertexBuffer(InSize, InUsage, InResourceState, InData);
	}

	RenderSystem::~RenderSystem()
	{
		GetDynamicRHI()->DestroyCommandContext(mImmedCommandCtx);
		mImmedCommandCtx = nullptr;
		DestroyEngineRHI();
	}

}