#pragma once
#include "ElaineCorePrerequirements.h"
#include "common/ElaineRHIProtocol.h"

namespace Elaine
{
	class RHISwapchain;

	//=============================================================================
	// RenderTarget - 渲染目标基类
	// 支持交换链渲染和离屏渲染
	//=============================================================================
	class ElaineCoreExport RenderTarget
	{
	public:
		RenderTarget() = default;
		virtual ~RenderTarget() = default;

		//=========================================================================
		// 基础接口
		//=========================================================================
		RHITexture* GetTarget()
		{
			return GetTargetImpl();
		}
		virtual RHITexture* GetTargetImpl() = 0;
		virtual void GetSize(uint32& OutWidth, uint32& OutHeight) = 0;

		//=========================================================================
		// 颜色和深度附件接口
		//=========================================================================
		virtual RHITexture* GetColorTarget(uint32 Index = 0) { return GetTargetImpl(); }
		virtual RHITexture* GetDepthStencilTarget() { return nullptr; }
		virtual uint32 GetNumColorTargets() const { return 1; }

		//=========================================================================
		// 交换链支持接口
		//=========================================================================
		virtual bool IsSwapchainTarget() const { return false; }
		virtual RHISwapchain* GetSwapchain() const { return nullptr; }
		virtual void SetCurrentImageIndex(uint32 Index) {}
		virtual uint32 GetCurrentImageIndex() const { return 0; }

	protected:
		RHITexture* mTextureRHI = nullptr;
	};
}