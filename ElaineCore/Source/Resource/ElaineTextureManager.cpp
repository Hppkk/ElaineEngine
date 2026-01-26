#include "ElainePrecompiledHeader.h"
#include "ElaineTextureManager.h"

namespace Elaine
{
	TextureManager::TextureManager()
		: ResourceManager(RT_Texture)
	{

	}

	TextureManager::~TextureManager()
	{

	}

	ResourceBasePtr Elaine::TextureManager::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<Texture>(new Texture(this, InPath));
	}
}