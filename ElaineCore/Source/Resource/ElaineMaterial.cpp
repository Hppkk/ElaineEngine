#include "ElainePrecompiledHeader.h"
#include "ElaineMaterial.h"
#include "ElaineShaderManager.h"
#include "ElaineShaderPassManager.h"
#include "ElaineTextureManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "Resource/ElaineResourceManager.h"


namespace Elaine
{
    struct Material::LoadData : ResourceLoadData
    {
        Version mVersion;
        TexturePtr mTextures[Tex_Count];
        std::unordered_map<Name, ShaderPass*> mPassMap;
        std::unordered_map<uint64_t, float> mScalarDefaults;
        std::unordered_map<uint64_t, Vector4> mVectorDefaults;
        std::unordered_map<uint64_t, int32_t> mIntDefaults;
        std::unordered_map<uint64_t, bool> mBoolDefaults;
        std::vector<MaterialParameterDesc> mParameterLayout;
        ~LoadData() override { for (auto& Pair : mPassMap) delete Pair.second; }
    };


	Material::Material(ResourceManager* InManager, const std::string& InResourceName)
		: ResourceBase(InManager, InResourceName)
	{
		
	}

	float Material::GetScalar(uint64_t InParamHash, float InDefault) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mScalarDefaults.find(InParamHash);
		return It == mScalarDefaults.end() ? InDefault : It->second;
	}

	Vector4 Material::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mVectorDefaults.find(InParamHash);
		return It == mVectorDefaults.end() ? InDefault : It->second;
	}

	int32_t Material::GetInt(uint64_t InParamHash, int32_t InDefault) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mIntDefaults.find(InParamHash);
		return It == mIntDefaults.end() ? InDefault : It->second;
	}

	bool Material::GetBool(uint64_t InParamHash, bool InDefault) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mBoolDefaults.find(InParamHash);
		return It == mBoolDefaults.end() ? InDefault : It->second;
	}

	bool Material::HasScalar(uint64_t InParamHash) const { std::lock_guard<std::mutex> Lock(mDataMutex); return mScalarDefaults.find(InParamHash) != mScalarDefaults.end(); }
	bool Material::HasVector(uint64_t InParamHash) const { std::lock_guard<std::mutex> Lock(mDataMutex); return mVectorDefaults.find(InParamHash) != mVectorDefaults.end(); }
	std::vector<MaterialParameterDesc> Material::GetParameterLayoutSnapshot() const { std::lock_guard<std::mutex> Lock(mDataMutex); return mParameterLayout; }

	Material::~Material()
	{
        for (auto& Pair : mPassMap) delete Pair.second;
	}

	void Material::SetTexture(TextureSemantics InSemantics, TexturePtr InTexture)
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		mTextures[InSemantics] = InTexture;
		mbDirty = true;
	}

	TexturePtr Material::GetTexture(TextureSemantics InSemantics) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		return mTextures[InSemantics];
	}

	std::unique_ptr<ResourceLoadData> Material::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
		DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
		Stream.ReadAll();
		JsonCpp JsonMaterial = JsonCpp::parse(Stream.GetDataStream());
		if (!JsonMaterial.contains("version") || JsonMaterial["version"] != "2.0.0")
		{
			LOG_ERROR("Material '%s' requires format version 2.0.0.", mResourceName.c_str());
			return {};
		}
		Data.mVersion = Version(JsonMaterial["version"]);
		Data.mScalarDefaults.clear();
		Data.mVectorDefaults.clear();
		Data.mIntDefaults.clear();
		Data.mBoolDefaults.clear();
		Data.mParameterLayout.clear();
		for (auto& [PassName, Pass] : Data.mPassMap)
			if (Pass) ShaderPassManager::instance()->DestroyShaderPass(Pass);
		Data.mPassMap.clear();
		for (auto& Texture : Data.mTextures)
			Texture = TexturePtr();

		const auto& Parameters = JsonMaterial["parameters"];
		if (Parameters.is_object())
		{
			for (auto& It : Parameters.items())
			{
				const uint64_t Hash = Name(It.key().c_str()).GetHashValue();
				const auto& Value = It.value();
				const std::string Type = Value.value("type", "");
				const auto& Default = Value.contains("default") ? Value["default"] : JsonCpp();
				MaterialParameterDesc Desc;
				Desc.Name = It.key();
				Desc.Hash = Hash;
				if (Type == "scalar") Desc.Kind = MaterialParameterKind::Scalar;
				else if (Type == "vector") Desc.Kind = MaterialParameterKind::Vector;
				else if (Type == "color") Desc.Kind = MaterialParameterKind::Color;
				else if (Type == "int") Desc.Kind = MaterialParameterKind::Integer;
				else if (Type == "bool") Desc.Kind = MaterialParameterKind::Boolean;
				else if (Type == "texture") Desc.Kind = MaterialParameterKind::Texture;
				if (Value.contains("min") && Value.contains("max")) { Desc.Min = Value["min"].get<float>(); Desc.Max = Value["max"].get<float>(); Desc.HasRange = true; }
				if (Type == "scalar" && Default.is_number())
					Data.mScalarDefaults[Hash] = Default.get<float>();
				else if ((Type == "vector" || Type == "color") && Default.is_array())
					Data.mVectorDefaults[Hash] = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f,
						Default.size() > 1 ? Default[1].get<float>() : 0.0f,
						Default.size() > 2 ? Default[2].get<float>() : 0.0f,
						Default.size() > 3 ? Default[3].get<float>() : 0.0f);
				else if (Type == "int" && Default.is_number_integer())
					Data.mIntDefaults[Hash] = Default.get<int32_t>();
				else if (Type == "bool" && Default.is_boolean())
					Data.mBoolDefaults[Hash] = Default.get<bool>();
				if (Default.is_number()) Desc.DefaultValue = Vector4(Default.get<float>(), 0, 0, 0);
				else if (Default.is_array()) Desc.DefaultValue = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f, Default.size() > 1 ? Default[1].get<float>() : 0.0f, Default.size() > 2 ? Default[2].get<float>() : 0.0f, Default.size() > 3 ? Default[3].get<float>() : 0.0f);
				Desc.HasDefault = !Default.is_null();
				Data.mParameterLayout.push_back(std::move(Desc));
			}
		}

		//ClearDependencies();

		JsonCpp TextureListJson = JsonMaterial["textures"];
		if (!TextureListJson.empty())
		{
			for (auto&& tex : TextureListJson.items())
			{
				std::string TexPath = tex.value();
				
				TextureSemantics TS = SemanticsRegister::GetSemantics(tex.key().c_str());
				Data.mTextures[TS] = TextureManager::instance()->RequestResource(TexPath).GetResult().Resource;
				ResourceBasePtr BasePtr = Data.mTextures[TS];
			}
		}

		// 解析ShaderPass (内部会加载Shader)
		JsonCpp ShaderPassJson = JsonMaterial["shaderpass"];
		if (!ShaderPassJson.empty())
		{
			for (auto&& passJson : ShaderPassJson)
			{
				std::unique_ptr<ShaderPass> NewShaderPass(ShaderPassManager::instance()->CreateShaderPass(passJson));
                if (!NewShaderPass) throw std::runtime_error("Invalid material shader pass");
                if (!Data.mPassMap.emplace(NewShaderPass->GetPassName(), NewShaderPass.get()).second)
                    throw std::runtime_error("Duplicate material shader pass name");
                NewShaderPass.release();
			}
		}

		for (const auto& Texture : Data.mTextures) if (Texture) Data.Dependencies.push_back(Texture);
        for (const auto& Pair : Data.mPassMap) Pair.second->GetDependentResources(Data.Dependencies);
        return Pending;
	}

    void Material::PrepareDependencies(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        for (auto& Pair : Data.mPassMap) Pair.second->PrepareDependencies();
    }
    bool Material::InitializeGPU(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        for (auto& Pair : Data.mPassMap)
        {
            auto* Pass = Pair.second;
            Pass->CompilePipeline();
					for (const auto& Reflected : Pass->GetMaterialParameters())
					{
						uint64 ReflectedHash = Reflected.NameHash;
						std::string ParameterName = Reflected.Name;
						if (ParameterName.rfind("U_", 0) == 0)
						{
							ParameterName = ParameterName.substr(2);
							ReflectedHash = Name(ParameterName.c_str()).GetHashValue();
						}
						auto It = std::find_if(Data.mParameterLayout.begin(), Data.mParameterLayout.end(), [ReflectedHash](const MaterialParameterDesc& Item) { return Item.Hash == ReflectedHash; });
						if (It == Data.mParameterLayout.end())
						{
							MaterialParameterDesc Desc;
							Desc.Name = ParameterName;
							Desc.Hash = ReflectedHash;
							Data.mParameterLayout.push_back(std::move(Desc));
							It = std::prev(Data.mParameterLayout.end());
						}
						It->Offset = Reflected.Offset;
						It->Size = Reflected.Size;
						It->ArrayCount = Reflected.ArrayCount;
						It->Set = Reflected.Set;
						It->Binding = Reflected.Binding;
						It->Reflected = true;
						switch (Reflected.Type)
						{
						case RHIPipeline::MaterialParameterType::Scalar: It->Kind = MaterialParameterKind::Scalar; break;
						case RHIPipeline::MaterialParameterType::Vector: It->Kind = MaterialParameterKind::Vector; break;
						case RHIPipeline::MaterialParameterType::Integer: It->Kind = MaterialParameterKind::Integer; break;
						case RHIPipeline::MaterialParameterType::Boolean: It->Kind = MaterialParameterKind::Boolean; break;
						default: break;
						}
					}
        }
        return true;
    }

	void Material::UnloadImpl()
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		for (auto& [PassName, Pass] : mPassMap)
			if (Pass) ShaderPassManager::instance()->DestroyShaderPass(Pass);
		mPassMap.clear();
		mScalarDefaults.clear();
		mVectorDefaults.clear();
		mIntDefaults.clear();
		mBoolDefaults.clear();
		mParameterLayout.clear();
		for (auto& Texture : mTextures)
			Texture = TexturePtr();
	}

	void Material::SaveResourceImpl()
	{

	}



    ShaderPass* Material::GetShaderPass()
    {
        return mPassMap.empty() ? nullptr : mPassMap.begin()->second;
    }

	ShaderPass* Material::GetPass(const Name& InPassType)
	{ auto It = mPassMap.find(InPassType);
		return It == mPassMap.end() ? nullptr : It->second;
	}

	bool Material::IsRegisterPass(const Name& InPassType)
	{ return mPassMap.find(InPassType) != mPassMap.end();
	}

	void Material::GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const
	{
        std::lock_guard<std::mutex> Lock(mDataMutex);
		// 纹理依赖
		for (int i = 0; i < Tex_Count; ++i)
		{
			if (!mTextures[i].isNull())
			{
				OutDependencies.push_back(mTextures[i]);
			}
		}

		// ShaderPass 中的 Shader 依赖
		for (const auto& [PassName, Pass] : mPassMap)
		{
			if (Pass)
			{
				Pass->GetDependentResources(OutDependencies);
			}
		}
	}
    void Material::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        std::swap(mVersion, Data.mVersion);
        for (size_t i = 0; i < Tex_Count; ++i) mTextures[i].swap(Data.mTextures[i]);
        std::swap(mPassMap, Data.mPassMap);
        std::swap(mScalarDefaults, Data.mScalarDefaults);
        std::swap(mVectorDefaults, Data.mVectorDefaults);
        std::swap(mIntDefaults, Data.mIntDefaults);
        std::swap(mBoolDefaults, Data.mBoolDefaults);
        std::swap(mParameterLayout, Data.mParameterLayout);
        ++mLayoutRevision;
    }

}
