#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderTarget.h"

namespace Elaine
{
	class RHISwapchain;
	class RHITexture;

	//=============================================================================
	// SwapchainRenderTarget - 交换链渲染目标
	// 用于渲染到窗口交换链
	//=============================================================================
	class ElaineCoreExport SwapchainRenderTarget : public RenderTarget
	{
	public:
		SwapchainRenderTarget() = default;
		explicit SwapchainRenderTarget(RHISwapchain* InSwapchain);
		virtual ~SwapchainRenderTarget() = default;

		//=========================================================================
		// 设置交换链
		//=========================================================================
		void SetSwapchain(RHISwapchain* InSwapchain) { mSwapchain = InSwapchain; }

		//=========================================================================
		// RenderTarget 接口实现
		//=========================================================================
		virtual RHITexture* GetTargetImpl() override;
		virtual void GetSize(uint32& OutWidth, uint32& OutHeight) override;
		virtual RHITexture* GetColorTarget(uint32 Index = 0) override;
		virtual RHITexture* GetDepthStencilTarget() override;
		
		//=========================================================================
		// 交换链特定接口
		//=========================================================================
		virtual bool IsSwapchainTarget() const override { return true; }
		virtual RHISwapchain* GetSwapchain() const override { return mSwapchain; }
		virtual void SetCurrentImageIndex(uint32 Index) override { mCurrentImageIndex = Index; }
		virtual uint32 GetCurrentImageIndex() const override { return mCurrentImageIndex; }

	protected:
		RHISwapchain* mSwapchain = nullptr;
		uint32 mCurrentImageIndex = 0;
	};
}
