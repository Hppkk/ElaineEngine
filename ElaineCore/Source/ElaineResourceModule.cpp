#include "ElainePrecompiledHeader.h"
#include "ElaineResourceModule.h"
#include "Resource/ElaineResourceLoader.h"
#include "ElaineTextureManager.h"
#include "Resource/ElaineResourceRegistry.h"
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
		new ResourceLoader();
        ResourceLoader::instance()->Start();
		RegisterResourceLoader(RT_Texture, new TextureManager());
		RegisterResourceLoader(RT_Mesh, new MeshManager());
		RegisterResourceLoader(RT_Material, new MaterialManager());
		RegisterResourceLoader(RT_MaterialInstance, new MaterialInstanceStaticManager());
		RegisterResourceLoader(RT_Shader, new ShaderManager());
        ResourceRegistry::Get().Start(Root::instance()->GetResourcePath());
        ResourceRegistry::Get().WaitForStartup();
	}

	void ResourceModule::Terminate()
	{
		delete ResourceLoader::instance();
		for (uint8_t Type = 0; Type < RT_Count; ++Type)
			UnregisterResourceLoader(static_cast<ResourceType>(Type));
	}

	void ResourceModule::RegisterResourceLoader(ResourceType InType, ResourceManager* InManager)
	{
		mResourceManagers[InType] = InManager;
		ResourceLoader::instance()->RegisterManager(InType, InManager);
	}

	void ResourceModule::UnregisterResourceLoader(ResourceType InType)
	{
		SAFE_DELETE(mResourceManagers[InType]);
	}
}
