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
	protected:
		virtual	ResourceBasePtr		CreateResourceImpl(const std::string& InPath) override;
	};
}