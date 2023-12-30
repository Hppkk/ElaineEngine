#include "ElainePrecompiledHeader.h"
#include "ElaineSceneManager.h"

namespace Elaine
{
	SceneManager::SceneManager(const String& name)
		: mName(name)
	{

	}

	SceneManager::~SceneManager()
	{
		for (SceneNode* node : mSceneNodeSets)
		{
			SAFE_DELETE(node);
		}
	}

	Camera* SceneManager::getMainCamera()
	{
		return mMainCamera;
	}

	void SceneManager::findVisibleObject()
	{
		if (mRootNode->isVisible())
			mRootNode->findVisibilityObject();


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
}