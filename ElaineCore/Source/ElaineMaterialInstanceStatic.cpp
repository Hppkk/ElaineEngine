#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInstanceStatic.h"
#include "ElaineDataStream.h"
#include "ElaineMaterialManager.h"
#include "ElaineTextureManager.h"
#include "Resource/ElaineResourceManager.h"  // RT_Material, RT_Texture

namespace Elaine
{
    struct MaterialInstanceStatic::LoadData : ResourceLoadData
    {
        ResourceReference OriginReference;
        std::unordered_map<std::string, ResourceReference> TextureReferences;
        MaterialPtr mOriginMaterialResource;
        JsonCpp SavedJson;
        std::unordered_map<std::string, TexturePtr> mOverridedTextures;
        std::unordered_map<uint64_t, float> mOverridedScalars;
        std::unordered_map<uint64_t, Vector4> mOverridedVectors;
        std::unordered_map<uint64_t, Matrix4x4> mOverridedMatrices;
        std::unordered_map<uint64_t, int32_t> mOverridedInts;
        std::unordered_map<uint64_t, bool> mOverridedBools;
    };


	MaterialInstanceStatic::MaterialInstanceStatic(ResourceManager* InManager, const std::string& InResourceName)
		: ResourceBase(InManager, InResourceName)
	{
		mMaterialType = MaterialStatic;
	}

	MaterialInstanceStatic::~MaterialInstanceStatic()
	{

	}

	TexturePtr MaterialInstanceStatic::GetTexture(const Name& InName) const
	{ auto it = mOverridedTextures.find(SemanticsRegister::CanonicalTextureName(InName.ToString()));
		if (it != mOverridedTextures.end())
			return it->second;

		return mOriginMaterialResource.isNull() ? TexturePtr() : mOriginMaterialResource->GetTexture(InName);
	}

	std::unique_ptr<ResourceLoadData> MaterialInstanceStatic::LoadImpl()
    {
        auto Pending = std::make_unique<LoadData>();
        auto& Data = *Pending;
        JsonCpp JsonMaterial = AssetFile::ReadJson(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName, "MaterialInstance");
        Data.SavedJson = JsonMaterial;
		if (!JsonMaterial.contains("version") || JsonMaterial["version"] != "2.0.0")
		{
			LOG_ERROR("Material instance '%s' requires format version 2.0.0.", mResourceName.c_str());
			return {};
		}
		Data.mOverridedTextures.clear();
		Data.mOverridedScalars.clear();
		Data.mOverridedVectors.clear();
		Data.mOverridedInts.clear();
		Data.mOverridedBools.clear();

        //ClearDependencies();

        // origin material
        const auto Origin = ResourceReference::FromJson(JsonMaterial.at("origin"));
        Data.OriginReference = Origin;

        // textures
        const auto& texJson = JsonMaterial["textures"];
        if (!texJson.empty())
        {
            for (auto& it : texJson.items())
            {
                const auto texPath = ResourceReference::FromJson(it.value());
                
                const auto TexSem = SemanticsRegister::CanonicalTextureName(it.key());
                Data.TextureReferences[TexSem] = texPath;
            }
        }

		const auto& params = JsonMaterial["parameters"];
        if (params.is_object())
        {
            for (auto& it : params.items())
            {
                const uint64_t Hash = Name(it.key().c_str()).GetHashValue();
                const auto& Value = it.value();
				const std::string Type = Value.is_object() ? Value.value("type", "") : (Value.is_number() ? "scalar" : "vector");
				const auto& RawValue = Value.is_object() && Value.contains("value") ? Value["value"] : Value;
				if (Type == "scalar" && RawValue.is_number())
					Data.mOverridedScalars[Hash] = RawValue.get<float>();
				else if ((Type == "vector" || Type == "color") && RawValue.is_array())
                {
					Data.mOverridedVectors[Hash] = Vector4(RawValue.size() > 0 ? RawValue[0].get<float>() : 0.0f,
						RawValue.size() > 1 ? RawValue[1].get<float>() : 0.0f,
						RawValue.size() > 2 ? RawValue[2].get<float>() : 0.0f,
						RawValue.size() > 3 ? RawValue[3].get<float>() : 0.0f);
				}
                else if (Type == "matrix")
                {
                    if (!RawValue.is_array() || RawValue.size() != 16) throw std::runtime_error("Expected 16 matrix components: " + it.key());
                    Matrix4x4 M;
                    for (size_t i = 0; i < 16; ++i) M[i / 4][i % 4] = RawValue[i].get<float>();
                    Data.mOverridedMatrices[Hash] = M;
                }
				else if (Type == "int" && RawValue.is_number_integer())
					Data.mOverridedInts[Hash] = RawValue.get<int32_t>();
				else if (Type == "bool" && RawValue.is_boolean())
					Data.mOverridedBools[Hash] = RawValue.get<bool>();
            }
        }

        return Pending;
	}

    void MaterialInstanceStatic::PrepareDependencies(ResourceLoadData& InData)
    {
        auto& Data = static_cast<LoadData&>(InData);
        Data.mOriginMaterialResource = static_pointer_cast<Material>(Data.GetDependency(Data.OriginReference, "Material"));
        for (const auto& [Name, Reference] : Data.TextureReferences)
            Data.mOverridedTextures[Name] = static_pointer_cast<Texture>(Data.GetDependency(Reference, "Texture"));
    }

