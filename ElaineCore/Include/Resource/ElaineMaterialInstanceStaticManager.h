#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineMaterialInstanceStatic.h"

namespace Elaine
{
	class ElaineCoreExport MaterialInstanceStaticManager : public ResourceManager, public Singleton<MaterialInstanceStaticManager>
	{
	public:
		MaterialInstanceStaticManager();
		virtual ~MaterialInstanceStaticManager() override;
        MaterialInstanceStaticPtr GetResource(const std::string& Path)
        { return static_pointer_cast<MaterialInstanceStatic>(ResourceManager::GetResource(Path)); }
        MaterialInstanceStaticPtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<MaterialInstanceStatic>(ResourceManager::GetResource(Id)); }
        MaterialInstanceStaticPtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<MaterialInstanceStatic>(ResourceManager::GetResource(Reference)); }
        MaterialInstanceStaticPtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<MaterialInstanceStatic>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}