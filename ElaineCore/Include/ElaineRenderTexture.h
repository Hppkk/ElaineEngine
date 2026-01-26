#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderTarget.h"

namespace Elaine
{
	class RenderTexture : public RenderTarget
	{
	public:
		RenderTexture(uint32 InWidth, uint32 InHeight);
		~RenderTexture();
		RHITexture* GetTargetImpl() override;
	private:

	};
}