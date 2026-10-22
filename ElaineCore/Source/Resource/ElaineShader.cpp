#include "ElainePrecompiledHeader.h"
#include "ElaineShaderManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineShader.h"
#include "ElaineMD5.h"
#include "render/common/ElaineRHI.h"

namespace Elaine
{
    struct Shader::LoadData : ResourceLoadData
    {
        std::string mShaderCode;
        std::shared_ptr<const RHIShaderCompileResult> mPrepared;
        uint64_t mHashKey = 0;
    };


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

	uint64_t Shader::GetHash() const { return mHashKey; }

	std::string Shader::GetShaderCode() const
	{
        std::lock_guard<std::mutex> Lock(mShaderMutex);
		return mShaderCode;
	}

    std::vector<std::string> Shader::GetSourceFiles() const
    {
        auto Files = mPrepared ? mPrepared->SourceFiles : std::vector<std::string>();
        Files.push_back("shader/vulkan/" + mResourceName); return Files;
    }
    std::shared_ptr<const RHIShaderCompileResult> Shader::GetPreparedShader() const { return mPrepared; }
	std::unique_ptr<ResourceLoadData> Shader::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
        DataStream Stream(Root::instance()->GetResourcePath() + "shader/vulkan/" + mResourceName, DataStream::In | DataStream::Binary);
        Stream.ReadAll();
        if (!Stream.GetDataStream() || Stream.GetSize() <= 1) return {};
        std::string NewCode(Stream.GetDataStream(), Stream.GetSize() - 1);
        while (!NewCode.empty() && NewCode.back() == '\0') NewCode.pop_back();
        if (NewCode.empty()) return {};

		Data.mShaderCode = NewCode;
		Data.mHashKey = GenerateShaderHashKey(Data.mShaderCode);
        auto Ext = std::filesystem::path(mResourceName).extension().string();
        EShaderStage Stage;
        if (Ext == ".vs" || Ext == ".vert") Stage = EShaderStage::VertexShader;
        else if (Ext == ".ps" || Ext == ".frag") Stage = EShaderStage::FragmentShader;
        else if (Ext == ".gs") Stage = EShaderStage::GeometryShader;
        else if (Ext == ".cs") Stage = EShaderStage::ComputeShader;
        else throw std::runtime_error("Shader stage cannot be inferred from extension");
        auto* RHI = GetDynamicRHI();
        if (!RHI) throw std::runtime_error("Shader preparation requires an active RHI backend");
        Data.mPrepared = RHI->PrepareShader(Data.mShaderCode, mResourceName, Stage);
        if (!Data.mPrepared) throw std::runtime_error("Shader preparation returned no compilation output");
        Data.mHashKey = GenerateShaderHashKey(Data.mShaderCode + ":" + std::to_string(Data.mPrepared->DeclarationSignature));
		Data.MemoryUsage = Data.mShaderCode.size();
        return Pending;
	}


	void Shader::UnloadImpl() { mShaderCode.clear(); mHashKey = 0; mPrepared.reset(); }

	void Shader::SaveResourceImpl()
	{

	}


    void Shader::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        std::swap(mShaderCode, Data.mShaderCode);
        std::swap(mPrepared, Data.mPrepared);
        std::swap(mHashKey, Data.mHashKey);
    }

}
