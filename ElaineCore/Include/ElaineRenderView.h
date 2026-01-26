#pragma once
#include "ElaineCorePrerequirements.h"
#include "math/ElaineVector4.h"

namespace Elaine
{
	class Camera;
	class RHIViewport;
	class SceneManager;
	class RenderTarget;

	//=============================================================================
	// 视口区域定义（支持分屏渲染）
	//=============================================================================
	struct ViewportRegion
	{
		float MinX = 0.0f;
		float MinY = 0.0f;
		float MaxX = 1.0f;
		float MaxY = 1.0f;

		// 归一化坐标 [0,1] -> 像素坐标
		void ToPixelCoords(uint32 ViewportWidth, uint32 ViewportHeight,
			uint32& OutX, uint32& OutY, uint32& OutWidth, uint32& OutHeight) const
		{
			OutX = static_cast<uint32>(MinX * ViewportWidth);
			OutY = static_cast<uint32>(MinY * ViewportHeight);
			OutWidth = static_cast<uint32>((MaxX - MinX) * ViewportWidth);
			OutHeight = static_cast<uint32>((MaxY - MinY) * ViewportHeight);
		}
	};

	//=============================================================================
	// RenderView - 渲染视图
	// 组合 Camera + Viewport + SceneManager，定义"如何渲染一个场景"
	//=============================================================================
	class ElaineCoreExport RenderView
	{
	public:
		RenderView();
		~RenderView();

		//=========================================================================
		// 核心组件
		//=========================================================================
		Camera* mCamera = nullptr;
		//RHIViewport* mViewport = nullptr;
		SceneManager* mSceneManager = nullptr;
		RenderTarget* mRenderTarget = nullptr;

		//=========================================================================
		// 渲染设置
		//=========================================================================
		bool mbEnablePostProcess = true;
		bool mbEnableShadows = true;
		bool mbEnableTransparency = true;
		bool mbActive = true;
		
		LinearColor mClearColor = LinearColor(0.0f, 0.2f, 0.4f, 1.0f);
		
		//=========================================================================
		// 视口区域（支持分屏）
		//=========================================================================
		ViewportRegion mRegion;

		//=========================================================================
		// 辅助方法
		//=========================================================================
		bool IsValid() const 
		{ 
			return mCamera != nullptr && mSceneManager != nullptr; 
		}

		bool IsActive() const { return mbActive; }

		// 获取视口像素尺寸
		void GetViewportSize(uint32& OutWidth, uint32& OutHeight) const;
	};
}
