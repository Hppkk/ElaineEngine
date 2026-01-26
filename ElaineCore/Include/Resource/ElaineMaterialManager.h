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
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}