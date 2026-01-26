#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineModuleBase.h"
#include "ElaineResourceManager.h"

namespace Elaine
{


	class ElaineCoreExport ResourceModule : public ModuleBase
	{
	public:
		ResourceModule() = default;
		virtual ~ResourceModule() override;
		virtual void Initialize() override;
		virtual void Terminate() override;
		void RegisterResourceLoader(ResourceType InType, ResourceManager* InManager);
		void UnregisterResourceLoader(ResourceType InType);
	private:
		std::unordered_map<ResourceType, ResourceManager*> mResourceManagers;
	};
}