#pragma once
#include "resource/ElaineResourceManager.h"

namespace Elaine
{
	class ElaineCoreExport GameObjectInfoMgr :public ResourceManager
	{
		GameObjectInfoMgr();
		~GameObjectInfoMgr();
		virtual	ResourceBase* createResource(const std::string& path) override;
	};
}