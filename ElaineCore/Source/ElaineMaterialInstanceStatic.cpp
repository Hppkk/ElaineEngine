#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInstanceStatic.h"
#include "ElaineDataStream.h"
#include "ElaineMaterialManager.h"
#include "ElaineTextureManager.h"
#include "Resource/ElaineResourceManager.h"  // RT_Material, RT_Texture

namespace Elaine
{
	MaterialInstanceStatic::MaterialInstanceStatic(ResourceManager* InManager, const std::string& InResourceName)
		: ResourceBase(InManager, InResourceName)
	{
		mMaterialType = MaterialStatic;
	}

	MaterialInstanceStatic::~MaterialInstanceStatic()
	{

	}

	TexturePtr MaterialInstanceStatic::GetTexture(TextureSemantics InSemantics) const
	{
		auto it = mOverridedTextures.find(InSemantics);
		if (it != mOverridedTextures.end())
			return it->second;

		return mOriginMaterialResource.isNull() ? TexturePtr() : mOriginMaterialResource->GetTexture(InSemantics);
	}

	bool MaterialInstanceStatic::LoadImpl()
	{
        DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
        Stream.ReadAll();
        JsonCpp JsonMaterial = JsonCpp::parse(Stream.GetDataStream());
		if (!JsonMaterial.contains("version") || JsonMaterial["version"] != "2.0.0")
		{
			LOG_ERROR("Material instance '%s' requires format version 2.0.0.", mResourceName.c_str());
			return false;
		}
		mOverridedTextures.clear();
		mOverridedScalars.clear();
		mOverridedVectors.clear();
		mOverridedInts.clear();
		mOverridedBools.clear();

        //ClearDependencies();

        // origin material
        std::string OriginPath = JsonMaterial.value("origin", "");
        if (!OriginPath.empty())
        {
            mOriginMaterialResource = MaterialManager::instance()->GetResource<Material>(OriginPath);
            //AddResourceEvent(ResourceEvent(OriginPath.c_str(), RT_Material, mOriginMaterialResource->GetLoadTask()));
        }

        // textures
        const auto& texJson = JsonMaterial["textures"];
        if (!texJson.empty())
        {
            for (auto& it : texJson.items())
            {
                std::string texPath = it.value();
                
                TextureSemantics TexSem = SemanticsRegister::GetSemantics(it.key().c_str());
                mOverridedTextures[TexSem] = TextureManager::instance()->GetResource<Texture>(texPath);
                //AddResourceEvent(ResourceEvent(texPath.c_str(), RT_Texture, mOverridedTextures[TexSem]->GetLoadTask()));
            }
        }

		const auto& params = JsonMaterial["parameters"];
        if (params.is_object())
        {
            for (auto& it : params.items())
            {
                const uint64_t Hash = Name(it.key().c_str()).GetHash();
                const auto& Value = it.value();
				const std::string Type = Value.is_object() ? Value.value("type", "") : (Value.is_number() ? "scalar" : "vector");
				const auto& RawValue = Value.is_object() && Value.contains("value") ? Value["value"] : Value;
				if (Type == "scalar" && RawValue.is_number())
					mOverridedScalars[Hash] = RawValue.get<float>();
				else if ((Type == "vector" || Type == "color") && RawValue.is_array())
                {
					mOverridedVectors[Hash] = Vector4(RawValue.size() > 0 ? RawValue[0].get<float>() : 0.0f,
						RawValue.size() > 1 ? RawValue[1].get<float>() : 0.0f,
						RawValue.size() > 2 ? RawValue[2].get<float>() : 0.0f,
						RawValue.size() > 3 ? RawValue[3].get<float>() : 0.0f);
				}
				else if (Type == "int" && RawValue.is_number_integer())
					mOverridedInts[Hash] = RawValue.get<int32_t>();
				else if (Type == "bool" && RawValue.is_boolean())
					mOverridedBools[Hash] = RawValue.get<bool>();
            }
        }

		return true;
	}

	float MaterialInstanceStatic::GetScalar(uint64_t InParamHash, float InDefault) const
	{
		auto It = mOverridedScalars.find(InParamHash);
		if (It != mOverridedScalars.end()) return It->second;
		return mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetScalar(InParamHash, InDefault);
	}

	Vector4 MaterialInstanceStatic::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
	{
		auto It = mOverridedVectors.find(InParamHash);
		if (It != mOverridedVectors.end()) return It->second;
		return mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetVector(InParamHash, InDefault);
	}

	int32_t MaterialInstanceStatic::GetInt(uint64_t InParamHash, int32_t InDefault) const
	{
		auto It = mOverridedInts.find(InParamHash);
		return It != mOverridedInts.end() ? It->second : (mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetInt(InParamHash, InDefault));
	}

	bool MaterialInstanceStatic::GetBool(uint64_t InParamHash, bool InDefault) const
	{
		auto It = mOverridedBools.find(InParamHash);
		return It != mOverridedBools.end() ? It->second : (mOriginMaterialResource.isNull() ? InDefault : mOriginMaterialResource->GetBool(InParamHash, InDefault));
	}

	bool MaterialInstanceStatic::HasScalar(uint64_t InParamHash) const { return mOverridedScalars.find(InParamHash) != mOverridedScalars.end() || (!mOriginMaterialResource.isNull() && mOriginMaterialResource->HasScalar(InParamHash)); }
	bool MaterialInstanceStatic::HasVector(uint64_t InParamHash) const { return mOverridedVectors.find(InParamHash) != mOverridedVectors.end() || (!mOriginMaterialResource.isNull() && mOriginMaterialResource->HasVector(InParamHash)); }

	void MaterialInstanceStatic::UnloadImpl()
	{
		mOverridedTextures.clear();
		mOverridedScalars.clear();
		mOverridedVectors.clear();
		mOverridedInts.clear();
		mOverridedBools.clear();
	}

	void MaterialInstanceStatic::SaveResourceImpl()
	{

	}

	void MaterialInstanceStatic::ResourceArrivedImpl()
	{

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
}
