#include <ElainePrecompiledHeader.h>
#include <ElaineShaderManager.h>

namespace Elaine
{
	ShaderManager::ShaderManager()
		: ResourceManager(RT_Shader)
	{

	}

	ShaderManager::~ShaderManager()
	{

	}

	ResourceBasePtr ShaderManager::CreateResourceImpl(const std::string& InPath)
	{
		return ResourcePtr<Shader>(new Shader(this, InPath));
	}
}