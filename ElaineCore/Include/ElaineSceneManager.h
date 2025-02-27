#pragma once
#include "ElaineQuadTree.h"
#include "render/common/ElaineRHICommandList.h"

namespace Elaine
{
	class ThreadWrap;
	class SkyObject;
	class RenderQueueSet;
	class RHICommandContext;
	class ElaineCoreExport SceneManager
	{
	public:
		SceneManager(const String& name);
		~SceneManager();

		Camera* getMainCamera();
		void findVisibleObject();
		SceneNode* createSceneNode();
		SceneNode* createSceneNode(const String& name);
		void destroySceneNode(SceneNode* InSceneNode);
		void RenderScene();
		void RenderThreadMain();
	private:
		String mName;
		Camera* mMainCamera = nullptr;
		Camera* mSecondCamera = nullptr;
		QuadTree* mQuadTree = nullptr;
		std::set<SceneNode*> mSceneNodeSets;
		SceneNode* mRootNode = nullptr;
		ThreadWrap* mRenderThread = nullptr;
		std::vector<SceneNode*> mVisibleNodes;
		//test code todo
		SkyObject* mSky = nullptr;
		RenderQueueSet* mRenderQueueSet = nullptr;
		RHICommandContext* mRHICommandCtx = nullptr;
		bool mbExit = false;
	};
}