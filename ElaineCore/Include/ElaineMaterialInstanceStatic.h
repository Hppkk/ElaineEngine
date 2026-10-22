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
		uint64_t GetLayoutRevision() const { return mOriginMaterialResource ? mOriginMaterialResource->GetLayoutRevision() : 0; }
        uint64_t GetContentRevision() const { return mDataRevision + (mOriginMaterialResource ? mOriginMaterialResource->GetContentRevision() : 0); }
        TexturePtr GetTexture(TextureSemantics InSemantics) const { return GetTexture(SemanticsRegister::GetName(InSemantics)); }
        TexturePtr GetTexture(const Name& InName) const;
        float GetScalar(uint64_t InParamHash, float InDefault = 0.0f) const;
        Vector4 GetVector(uint64_t InParamHash, const Vector4& InDefault = Vector4::ZERO) const;
        Matrix4x4 GetMatrix(uint64_t InParamHash, const Matrix4x4& InDefault = Matrix4x4::IDENTITY) const;
        int32_t GetInt(uint64_t InParamHash, int32_t InDefault = 0) const;
        bool GetBool(uint64_t InParamHash, bool InDefault = false) const;
        bool HasScalar(uint64_t InParamHash) const;
        bool HasVector(uint64_t InParamHash) const;
        const std::vector<MaterialParameterDesc>& GetParameterLayout() const
        {
            static const std::vector<MaterialParameterDesc> Empty;
            return mOriginMaterialResource.isNull() ? Empty : mOriginMaterialResource->GetParameterLayout();
        }

    protected:
        std::unique_ptr<ResourceLoadData> LoadImpl() override;
        virtual	void UnloadImpl() override;
        virtual void SaveResourceImpl() override;

    public:
        // ResourceBase override: 获取所有直接依赖（原始材质 + 覆盖的纹理）
        void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const override;

    private:
        friend struct MaterialParamSnapshot;
        uint64_t mDataRevision = 0;
        struct LoadData;
        void ApplyLoadedData(ResourceLoadData& Data) noexcept override;
        MaterialPtr mOriginMaterialResource;
        std::unordered_map<std::string, TexturePtr> mOverridedTextures;
        std::unordered_map<uint64_t, float> mOverridedScalars;
        std::unordered_map<uint64_t, Vector4> mOverridedVectors;
        std::unordered_map<uint64_t, Matrix4x4> mOverridedMatrices;
        std::unordered_map<uint64_t, int32_t> mOverridedInts;
        std::unordered_map<uint64_t, bool> mOverridedBools;
    };

    using MaterialInstanceStaticPtr = ResourcePtr<MaterialInstanceStatic>;
}
