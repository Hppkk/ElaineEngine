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
        std::unordered_map<std::string, TexturePtr> mTextures;
        std::unordered_map<Name, ShaderPass*> mPassMap;
        std::unordered_map<uint64_t, float> mScalarDefaults;
        std::unordered_map<uint64_t, Vector4> mVectorDefaults;
        std::unordered_map<uint64_t, Matrix4x4> mMatrixDefaults;
        std::unordered_map<uint64_t, int32_t> mIntDefaults;
        std::unordered_map<uint64_t, bool> mBoolDefaults;
        std::vector<MaterialParameterDesc> mParameterLayout, mPublicParameterLayout;
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

    Matrix4x4 Material::GetMatrix(uint64_t InParamHash, const Matrix4x4& InDefault) const
    {
        std::lock_guard<std::mutex> Lock(mDataMutex);
        auto It = mMatrixDefaults.find(InParamHash);
        return It == mMatrixDefaults.end() ? InDefault : It->second;
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
	std::vector<MaterialParameterDesc> Material::GetParameterLayoutSnapshot() const { std::lock_guard<std::mutex> Lock(mDataMutex); return mPublicParameterLayout; }

	Material::~Material()
	{
        for (auto& Pair : mPassMap) delete Pair.second;
	}

	void Material::SetTexture(const Name& InName, TexturePtr InTexture)
	{
		std::lock_guard<std::mutex> Lock(mDataMutex);
		mTextures[SemanticsRegister::CanonicalTextureName(InName.ToString())] = InTexture;
		mbDirty = true;
        ++mContentRevision;
	}

    void Material::SetReadOnlyBuffer(const Name& InName, const RHIBufferView& View)
    {
        std::lock_guard<std::mutex> Lock(mDataMutex);
        mReadOnlyBuffers[InName.ToString()] = View;
        ++mContentRevision;
    }

	TexturePtr Material::GetTexture(const Name& InName) const
	{ std::lock_guard<std::mutex> Lock(mDataMutex);
		auto It = mTextures.find(SemanticsRegister::CanonicalTextureName(InName.ToString()));
        return It == mTextures.end() ? TexturePtr() : It->second;
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
		Data.mTextures.clear();

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
                else if (Type == "buffer") Desc.Kind = MaterialParameterKind::Buffer;
                else if (Type == "matrix") Desc.Kind = MaterialParameterKind::Matrix;
				if (Value.contains("min") && Value.contains("max")) { Desc.Min = Value["min"].get<float>(); Desc.Max = Value["max"].get<float>(); Desc.HasRange = true; }
				if (Type == "scalar" && Default.is_number())
					Data.mScalarDefaults[Hash] = Default.get<float>();
				else if ((Type == "vector" || Type == "color") && Default.is_array())
					Data.mVectorDefaults[Hash] = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f,
						Default.size() > 1 ? Default[1].get<float>() : 0.0f,
						Default.size() > 2 ? Default[2].get<float>() : 0.0f,
						Default.size() > 3 ? Default[3].get<float>() : 0.0f);
                else if (Type == "matrix" && !Default.is_null())
                {
                    if (!Default.is_array() || Default.size() != 16) throw std::runtime_error("Expected 16 matrix components: " + Desc.Name);
                    for (size_t i = 0; i < 16; ++i) Desc.DefaultMatrix[i / 4][i % 4] = Default[i].get<float>();
                    Data.mMatrixDefaults[Hash] = Desc.DefaultMatrix;
                }
				else if (Type == "int" && Default.is_number_integer())
					Data.mIntDefaults[Hash] = Default.get<int32_t>();
				else if (Type == "bool" && Default.is_boolean())
					Data.mBoolDefaults[Hash] = Default.get<bool>();
				if (Default.is_number()) Desc.DefaultValue = Vector4(Default.get<float>(), 0, 0, 0);
				else if (Default.is_array()) Desc.DefaultValue = Vector4(Default.size() > 0 ? Default[0].get<float>() : 0.0f, Default.size() > 1 ? Default[1].get<float>() : 0.0f, Default.size() > 2 ? Default[2].get<float>() : 0.0f, Default.size() > 3 ? Default[3].get<float>() : 0.0f);
				if (Type == "texture")
                {
                    Desc.Kind = MaterialParameterKind::Texture;
                    const auto Fallback = Default.is_string() ? Default.get<std::string>() : std::string();
                    if (Fallback == "white") Desc.DefaultTexture = RHIDefaultTexture::White;
                    else if (Fallback == "black") Desc.DefaultTexture = RHIDefaultTexture::Black;
                    else if (Fallback == "flat_normal") Desc.DefaultTexture = RHIDefaultTexture::FlatNormal;
                    else if (!Fallback.empty()) throw std::runtime_error("Unknown default texture: " + Fallback);
                }
                Desc.HasDefault = !Default.is_null();
				Data.mParameterLayout.push_back(std::move(Desc));
			}
		}

        const auto Derived = JsonMaterial.find("derivedParameters");
        if (Derived != JsonMaterial.end())
        {
            if (!Derived->is_object()) throw std::runtime_error("derivedParameters must be an object");
            for (const auto& Entry : Derived->items())
            {
                const auto& Rule = Entry.value();
                if (!Rule.is_object() || Rule.size() != 1 || !Rule.contains("texturePresent") || !Rule["texturePresent"].is_string())
                    throw std::runtime_error("Unsupported derived material parameter: " + Entry.key());
                if (std::any_of(Data.mParameterLayout.begin(), Data.mParameterLayout.end(), [&](const MaterialParameterDesc& P) { return P.Name == Entry.key(); }))
                    throw std::runtime_error("Derived parameter cannot also be a public parameter: " + Entry.key());
                MaterialParameterDesc Desc; Desc.Name = Entry.key(); Desc.Hash = Name(Desc.Name.c_str()).GetHashValue();
                Desc.Kind = MaterialParameterKind::Integer;
                Desc.TexturePresence = SemanticsRegister::CanonicalTextureName(Rule["texturePresent"].get<std::string>());
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
				
				const auto TS = SemanticsRegister::CanonicalTextureName(tex.key());
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

		for (const auto& Pair : Data.mTextures) if (Pair.second) Data.Dependencies.push_back(Pair.second);
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
            for (const auto& Resource : Pass->GetPipelineRHI()->GetResourceLayout())
            {
                if (Resource.Group != static_cast<uint32>(RHIResourceGroup::PerMaterial) || Resource.Type == RHIShaderResourceType::Uniform) continue;
                auto It = std::find_if(Data.mParameterLayout.begin(), Data.mParameterLayout.end(), [&](const MaterialParameterDesc& P) { return P.Name == Resource.Name; });
                if (It == Data.mParameterLayout.end())
                {
                    MaterialParameterDesc Desc; Desc.Name = Resource.Name; Desc.Hash = Name(Resource.Name.c_str()).GetHashValue();
                    Desc.Kind = Resource.Type == RHIShaderResourceType::Texture ? MaterialParameterKind::Texture : MaterialParameterKind::Buffer;
                    Data.mParameterLayout.push_back(std::move(Desc));
                }
                else if ((Resource.Type == RHIShaderResourceType::Texture && It->Kind != MaterialParameterKind::Texture) ||
                    (Resource.Type == RHIShaderResourceType::ReadOnlyBuffer && It->Kind != MaterialParameterKind::Buffer))
                    throw std::runtime_error("Material resource type disagrees with shader: " + Resource.Name);
            }
            for (const auto& Reflected : Pass->GetMaterialParameters())
            {
                MaterialParameterKind Kind = MaterialParameterKind::Unknown;
                switch (Reflected.Type)
                {
                case RHIPipeline::MaterialParameterType::Scalar: Kind = MaterialParameterKind::Scalar; break;
                case RHIPipeline::MaterialParameterType::Vector: Kind = MaterialParameterKind::Vector; break;
                case RHIPipeline::MaterialParameterType::Integer: Kind = MaterialParameterKind::Integer; break;
                case RHIPipeline::MaterialParameterType::Boolean: Kind = MaterialParameterKind::Boolean; break;
                case RHIPipeline::MaterialParameterType::Matrix: Kind = MaterialParameterKind::Matrix; break;
                default: throw std::runtime_error("Unsupported material parameter type: " + Reflected.Name);
                }
                auto It = std::find_if(Data.mParameterLayout.begin(), Data.mParameterLayout.end(), [&](const MaterialParameterDesc& P) { return P.Hash == Reflected.NameHash; });
                if (It == Data.mParameterLayout.end())
                {
                    MaterialParameterDesc Desc; Desc.Name = Reflected.Name; Desc.Hash = Reflected.NameHash; Desc.Kind = Kind;
                    Data.mParameterLayout.push_back(std::move(Desc));
                }
                else if (!It->TexturePresence.empty())
                {
                    if (Kind != MaterialParameterKind::Integer && Kind != MaterialParameterKind::Boolean)
                        throw std::runtime_error("Texture presence flags must be integer or boolean: " + Reflected.Name);
                }
                else if (It->Kind == MaterialParameterKind::Unknown) It->Kind = Kind;
                else if (It->Kind != Kind && !(It->Kind == MaterialParameterKind::Color && Kind == MaterialParameterKind::Vector) &&
                    !(It->Kind == MaterialParameterKind::Boolean && Kind == MaterialParameterKind::Integer))
                    throw std::runtime_error("Material parameter type disagrees with shader: " + Reflected.Name);
            }
        }
        for (const auto& Parameter : Data.mParameterLayout)
        {
            if (Parameter.TexturePresence.empty()) { Data.mPublicParameterLayout.push_back(Parameter); continue; }
            const auto Source = std::find_if(Data.mParameterLayout.begin(), Data.mParameterLayout.end(),
                [&](const MaterialParameterDesc& P) { return P.Name == Parameter.TexturePresence && P.Kind == MaterialParameterKind::Texture; });
            if (Source == Data.mParameterLayout.end()) throw std::runtime_error("Derived parameter texture is not declared: " + Parameter.Name + " -> " + Parameter.TexturePresence);
            bool Found = false;
            for (const auto& Pass : Data.mPassMap)
                for (const auto& P : Pass.second->GetMaterialParameters()) if (P.Name == Parameter.Name) Found = true;
            if (!Found) throw std::runtime_error("Derived parameter is not declared by a shader: " + Parameter.Name);
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
        mMatrixDefaults.clear();
		mIntDefaults.clear();
		mBoolDefaults.clear();
		mParameterLayout.clear(); mPublicParameterLayout.clear();
		mTextures.clear(); mReadOnlyBuffers.clear();
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
        for (const auto& Pair : mTextures) if (Pair.second) OutDependencies.push_back(Pair.second);

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
        bool LayoutChanged = mPassMap.size() != Data.mPassMap.size();
        for (const auto& Pair : Data.mPassMap)
        {
            auto It = mPassMap.find(Pair.first);
            if (It == mPassMap.end() || It->second->GetPipelineRHI()->GetResourceLayout() != Pair.second->GetPipelineRHI()->GetResourceLayout())
                LayoutChanged = true;
        }
        std::swap(mVersion, Data.mVersion);
        mTextures.swap(Data.mTextures);
        std::swap(mPassMap, Data.mPassMap);
        std::swap(mScalarDefaults, Data.mScalarDefaults);
        std::swap(mVectorDefaults, Data.mVectorDefaults);
        std::swap(mMatrixDefaults, Data.mMatrixDefaults);
        std::swap(mIntDefaults, Data.mIntDefaults);
        std::swap(mBoolDefaults, Data.mBoolDefaults);
        std::swap(mParameterLayout, Data.mParameterLayout);
        std::swap(mPublicParameterLayout, Data.mPublicParameterLayout);
        if (LayoutChanged) ++mLayoutRevision;
        ++mContentRevision;
    }

}
