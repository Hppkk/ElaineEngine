#include <ElainePrecompiledHeader.h>
#include <ElaineMaterialManager.h>

namespace Elaine
{
	//material example:
	//{
	//  "version": "0.0.1",
	//	"shader": {
	//		"vs": "test.vs",
	//		"ps": "test.ps"
	//  },
	//  "textures":{
	//		
	//  },
	//  "samplers": {
	//		
	//  },
	//  "shaderpass": {
	//		"pbr": {
	//			"defines": ["USE_POINT_LIGHT"]
	//			"depth_stencil_state": {
	//			}
	//		}
	//  },
	//}

	MaterialManager::MaterialManager()
		: ResourceManager(RT_Material)
	{

	}

	MaterialManager::~MaterialManager()
	{

	}

	ResourceBasePtr Elaine::MaterialManager::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<Material>(new Material(this, InPath));
	}
}