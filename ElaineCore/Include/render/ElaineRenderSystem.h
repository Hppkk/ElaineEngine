#pragma once
#include "ElaineSingleton.h"
#include "GLFW/glfw3.h"
#include "render/common/ElaineRHIProtocol.h"

namespace Elaine
{
	class ElaineCoreExport RenderSystem :public Singleton<RenderSystem>
	{
	public:
		RenderSystem();
		~RenderSystem();
		void Initilize(const RHI_PARAM_DESC& InDesc);
		RHIBuffer* CreateBuffer(BufferUsageFlags InUsage, ERHIAccess InResourceState, void* InData, size_t InSize);
	private:
		RHICommandContext* mImmedCommandCtx = nullptr;
		void* mWindowHandle = nullptr;
	};
}