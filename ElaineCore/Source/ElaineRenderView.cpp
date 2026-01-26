#include "ElainePrecompiledHeader.h"
#include "ElaineRenderView.h"
#include "render/common/ElaineRHITypes.h"
#include "ElaineRenderTarget.h"

namespace Elaine
{
	RenderView::RenderView()
	{
	}

	RenderView::~RenderView()
	{
		// 注意：不负责删除 Camera/Viewport/SceneManager
		// 这些对象的生命周期由外部管理
	}

	void RenderView::GetViewportSize(uint32& OutWidth, uint32& OutHeight) const
	{
		if (mRenderTarget)
		{
			mRenderTarget->GetSize(OutWidth, OutHeight);
			OutWidth = static_cast<uint32>((mRegion.MaxX - mRegion.MinX) * OutWidth);
			OutHeight = static_cast<uint32>((mRegion.MaxY - mRegion.MinY) * OutHeight);
		}
		else
		{
			assert(false);
			OutWidth = 0;
			OutHeight = 0;
		}
	}
}
