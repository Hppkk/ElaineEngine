#pragma once
#include "ElaineQuadTree.h"
#include "render/common/ElaineRHICommandList.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "ElaineRenderFrameData.h"

namespace Elaine
{
	class ThreadWrap;
	class RenderQueueSet;
	class RHICommandContext;
	struct CommonUniformBufferCPU;
	class RenderProxy;
	class RenderPipeline;

	class ElaineCoreExport SceneManager
	{
	public:
		SceneManager(const String& name);
		~SceneManager();

		void Initialize();

		Camera* GetMainCamera();
		void findVisibleObject();
		RenderProxy* CreateRenderProxy(EProxyType InType);
		void DestroyRenderProxy(RenderProxy* InProxy);
		void RenderScene();
		void RenderThreadMain();
		void UpdateCommonUniformBuffer(RenderFrameData& FrameData, Camera* InCamera);
		RHIUniformBuffer* GetCommonUniformBufferRHI() const { return mCommonUniformBufferRHI; }
		
		void PrepareRenderData(Camera* InCamera);
		RenderFrameData& GetRenderFrameData();

		//=========================================================================
		// Camera 管理（渲染线程）
		//=========================================================================
		void RegisterCamera(Camera* InCamera);
		void UnregisterCamera(Camera* InCamera);
		void RegisterShadowCamera(Camera* InCamera);
		void SetActiveCamera(Camera* InCamera) { mActiveCamera = InCamera; }
		Camera* GetActiveCamera() const { return mActiveCamera; }
	private:
		String mName;
		Camera* mMainCamera = nullptr;
		Camera* mSecondCamera = nullptr;
		QuadTree* mQuadTree = nullptr;
		//ThreadWrap* mRenderThread = nullptr;
		std::set<RenderProxy*> mRenderProxys;
		RHICommandContext* mRHICommandCtx = nullptr;
		std::array<RenderFrameData, 2> mFrameData;
		std::atomic<uint32_t> mCurrentWriteFrame{ 0 };
		std::atomic<bool> mFrameReady{ false };
		RHIUniformBuffer* mCommonUniformBufferRHI = nullptr;
		//=========================================================================
		// Camera 管理（渲染线程）
		//=========================================================================
		std::vector<Camera*> mCameras;
		std::vector<Camera*> mShadowCameras;
		Camera* mActiveCamera = nullptr;

		bool mbExit = false;
		bool mbInit = false;
	};
}