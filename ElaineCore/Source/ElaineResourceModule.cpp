#include "ElainePrecompiledHeader.h"
#include "ElaineResourceModule.h"
#include "ElaineTextureManager.h"
#include "ElaineShaderManager.h"
#include "ElaineMeshManager.h"
#include "ElaineMaterialManager.h"
#include "ElaineMaterialInstanceStaticManager.h"

namespace Elaine
{
	ResourceModule::~ResourceModule()
	{

	}

	void ResourceModule::Initialize()
	{
		RegisterResourceLoader(RT_Texture, new TextureManager());
		RegisterResourceLoader(RT_Mesh, new MeshManager());
		RegisterResourceLoader(RT_Material, new MaterialManager());
		RegisterResourceLoader(RT_MaterialInstance, new MaterialInstanceStaticManager());
		RegisterResourceLoader(RT_Shader, new ShaderManager());

	}

	void ResourceModule::Terminate()
	{
		UnregisterResourceLoader(RT_Texture);
		UnregisterResourceLoader(RT_GameObject);
	}

	void ResourceModule::RegisterResourceLoader(ResourceType InType, ResourceManager* InManager)
	{
		mResourceManagers[InType] = InManager;
	}

	void ResourceModule::UnregisterResourceLoader(ResourceType InType)
	{
		SAFE_DELETE(mResourceManagers[InType]);
	}
}