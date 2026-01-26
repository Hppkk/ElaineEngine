#pragma once
#include "ElaineSceneManager.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	class ElaineCoreExport RenderContext
	{
	public:
		//SceneManager* mSceneManager = nullptr;
		RHICommandContext* mCommandCtxRHI = nullptr;
	};
}