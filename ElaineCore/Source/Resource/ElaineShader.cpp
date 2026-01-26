#include "ElainePrecompiledHeader.h"
#include "ElaineShaderManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineShader.h"
#include "ElaineMD5.h"

namespace Elaine
{
	static uint64_t GenerateShaderHashKey(const std::string& InShaderCode)
	{
		return FNV_1A_HASH_64(InShaderCode);
	}

	Shader::Shader(ResourceManager* InManager, const std::string& InResourceName)
		: ResourceBase(InManager, InResourceName)
	{

	}

	Shader::~Shader()
	{

	}

	uint64_t Shader::GetHash() const
	{
		return 0;
	}

	bool Shader::LoadImpl()
	{
		DataStream Stream(Root::instance()->GetResourcePath() + "shader/vulkan/" + mResourceName);
		Stream.ReadAll();
		mShaderCode = Stream.GetDataStream();
		mHashKey = GenerateShaderHashKey(mShaderCode);
		return true;
	}

	void Shader::UnloadImpl()
	{

	}

	void Shader::SaveResourceImpl()
	{

	}

	void Shader::ResourceArrivedImpl()
	{

	}
}
