#include "ElainePrecompiledHeader.h"
#include "ElaineSwapchainRenderTarget.h"
#include "render/common/ElaineRHIProtocol.h"

namespace Elaine
{
	SwapchainRenderTarget::SwapchainRenderTarget(RHISwapchain* InSwapchain)
		: mSwapchain(InSwapchain)
		, mCurrentImageIndex(0)
	{
	}

	RHITexture* SwapchainRenderTarget::GetTargetImpl()
	{
		if (mSwapchain)
		{
			return mSwapchain->AcquireNextTexture();
		}
		return nullptr;
	}

	void SwapchainRenderTarget::GetSize(uint32& OutWidth, uint32& OutHeight)
	{
		if (mSwapchain)
		{
			mSwapchain->GetSize(OutWidth, OutHeight);
		}
		else
		{
			OutWidth = 0;
			OutHeight = 0;
		}
	}

	RHITexture* SwapchainRenderTarget::GetColorTarget(uint32 Index)
	{
		// 交换链只有一个颜色目标
		if (Index == 0)
		{
			return GetTargetImpl();
		}
		return nullptr;
	}

	RHITexture* SwapchainRenderTarget::GetDepthStencilTarget()
	{
		// 交换链的深度缓冲由交换链管理
		// 需要从交换链获取深度缓冲
		if (mSwapchain)
		{
			return mSwapchain->GetDepthStencilTexture();
		}
		return nullptr;
	}
}
