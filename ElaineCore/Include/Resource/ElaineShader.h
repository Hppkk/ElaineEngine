#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceBase.h"

namespace Elaine
{
	class ElaineCoreExport Shader : public ResourceBase
	{
	public:
		Shader(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~Shader();
		uint64_t GetHash() const;
		const std::string& GetShaderCode() const { return mShaderCode; }
	protected:
		virtual bool LoadImpl() override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
		virtual void ResourceArrivedImpl() override;
	private:
		std::string mShaderCode;
		uint64_t mHashKey = 0;
	};

	using ShaderPtr = ResourcePtr<Shader>;

	struct ShaderStageEntry
	{
		EShaderStage mStage;
		ResourcePtr<Shader> mShader;
	};
}