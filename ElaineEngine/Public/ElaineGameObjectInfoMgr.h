#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineResourceManager.h"

namespace Elaine
{
	class ElaineEngineExport GameObjectInfoMgr :public ResourceManager, public Singleton<GameObjectInfoMgr>
	{
	public:
		GameObjectInfoMgr();
		~GameObjectInfoMgr();
	protected:
		virtual	ResourceBasePtr CreateResourceImpl(const std::string& InPath) override;
	};
}