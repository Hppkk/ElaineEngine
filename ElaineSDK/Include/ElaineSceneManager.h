#pragma once
#include "ElaineQuadTree.h"

namespace Elaine
{
	class ElaineCoreExport SceneManager
	{
	public:
		SceneManager(const String& name);
		~SceneManager();

		Camera* getMainCamera();
		void findVisibleObject();
		SceneNode* createSceneNode();
		SceneNode* createSceneNode(const String& name);
		
	private:
		String mName;
		Camera* mMainCamera = nullptr;
		Camera* mSecondCamera = nullptr;
		QuadTree* mQuadTree = nullptr;
		std::set<SceneNode*> mSceneNodeSets;
		SceneNode* mRootNode = nullptr;
	};
}