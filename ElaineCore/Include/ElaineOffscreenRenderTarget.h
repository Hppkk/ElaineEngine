#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderTarget.h"

namespace Elaine
{
	//=============================================================================
	// OffscreenRenderTarget - 离屏渲染目标
	// 用于渲染到纹理 (后处理、阴影图等)
	//=============================================================================
	class ElaineCoreExport OffscreenRenderTarget : public RenderTarget
	{
	public:
		static constexpr uint32 MAX_RENDER_TARGETS = 8;

		OffscreenRenderTarget() = default;
		OffscreenRenderTarget(uint32 InWidth, uint32 InHeight);
		virtual ~OffscreenRenderTarget() = default;

		//=========================================================================
		// 设置渲染目标
		//=========================================================================
		void SetColorTarget(uint32 Index, RHITexture* InTexture);
		void SetDepthStencilTarget(RHITexture* InTexture);
		void SetSize(uint32 InWidth, uint32 InHeight);

		//=========================================================================
		// RenderTarget 接口实现
		//=========================================================================
		virtual RHITexture* GetTargetImpl() override { return GetColorTarget(0); }
		virtual void GetSize(uint32& OutWidth, uint32& OutHeight) override;
		virtual RHITexture* GetColorTarget(uint32 Index = 0) override;
		virtual RHITexture* GetDepthStencilTarget() override { return mDepthStencilTarget; }
		virtual uint32 GetNumColorTargets() const override { return mNumColorTargets; }

		//=========================================================================
		// 离屏渲染特定接口 (非交换链)
		//=========================================================================
		virtual bool IsSwapchainTarget() const override { return false; }

	private:
		RHITexture* mColorTargets[MAX_RENDER_TARGETS] = {};
		RHITexture* mDepthStencilTarget = nullptr;
		uint32 mWidth = 0;
		uint32 mHeight = 0;
		uint32 mNumColorTargets = 0;
	};
}
