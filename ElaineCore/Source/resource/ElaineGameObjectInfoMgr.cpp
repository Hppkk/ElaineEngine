#include "ElainePrecompiledHeader.h"
#include "resource/ElaineGameObjectInfoMgr.h"

namespace Elaine
{
	//std::string GameObjectInfoMgr::m_sResType = GetTypeStringName(GameObjectInfo);

	GameObjectInfoMgr::GameObjectInfoMgr()
	{
		
	}

	GameObjectInfoMgr::~GameObjectInfoMgr()
	{

	}

	ResourceBasePtr GameObjectInfoMgr::createResourceImpl(const std::string& path)
	{
		if (path.empty())
			return new EGameObjectInfo();
		return new EGameObjectInfo(this, path);
	}
}