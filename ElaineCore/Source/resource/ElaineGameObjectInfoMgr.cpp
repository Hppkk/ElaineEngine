#include "ElainePrecompiledHeader.h"
#include "resource/ElaineGameObjectInfoMgr.h"

namespace Elaine
{
	GameObjectInfoMgr::GameObjectInfoMgr()
	{
		m_sResType = GetTypeStringName(GameObjectInfo);
	}

	GameObjectInfoMgr::~GameObjectInfoMgr()
	{

	}

	ResourceBase* GameObjectInfoMgr::createResource(const std::string& path)
	{
		if (path.empty())
			return new EGameObjectInfo();
		return new EGameObjectInfo(this, path);
	}
}