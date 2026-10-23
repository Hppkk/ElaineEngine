#include "ElainePrecompiledHeader.h"
#include "ElaineShaderManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineShader.h"
#include "ElaineMD5.h"
#include "render/common/ElaineRHI.h"
#include <fstream>

namespace Elaine
{
    struct Shader::LoadData : ResourceLoadData
    {
        std::string mShaderCode, SourcePath;
        JsonCpp SavedJson;
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
        Files.push_back(mSourcePath); Files.push_back(mResourceName); return Files;
    }
    std::shared_ptr<const RHIShaderCompileResult> Shader::GetPreparedShader() const { return mPrepared; }
	std::unique_ptr<ResourceLoadData> Shader::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
        std::ifstream DescriptorFile(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName, std::ios::binary);
        Data.SavedJson = JsonCpp::parse(DescriptorFile);
        Data.SourcePath = CanonicalResourcePath(Data.SavedJson.at("source").get<std::string>());
        DataStream Stream(Root::instance()->GetResourcePath() + Data.SourcePath, DataStream::In | DataStream::Binary);
        Stream.ReadAll();
        if (!Stream.GetDataStream() || Stream.GetSize() <= 1) return {};
        std::string NewCode(Stream.GetDataStream(), Stream.GetSize() - 1);
        while (!NewCode.empty() && NewCode.back() == '\0') NewCode.pop_back();
        if (NewCode.empty()) return {};

		Data.mShaderCode = NewCode;
		Data.mHashKey = GenerateShaderHashKey(Data.mShaderCode);
        auto Ext = Data.SavedJson.at("stage").get<std::string>();
        EShaderStage Stage;
        if (Ext == "vertex") Stage = EShaderStage::VertexShader;
        else if (Ext == "fragment") Stage = EShaderStage::FragmentShader;
        else if (Ext == "geometry") Stage = EShaderStage::GeometryShader;
        else if (Ext == "compute") Stage = EShaderStage::ComputeShader;
        else throw std::runtime_error("Shader stage cannot be inferred from extension");
        auto* RHI = GetDynamicRHI();
        if (!RHI) throw std::runtime_error("Shader preparation requires an active RHI backend");
        Data.mPrepared = RHI->PrepareShader(Data.mShaderCode, (std::filesystem::path(Root::instance()->GetResourcePath()) / Data.SourcePath).generic_string(), Stage);
        if (!Data.mPrepared) throw std::runtime_error("Shader preparation returned no compilation output");
        Data.mHashKey = GenerateShaderHashKey(Data.mShaderCode + ":" + std::to_string(Data.mPrepared->DeclarationSignature));
		Data.MemoryUsage = Data.mShaderCode.size();
        return Pending;
	}


	void Shader::UnloadImpl() { mShaderCode.clear(); mHashKey = 0; mPrepared.reset(); }

    EShaderStage Shader::GetStage() const
    {
        const auto Stage = mSavedJson.value("stage", "");
        if (Stage == "vertex") return EShaderStage::VertexShader;
        if (Stage == "fragment") return EShaderStage::FragmentShader;
        if (Stage == "geometry") return EShaderStage::GeometryShader;
        if (Stage == "compute") return EShaderStage::ComputeShader;
        throw std::runtime_error("Invalid shader stage");
    }
    void Shader::SaveResourceImpl()
    {
        auto Json = mSavedJson; Json["resource"] = mDescriptor.ToJson();
        const auto Text = Json.dump(2);
        AssetFile::AtomicWrite(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName, {Text.begin(), Text.end()});
    }


    void Shader::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        mSavedJson.swap(Data.SavedJson); mSourcePath.swap(Data.SourcePath);
        std::swap(mShaderCode, Data.mShaderCode);
        std::swap(mPrepared, Data.mPrepared);
        std::swap(mHashKey, Data.mHashKey);
    }

}
