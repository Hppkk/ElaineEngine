#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterial.h"
#include "ElaineMaterialInstanceStaticManager.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
    /**
     * MaterialInstanceDynamic — 逻辑线程材质实例
     *
     * 线程模型说明：
     * - 创建、修改、销毁：仅在逻辑线程
     * - 不持有任何 RHI 资源（RHITexture*, RHIPipeline* 等）
     * - 通过 CreateSnapshot() 生成参数快照，经 ENQUEUE_RENDER_COMMAND 传递给渲染线程
     * - 渲染线程使用 RenderMaterialProxy 接收快照并解析 RHI 资源
     *
     * 使用模式：
     *   // 逻辑线程
     *   mMaterial->ChangeMaterial("material_instance/PBR.mi");
     *   mMaterial->SetTexture(BaseColor, albedoTex);
     *
     *   MaterialParamSnapshot Snap = mMaterial->CreateSnapshot();
     *   ENQUEUE_RENDER_COMMAND(UpdateMat)([Proxy, Snap = std::move(Snap)](RenderContext&) {
     *       Proxy->GetMaterialProxy().UpdateFromSnapshot(Snap);
     *   });
     */
    class ElaineCoreExport MaterialInstanceDynamic : public MaterialInterface
    {
    public:
        MaterialInstanceDynamic();
        virtual ~MaterialInstanceDynamic();

        // ============ 材质资源管理（逻辑线程调用） ============
        void ChangeMaterial(const std::string& InPath);
        const std::string& GetMaterialPath() const { return mPath; }
        
        // ============ 资源状态检查（逻辑线程调用） ============
        bool IsValid() const { return !mSource.isNull(); }
        bool IsReady() const;

        // ============ 纹理参数覆盖（逻辑线程调用） ============
        void SetTexture(TextureSemantics InSemantics, const TexturePtr& InTexture);
        TexturePtr GetTexture(TextureSemantics InSemantics) const;
        bool HasTextureOverride(TextureSemantics InSemantics) const;

        // ============ 标量参数覆盖（逻辑线程调用） ============
        void SetScalar(const Name& InParamName, float InValue);
        float GetScalar(const Name& InParamName, float InDefault = 0.0f) const;
        bool HasScalarOverride(const Name& InParamName) const;

        // ============ 向量参数覆盖（逻辑线程调用） ============
        void SetVector(const Name& InParamName, const Vector4& InValue);
        Vector4 GetVector(const Name& InParamName, const Vector4& InDefault = Vector4::ZERO) const;
        bool HasVectorOverride(const Name& InParamName) const;

        // ============ 快照生成（逻辑线程调用，用于跨线程传递） ============
        
        /**
         * 创建材质参数快照。
         * 快照仅包含 ResourcePtr（引用计数安全）和 POD 参数值，
         * 不包含任何 RHI 资源指针。
         * 通过 ENQUEUE_RENDER_COMMAND 传递给渲染线程的 RenderMaterialProxy。
         */
        MaterialParamSnapshot CreateSnapshot() const override;

        // ============ 底层资源访问（逻辑线程） ============
        MaterialInstanceStaticPtr GetSourceMaterial() const { return mSource; }

    private:
        MaterialInstanceStaticPtr mSource;
        std::string mPath;
        std::unordered_map<TextureSemantics, TexturePtr> mOverridedTextures;
        std::unordered_map<uint64_t, float> mOverridedScalars;
        std::unordered_map<uint64_t, Vector4> mOverridedVectors;
    };
}