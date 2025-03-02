#include "ElainePrecompiledHeader.h"
#include "ElaineSceneManager.h"
#include "ElaineThreadManager.h"
#include "ElaineSky.h"
#include "ElaineRenderQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	SceneManager::SceneManager(const String& name)
		: mName(name)
	{
		mRootNode = createSceneNode();
		mRenderQueueSet = new RenderQueueSet();
		mSky = new SkyObject(mRootNode, this);
		mRHICommandCtx = GetDynamicRHI()->GetDefaultCommandContext();
		mRenderThread = ThreadManager::instance()->GetOrCreateThread(NamedThread::RenderThread, &SceneManager::RenderThreadMain, this);
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

	Camera* SceneManager::getMainCamera()
	{
		return mMainCamera;
	}

	void SceneManager::findVisibleObject()
	{
		//todo quad tree , now temporarily place all nodes under the root node

		if (mRootNode->isVisible())
		{
			mVisibleNodes.push_back(mRootNode);
			for (auto&& ChildNode : mRootNode->GetChildNodes())
			{
				if (ChildNode->isVisible())
				{
					mVisibleNodes.push_back(ChildNode);
				}
			}
		}
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
		for (size_t Index = RenderQueue_Normal; Index < RenderQueue_Count; ++Index)
		{
			RenderQueue* CurrentRenderQueue = mRenderQueueSet->GetRenderQueue((NamedRenderQueue)Index);
			CurrentRenderQueue->Render(NewCmdList);
		}
	}

	void SceneManager::RenderThreadMain()
	{
		static bool temp = false;
		while (!mbExit)
		{
			if(temp)
				WaitForRenderThread_Gfx();
			RenderScene();
			NotifyForRHIThread_Gfx();
			temp = true;
		}
	}
}