	float MaterialInstanceStatic::GetScalar(uint64_t InParamHash, float InDefault) const
	{ auto It = mOverridedScalars.find(InParamHash);
		if (It != mOverridedScalars.end()) return It->second;
		return mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetScalar(InParamHash, InDefault);
	}

	Vector4 MaterialInstanceStatic::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
	{ auto It = mOverridedVectors.find(InParamHash);
		if (It != mOverridedVectors.end()) return It->second;
		return mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetVector(InParamHash, InDefault);
	}

    Matrix4x4 MaterialInstanceStatic::GetMatrix(uint64_t InParamHash, const Matrix4x4& InDefault) const
    {
        auto It = mOverridedMatrices.find(InParamHash);
        return It != mOverridedMatrices.end() ? It->second : (mOriginMaterialResource ? mOriginMaterialResource->GetMatrix(InParamHash, InDefault) : InDefault);
    }
	int32_t MaterialInstanceStatic::GetInt(uint64_t InParamHash, int32_t InDefault) const
	{ auto It = mOverridedInts.find(InParamHash);
		return It != mOverridedInts.end() ? It->second : (mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetInt(InParamHash, InDefault));
	}

	bool MaterialInstanceStatic::GetBool(uint64_t InParamHash, bool InDefault) const
	{ auto It = mOverridedBools.find(InParamHash);
		return It != mOverridedBools.end() ? It->second : (mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetBool(InParamHash, InDefault));
	}

	bool MaterialInstanceStatic::HasScalar(uint64_t InParamHash) const { return mOverridedScalars.find(InParamHash) != mOverridedScalars.end() || (!mOriginMaterialResource.isNull() && mOriginMaterialResource->HasScalar(InParamHash)); }
	bool MaterialInstanceStatic::HasVector(uint64_t InParamHash) const { return mOverridedVectors.find(InParamHash) != mOverridedVectors.end() || (!mOriginMaterialResource.isNull() && mOriginMaterialResource->HasVector(InParamHash)); }



	void MaterialInstanceStatic::UnloadImpl()
	{
		mOriginMaterialResource = {};
        mOverridedTextures.clear();
		mOverridedScalars.clear();
		mOverridedVectors.clear();
        mOverridedMatrices.clear();
		mOverridedInts.clear();
		mOverridedBools.clear();
	}

    void MaterialInstanceStatic::SaveResourceImpl()
    {
        auto Json = mSavedJson;
        if (mOriginMaterialResource) Json["origin"] = mOriginMaterialResource->GetReference().ToJson();
        Json["textures"] = JsonCpp::object();
        for (const auto& [Key, Texture] : mOverridedTextures) if (Texture) Json["textures"][Key] = Texture->GetReference().ToJson();
        for (auto& Entry : Json["parameters"].items())
        {
            const auto Hash = Name(Entry.key().c_str()).GetHashValue(); auto& Node = Entry.value();
            auto Set = [&](const JsonCpp& Value) { if (Node.is_object()) Node["value"] = Value; else Node = Value; };
            if (auto It = mOverridedScalars.find(Hash); It != mOverridedScalars.end()) Set(It->second);
            if (auto It = mOverridedVectors.find(Hash); It != mOverridedVectors.end()) Set({It->second.x,It->second.y,It->second.z,It->second.w});
            if (auto It = mOverridedInts.find(Hash); It != mOverridedInts.end()) Set(It->second);
            if (auto It = mOverridedBools.find(Hash); It != mOverridedBools.end()) Set(It->second);
            if (auto It = mOverridedMatrices.find(Hash); It != mOverridedMatrices.end())
            { JsonCpp Value=JsonCpp::array(); for (size_t i=0;i<16;++i) Value.push_back(It->second[i/4][i%4]); Set(Value); }
        }
        SaveAssetJson(Json, "MaterialInstance"); mSavedJson = std::move(Json);
    }



	void MaterialInstanceStatic::GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const
	{
        // 原始材质依赖
		if (!mOriginMaterialResource.isNull())
		{
			OutDependencies.push_back(mOriginMaterialResource);
		}

		// 覆盖的纹理依赖
		for (const auto& [Semantics, Tex] : mOverridedTextures)
		{
			if (!Tex.isNull())
			{
				OutDependencies.push_back(Tex);
			}
		}
	}
    void MaterialInstanceStatic::ApplyLoadedData(ResourceLoadData& InData) noexcept
    {
        auto& Data = static_cast<LoadData&>(InData);
        mSavedJson.swap(Data.SavedJson);
        std::swap(mOriginMaterialResource, Data.mOriginMaterialResource);
        std::swap(mOverridedTextures, Data.mOverridedTextures);
        std::swap(mOverridedScalars, Data.mOverridedScalars);
        std::swap(mOverridedVectors, Data.mOverridedVectors);
        std::swap(mOverridedMatrices, Data.mOverridedMatrices);
        std::swap(mOverridedInts, Data.mOverridedInts);
        std::swap(mOverridedBools, Data.mOverridedBools);
        ++mDataRevision;
    }

}
