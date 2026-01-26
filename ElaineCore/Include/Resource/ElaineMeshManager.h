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
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}