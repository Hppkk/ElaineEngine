#include "ElainePrecompiledHeader.h"
#include "ElaineSceneManager.h"
#include "ElaineThreadManager.h"
#include "ElaineRenderQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineUniformGPUManager.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineRenderPipeline.h"
#include "ElaineForwardRenderPipeline.h"
#include "ElaineRenderView.h"
//-----------------------------------
#include "RenderProxy/ElaineMeshRenderProxy.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
//-----------------------------------

namespace Elaine
{
	SceneManager::SceneManager(const String& name)
		: mName(name)
	{
		mQuadTree = new QuadTree();
		// mRenderQueueSet removed
		//mMainCamera = new Camera("Main Camera");
		mRHICommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		//mRenderThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RenderThread, &SceneManager::RenderThreadMain, this);
		// mCommonUniformBuffer removed
	}

	SceneManager::~SceneManager()
	{
		// QueueSet is deleted in FrameData destructor
		//ThreadManager::instance()->DestroyThread(mRenderThread);
	}

	void SceneManager::Initialize()
	{
		mbInit = true;
	}

	Camera* SceneManager::GetMainCamera()
	{
		return mMainCamera;
	}

	void SceneManager::findVisibleObject()
	{
		
	}
	
	RenderFrameData& SceneManager::GetRenderFrameData()
	{
		// 读取正在被 RenderThread 使用的帧（也就是 GameThread 已经写完的）
		// 如果 CurrentWrite 是 0，说明 GameThread 正在写 0（或准备写），RenderThread 读 1。
		// 如果 GameThread 刚写完 0，CurrentWrite 变为 1，RenderThread 读 0。
		uint32 ReadIndex = (mCurrentWriteFrame + 1) % 2;
		return mFrameData[ReadIndex];
	}

	void SceneManager::PrepareRenderData(Camera* InCamera)
	{
		//if (!InCamera)
		//	InCamera = mMainCamera; // Fallback to main camera

		uint32 WriteIndex = mCurrentWriteFrame;
		RenderFrameData& FrameData = mFrameData[WriteIndex];
		
		FrameData.Clear();
		
		std::vector<RenderProxy*> VisibleRenderProxys;
		mQuadTree->FindVisibleObjectsByCamera(InCamera, VisibleRenderProxys);

		RenderQueueSet* QueueSet = FrameData.mRenderQueueSet;
		for (auto VisibleProxy : VisibleRenderProxys)
		{
			VisibleProxy->UpdateRenderQueue(QueueSet);
		}
		
		UpdateCommonUniformBuffer(FrameData, InCamera);
		
		if (mCommonUniformBufferRHI == nullptr)
		{
			RHIUniformBufferDesc Desc;
			Desc.Dynamic = true;
			Desc.InitialData = nullptr;
			Desc.Size = sizeof(CommonUniformBufferCPU);
			Desc.Slot = RHIUniformSlot::PerFrame;
			mCommonUniformBufferRHI = mRHICommandCtx->RHICreateUniformBufferWithSlot(Desc);
		}

		// 提交
		mCurrentWriteFrame = (WriteIndex + 1) % 2;
		mFrameReady = true;
	}

	RenderProxy* SceneManager::CreateRenderProxy(EProxyType InType)
	{
		switch (InType)
		{
		case Elaine::EProxyType::StaticMesh:
		{
			RenderProxy* NewRenderProxy = new StaticMeshRenderProxy();
			mRenderProxys.insert(NewRenderProxy);
			mQuadTree->AddRenderProxy(NewRenderProxy);
			return NewRenderProxy;
		}
		case Elaine::EProxyType::Sky:
		{
			RenderProxy* NewRenderProxy = new SkyRenderProxy();
			mRenderProxys.insert(NewRenderProxy);
			// Sky is global; still insert to proxy set but do not add to quad tree
			mQuadTree->AddRenderProxy(NewRenderProxy);
			return NewRenderProxy;
		}
		case Elaine::EProxyType::Light:
			break;
		case Elaine::EProxyType::Particle:
			break;
		case Elaine::EProxyType::Unknown:
			LOG_FATAL("Can not create unknown type.")
			break;
		default:
			break;
		}

		return nullptr;
	}

	void SceneManager::DestroyRenderProxy(RenderProxy* InProxy)
	{
		if (InProxy == nullptr)
			return;

		mRenderProxys.erase(InProxy);
		mQuadTree->RemoveRenderProxy(InProxy);
		SAFE_DELETE(InProxy);
	}

	void SceneManager::RenderScene()
	{
		RenderSystem::instance()->GetRenderCommandQueue()->Execute();
		PrepareRenderData(mMainCamera);

		if (!mFrameReady) return;
		
		static RenderView* MainView = nullptr;
		if (!MainView)
		{
			MainView = new RenderView();
			MainView->mCamera = mMainCamera;
			MainView->mSceneManager = this;
		}
		
		static ForwardRenderPipeline* Pipeline = new ForwardRenderPipeline();
		static bool bPipelineInit = false;
		if (!bPipelineInit)
		{
			Pipeline->Initialize();
			bPipelineInit = true;
		}

		Pipeline->Render(MainView);
	}

	void SceneManager::RenderThreadMain()
	{
		while (!mbExit)
		{
			WaitForRenderThread_Gfx();
			RenderScene();
			TaskGraph::TaskGraph::instance()->ExecuteInThread(NamedThread::RenderThread);
			NotifyForRHIThread_Gfx();
		}
	}

	void SceneManager::UpdateCommonUniformBuffer(RenderFrameData& FrameData, Camera* InCamera)
	{
		if (!InCamera)
			InCamera = mMainCamera;

		float radius = 1.0f;
		float camX = Root::instance()->GetTimer()->getSeconds() * radius;
		InCamera->SetRotation(Vector3(0.0f, camX, 0.0f));

		CommonUniformBufferCPU& UB = FrameData.mCommonUniformBuffer;
		UB.U_ViewMatrix = InCamera->GetViewMatrix();
		UB.U_ProjectionMatrix = InCamera->GetProjMatrix();
		UB.U_ViewProjectionMatrix = InCamera->GetViewProjMatrix();
		UB.U_CameraDirection = Vector4(InCamera->GetForward(), 0.0f);
		UB.U_CameraPosition = Vector4(InCamera->GetPosition(), 1.0f);
	}

	//=============================================================================
	// Camera 管理
	//=============================================================================
	void SceneManager::RegisterCamera(Camera* InCamera)
	{
		if (InCamera)
		{
			mCameras.push_back(InCamera);
			if (!mActiveCamera)
				mActiveCamera = InCamera;
		}
	}

	void SceneManager::UnregisterCamera(Camera* InCamera)
	{
		auto It = std::find(mCameras.begin(), mCameras.end(), InCamera);
		if (It != mCameras.end())
		{
			mCameras.erase(It);
			if (mActiveCamera == InCamera)
				mActiveCamera = mCameras.empty() ? nullptr : mCameras[0];
		}
	}

	void SceneManager::RegisterShadowCamera(Camera* InCamera)
	{
		if (InCamera)
			mShadowCameras.push_back(InCamera);
	}
}