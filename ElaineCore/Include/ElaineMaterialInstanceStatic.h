#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterial.h"

namespace Elaine
{
    class ElaineCoreExport MaterialInstanceStatic : public MaterialInterface, public ResourceBase
    {
    public:
        MaterialInstanceStatic(ResourceManager* InManager, const std::string& InResourceName);
        virtual ~MaterialInstanceStatic();
        ShaderPass* GetShaderPass() override
        {
            if (mOriginMaterialResource.isNull())
                return nullptr;
            if (!mOriginMaterialResource->IsLoaded())
                return nullptr;
            return mOriginMaterialResource->GetShaderPass();
        }
        ShaderPass* GetPass(const Name& InPassType)
        {
            if (mOriginMaterialResource.isNull())
                return nullptr;
            if (!mOriginMaterialResource->IsLoaded())
                return nullptr;
            return mOriginMaterialResource->GetPass(InPassType);
        }
        const MaterialPtr& GetOriginMaterial() const { return mOriginMaterialResource; }
        TexturePtr GetTexture(TextureSemantics InSemantics) const;

    protected:
        virtual bool LoadImpl() override;
        virtual	void UnloadImpl() override;
        virtual void SaveResourceImpl() override;
        virtual void ResourceArrivedImpl() override;

    public:
        // ResourceBase override: 获取所有直接依赖（原始材质 + 覆盖的纹理）
        void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const override;

    private:
        MaterialPtr mOriginMaterialResource;
        std::unordered_map<TextureSemantics, TexturePtr> mOverridedTextures;
    };

    using MaterialInstanceStaticPtr = ResourcePtr<MaterialInstanceStatic>;
}