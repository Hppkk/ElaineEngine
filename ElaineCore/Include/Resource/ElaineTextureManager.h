#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineTextureResource.h"

namespace Elaine
{
	class ElaineCoreExport TextureManager : public ResourceManager, public Singleton<TextureManager>
	{
	public:
		TextureManager();
		virtual ~TextureManager() override;
        TexturePtr GetResource(const std::string& Path)
        { return static_pointer_cast<Texture>(ResourceManager::GetResource(Path)); }
        TexturePtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<Texture>(ResourceManager::GetResource(Id)); }
        TexturePtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<Texture>(ResourceManager::GetResource(Reference)); }
        TexturePtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<Texture>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}