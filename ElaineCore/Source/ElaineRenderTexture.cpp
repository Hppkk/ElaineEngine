#include "ElainePrecompiledHeader.h"
#include "ElaineRenderTexture.h"

namespace Elaine
{
	RenderTexture::RenderTexture(uint32 InWidth, uint32 InHeight)
	{

	}

	RenderTexture::~RenderTexture()
	{

	}

	RHITexture* RenderTexture::GetTargetImpl()
	{
		return mTextureRHI;
	}
}