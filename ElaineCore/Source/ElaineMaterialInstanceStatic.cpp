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

		return mOriginMaterialResource->GetTexture(InSemantics);
	}

	bool MaterialInstanceStatic::LoadImpl()
	{
        DataStream Stream(Root::instance()->GetResourcePath() + mResourceName);
        Stream.ReadAll();
        JsonCpp JsonMaterial = JsonCpp::parse(Stream.GetDataStream());

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

        // scalars
        //const auto& scalarJson = JsonMaterial["scalars"];
        //if (!scalarJson.empty())
        //{
        //    for (auto& it : scalarJson.items())
        //    {
        //        NameHash paramHash = Name(it.key().c_str()).GetHash();
        //        ScalarParams[paramHash] = it.value();
        //    }
        //}

		return true;
	}

	void MaterialInstanceStatic::UnloadImpl()
	{

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