#include "ElainePrecompiledHeader.h"
#include "ElaineOffscreenRenderTarget.h"

namespace Elaine
{
	OffscreenRenderTarget::OffscreenRenderTarget(uint32 InWidth, uint32 InHeight)
		: mWidth(InWidth)
		, mHeight(InHeight)
		, mNumColorTargets(0)
		, mDepthStencilTarget(nullptr)
	{
		for (uint32 i = 0; i < MAX_RENDER_TARGETS; ++i)
		{
			mColorTargets[i] = nullptr;
		}
	}

	void OffscreenRenderTarget::SetColorTarget(uint32 Index, RHITexture* InTexture)
	{
		if (Index < MAX_RENDER_TARGETS)
		{
			mColorTargets[Index] = InTexture;
			// 更新颜色目标数量
			if (InTexture && Index >= mNumColorTargets)
			{
				mNumColorTargets = Index + 1;
			}
		}
	}

	void OffscreenRenderTarget::SetDepthStencilTarget(RHITexture* InTexture)
	{
		mDepthStencilTarget = InTexture;
	}

	void OffscreenRenderTarget::SetSize(uint32 InWidth, uint32 InHeight)
	{
		mWidth = InWidth;
		mHeight = InHeight;
	}

	void OffscreenRenderTarget::GetSize(uint32& OutWidth, uint32& OutHeight)
	{
		OutWidth = mWidth;
		OutHeight = mHeight;
	}

	RHITexture* OffscreenRenderTarget::GetColorTarget(uint32 Index)
	{
		if (Index < MAX_RENDER_TARGETS)
		{
			return mColorTargets[Index];
		}
		return nullptr;
	}
}
