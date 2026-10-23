#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourceBase.h"

namespace Elaine
{
    struct RHIShaderCompileResult;
	class ElaineCoreExport Shader : public ResourceBase
	{
	public:
		Shader(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~Shader();
		uint64_t GetHash() const;
		std::string GetShaderCode() const;
        std::vector<std::string> GetSourceFiles() const override;
        std::shared_ptr<const RHIShaderCompileResult> GetPreparedShader() const;
        EShaderStage GetStage() const;
	protected:
		void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
        std::unique_ptr<ResourceLoadData> LoadImpl() override;
		virtual	void UnloadImpl() override;
		virtual void SaveResourceImpl() override;
	private:
        struct LoadData;
        JsonCpp mSavedJson;
        std::string mSourcePath;

		std::string mShaderCode;
        std::shared_ptr<const RHIShaderCompileResult> mPrepared;
		uint64_t mHashKey = 0;
		mutable std::mutex mShaderMutex;
	};

	using ShaderPtr = ResourcePtr<Shader>;

	struct ShaderStageEntry
	{
		EShaderStage mStage;
		ResourcePtr<Shader> mShader;
        ResourceReference mReference;
	};
}
