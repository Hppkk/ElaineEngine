#pragma once
#include "ElaineSingleton.h"
#include "render/common/ElaineRHIProtocol.h"
#include "ElaineRenderContext.h"

namespace Elaine
{
	class RenderCommandQueue;

	class ElaineCoreExport RenderSystem :public Singleton<RenderSystem>
	{
	public:
		RenderSystem();
		~RenderSystem();
		void Initialize(const RHI_PARAM_DESC& InDesc);
		RHIBuffer* CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize);
		RenderCommandQueue* GetRenderCommandQueue() const { return mRenderCommandQueue; }
		RenderContext& GetRenderContext() { return mRenderContext; }
		RHICommandContext* GetRHICommandContext() const { return mImmedCommandCtx; }
	private:
		RHICommandContext* mImmedCommandCtx = nullptr;
		RenderCommandQueue* mRenderCommandQueue = nullptr;
		RenderContext mRenderContext;
		void* mWindowHandle = nullptr;
	};
}