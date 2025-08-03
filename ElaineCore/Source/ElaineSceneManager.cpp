#include "ElainePrecompiledHeader.h"
#include "ElaineSceneManager.h"
#include "ElaineThreadManager.h"
#include "ElaineSky.h"
#include "ElaineRenderQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
	SceneManager::SceneManager(const String& name)
		: mName(name)
	{
		mRootNode = createSceneNode();
		mRenderQueueSet = new RenderQueueSet();
		mSky = new SkyObject(mRootNode, this);
		mMainCamera = new Camera("Main Camera");
		mRHICommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		mRenderThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RenderThread, &SceneManager::RenderThreadMain, this);
		mCommonUniformBuffer = new CommonUniformBufferCPU();
	}

	SceneManager::~SceneManager()
	{
		SAFE_DELETE(mSky);
		SAFE_DELETE(mRenderQueueSet);
		for (SceneNode* node : mSceneNodeSets)
		{
			SAFE_DELETE(node);
		}
		ThreadManager::instance()->DestroyThread(mRenderThread);
	}

	void SceneManager::initilize()
	{
		mbInit = true;
		RenderScene();
	}

	Camera* SceneManager::getMainCamera()
	{
		return mMainCamera;
	}

	void SceneManager::findVisibleObject()
	{
		//todo quad tree , now temporarily place all nodes under the root node
		std::function<void(SceneNode*)> checkFunc = [this, &checkFunc](SceneNode* InNode)
			{
				if (InNode == nullptr)
					return;

				if (InNode->isVisible())
				{
					mVisibleNodes.push_back(InNode);
					for (auto&& ChildNode : InNode->GetChildNodes())
					{
						if (ChildNode->isVisible())
						{
							checkFunc(ChildNode);
						}
					}
				}
			};

		checkFunc(mRootNode);

	}

	SceneNode* SceneManager::createSceneNode()
	{
		SceneNode* node = new SceneNode(this);
		mSceneNodeSets.insert(node);
		return node;
	}

	SceneNode* SceneManager::createSceneNode(const String& name)
	{
		SceneNode* node = new SceneNode(this, name);
		mSceneNodeSets.insert(node);
		return node;
	}

	void SceneManager::destroySceneNode(SceneNode* InSceneNode)
	{
		mSceneNodeSets.erase(InSceneNode);
		SAFE_DELETE(InSceneNode);
	}

	void SceneManager::RenderScene()
	{
		mRootNode->update();
		mRenderQueueSet->ClearRenderQueue();

		findVisibleObject();

		for (auto&& CurrentNode : mVisibleNodes)
		{
			CurrentNode->UpdateRenderQueue(mRenderQueueSet);
		}

		mVisibleNodes.clear();

		//record render commands
		RHICommandList* NewCmdList = GetDynamicRHI()->GetDefaultCommandContext()->GetRHICommandListMgr()->CreateCommandList();

		UpdateCommonUniformBuffer(NewCmdList);

		for (size_t Index = RenderQueue_Normal; Index < RenderQueue_Count; ++Index)
		{
			RenderQueue* CurrentRenderQueue = mRenderQueueSet->GetRenderQueue((NamedRenderQueue)Index);
			CurrentRenderQueue->Render(NewCmdList);
		}
	}

	void SceneManager::RenderThreadMain()
	{
		while (!mbExit)
		{
			WaitForRenderThread_Gfx();
			RenderScene();
			NotifyForRHIThread_Gfx();
		}
	}

	void SceneManager::UpdateCommonUniformBuffer(RHICommandList* InRHICmdList)
	{
		float radius = 1.0f;
		float camX = Root::instance()->getTimer()->getSeconds() * radius;
		mMainCamera->SetRotation(Vector3(0.0f, camX, 0.0f));

		mCommonUniformBuffer->U_ViewMatrix = mMainCamera->GetViewMatrix();
		mCommonUniformBuffer->U_ProjectionMatrix = mMainCamera->GetProjMatrix();
		mCommonUniformBuffer->U_ViewProjectionMatrix = mMainCamera->GetViewProjMatrix();
		mCommonUniformBuffer->U_CameraDirection = Vector4(mMainCamera->GetForward(), 0.0f);
		mCommonUniformBuffer->U_CameraPosition = Vector4(mMainCamera->GetPosition(), 1.0f);

		InRHICmdList->UpdateCommonUniformBuffer(sizeof(CommonUniformBufferCPU), mCommonUniformBuffer);
	}
}