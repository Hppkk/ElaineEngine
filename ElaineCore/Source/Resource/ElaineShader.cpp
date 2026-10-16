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
		std::lock_guard<std::mutex> Lock(mShaderMutex);
		return mHashKey;
	}

	std::string Shader::GetShaderCode() const
	{
		std::lock_guard<std::mutex> Lock(mShaderMutex);
		return mShaderCode;
	}

	bool Shader::LoadImpl()
	{
		DataStream Stream(Root::instance()->GetResourcePath() + "shader/vulkan/" + mResourceName);
		Stream.ReadAll();
		if (Stream.GetSize() == 0 || Stream.GetDataStream() == nullptr)
			return false;
		const std::string NewCode(Stream.GetDataStream(), Stream.GetSize() > 0 ? Stream.GetSize() - 1 : 0);
		if (NewCode.empty()) return false;
		std::lock_guard<std::mutex> Lock(mShaderMutex);
		mShaderCode = NewCode;
		mHashKey = GenerateShaderHashKey(mShaderCode);
		return true;
	}

	void Shader::ReloadResource()
	{
		std::lock_guard<std::recursive_mutex> Lock(mLifecycleMutex);
		std::string OldCode;
		uint64_t OldHash = 0;
		{
			std::lock_guard<std::mutex> ShaderLock(mShaderMutex);
			OldCode = mShaderCode;
			OldHash = mHashKey;
		}
		bool Reloaded = false;
		try { Reloaded = LoadImpl(); }
		catch (...) { Reloaded = false; }
		if (!Reloaded)
		{
			std::lock_guard<std::mutex> ShaderLock(mShaderMutex);
			mShaderCode = OldCode;
			mHashKey = OldHash;
			mLoadState = Loaded;
			LOG_ERROR("Shader hot reload failed; keeping previous shader: {}", mResourceName);
			return;
		}
		mLoadState = Loaded;
		mResourceArrived.store(true, std::memory_order_release);
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
