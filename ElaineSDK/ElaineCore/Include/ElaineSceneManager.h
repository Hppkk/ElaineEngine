#pragma once
#include "ElaineQuadTree.h"

namespace Elaine
{
	class ElaineCoreExport SceneManager
	{
	public:
		SceneManager(const String& name);

		Camera* getMainCamera();
		void findVisibleObject();
	private:
		String mName;
		Camera* mMainCamera = nullptr;
		Camera* mSecondCamera = nullptr;
		QuadTree* mQuadTree = nullptr;
	};
}