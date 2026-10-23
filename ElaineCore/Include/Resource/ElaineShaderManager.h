#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineShader.h"

namespace Elaine
{
	class ElaineCoreExport ShaderManager : public ResourceManager, public Singleton<ShaderManager>
	{
	public:
		ShaderManager();
		virtual ~ShaderManager() override;
        ShaderPtr GetResource(const std::string& Path)
        { return static_pointer_cast<Shader>(ResourceManager::GetResource(Path)); }
        ShaderPtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<Shader>(ResourceManager::GetResource(Id)); }
        ShaderPtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<Shader>(ResourceManager::GetResource(Reference)); }
        ShaderPtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<Shader>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}