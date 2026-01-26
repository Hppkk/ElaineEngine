#include "ElainePrecompiledHeader.h"
#include "ElaineGameObjectInfoMgr.h"
#include "GamePlay/ElaineGameObject.h"

namespace Elaine
{
	GameObjectInfoMgr::GameObjectInfoMgr()
		: ResourceManager(RT_GameObject)
	{
		
	}

	GameObjectInfoMgr::~GameObjectInfoMgr()
	{

	}

	ResourceBasePtr Elaine::GameObjectInfoMgr::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<GameObjectInfo>(new GameObjectInfo(this, InPath));
	}
}