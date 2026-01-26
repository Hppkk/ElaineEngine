#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInstanceStaticManager.h"

namespace Elaine
{
	MaterialInstanceStaticManager::MaterialInstanceStaticManager()
		: ResourceManager(RT_MaterialInstance)
	{

	}

	MaterialInstanceStaticManager::~MaterialInstanceStaticManager()
	{

	}

	ResourceBasePtr MaterialInstanceStaticManager::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<MaterialInstanceStatic>(new MaterialInstanceStatic(this, InPath));
	}
}