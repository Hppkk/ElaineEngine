#include "ElainePrecompiledHeader.h"
#include "ElaineSceneManager.h"

namespace Elaine
{
	SceneManager::SceneManager(const String& name)
		: mName(name)
	{

	}

	Camera* SceneManager::getMainCamera()
	{
		return mMainCamera;
	}

	void SceneManager::findVisibleObject()
	{

	}
}