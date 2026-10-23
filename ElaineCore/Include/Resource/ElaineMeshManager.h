#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceManager.h"
#include "ElaineMesh.h"

namespace Elaine
{
	class ElaineCoreExport MeshManager : public ResourceManager, public Singleton<MeshManager>
	{
	public:
		MeshManager();
		virtual ~MeshManager() override;
        MeshPtr GetResource(const std::string& Path)
        { return static_pointer_cast<Mesh>(ResourceManager::GetResource(Path)); }
        MeshPtr GetResource(const ResourceId& Id)
        { return static_pointer_cast<Mesh>(ResourceManager::GetResource(Id)); }
        MeshPtr GetResource(const ResourceReference& Reference)
        { return static_pointer_cast<Mesh>(ResourceManager::GetResource(Reference)); }
        MeshPtr ReloadResource(const std::string& Path)
        { return static_pointer_cast<Mesh>(ResourceManager::ReloadResource(Path)); }
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}