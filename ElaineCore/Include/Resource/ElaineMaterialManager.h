#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineMaterial.h"

namespace Elaine
{
	class ElaineCoreExport MaterialManager : public ResourceManager, public Singleton<MaterialManager>
	{
	public:
		MaterialManager();
		virtual ~MaterialManager() override;
        MaterialPtr GetResource(const std::string& Path)
        { return static_pointer_cast<Material>(ResourceManager::GetResource(Path)); }
        MaterialPtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<Material>(ResourceManager::GetResource(Id)); }
        MaterialPtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<Material>(ResourceManager::GetResource(Reference)); }
        MaterialPtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<Material>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}