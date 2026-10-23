#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineResourceManager.h"
#include "GamePlay/ElaineActor.h"

namespace Elaine
{
	class ElaineEngineExport ActorInfoMgr :public ResourceManager, public Singleton<ActorInfoMgr>
	{
	public:
		ActorInfoMgr();
		~ActorInfoMgr();
        ActorInfoPtr GetResource(const std::string& Path)
        { return static_pointer_cast<ActorInfo>(ResourceManager::GetResource(Path)); }
        ActorInfoPtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<ActorInfo>(ResourceManager::GetResource(Id)); }
        ActorInfoPtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<ActorInfo>(ResourceManager::GetResource(Reference)); }
        ActorInfoPtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<ActorInfo>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr CreateResourceImpl(const std::string& InPath) override;
	};
}