#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineResourceManager.h"

namespace Elaine
{
	class ElaineEngineExport ActorInfoMgr :public ResourceManager, public Singleton<ActorInfoMgr>
	{
	public:
		ActorInfoMgr();
		~ActorInfoMgr();
	protected:
		virtual	ResourceBasePtr CreateResourceImpl(const std::string& InPath) override;
	};
}