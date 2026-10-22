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
     * - 只持有资源引用和参数，不访问平台 GPU API
     * - 参数修改同步通知逻辑线程订阅者，由组件捕获快照并提交给渲染线程
     * - 渲染线程使用 RenderMaterialProxy 接收快照并解析 RHI 资源
     *
     * 使用模式：
     *   // 逻辑线程
     *   mMaterial->ChangeMaterial("material_instance/PBR.mi");
     *   mMaterial->SetTexture(BaseColor, albedoTex);
     *
     *   // Mesh / Sky 组件自动提交新快照，业务不需要手动标记渲染状态。
     *   mMaterial->SetScalar(Name("Roughness"), 0.5f);
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
        void SetTexture(TextureSemantics InSemantics, const TexturePtr& InTexture) { SetTexture(SemanticsRegister::GetName(InSemantics), InTexture); }
        void SetTexture(const Name& InName, const TexturePtr& InTexture);
        void SetReadOnlyBuffer(const Name& InName, const RHIBufferView& View);
        TexturePtr GetTexture(TextureSemantics InSemantics) const { return GetTexture(SemanticsRegister::GetName(InSemantics)); }
        TexturePtr GetTexture(const Name& InName) const;
        bool HasTextureOverride(TextureSemantics InSemantics) const;

        // ============ 标量参数覆盖（逻辑线程调用） ============
        void SetScalar(const Name& InParamName, float InValue);
        float GetScalar(const Name& InParamName, float InDefault = 0.0f) const;
        bool HasScalarOverride(const Name& InParamName) const;

        // ============ 向量参数覆盖（逻辑线程调用） ============
        void SetVector(const Name& InParamName, const Vector4& InValue);
        Vector4 GetVector(const Name& InParamName, const Vector4& InDefault = Vector4::ZERO) const;
        bool HasVectorOverride(const Name& InParamName) const;
        void SetMatrix(const Name& InParamName, const Matrix4x4& InValue);
        Matrix4x4 GetMatrix(const Name& InParamName, const Matrix4x4& InDefault = Matrix4x4::IDENTITY) const;
        void SetInt(const Name& InParamName, int32_t InValue);
        int32_t GetInt(const Name& InParamName, int32_t InDefault = 0) const;
        bool HasIntOverride(const Name& InParamName) const;
        void SetBool(const Name& InParamName, bool InValue);
        bool GetBool(const Name& InParamName, bool InDefault = false) const;
        bool HasBoolOverride(const Name& InParamName) const;

        // ============ 快照生成（逻辑线程调用，用于跨线程传递） ============
        
        /**
         * 创建材质参数快照。
         * 快照包含资源引用、参数副本和 RHI 强句柄，
         * 不借用可变资源内部数据。
         * 通过 ENQUEUE_RENDER_COMMAND 传递给渲染线程的 RenderMaterialProxy。
         */
        MaterialParamSnapshot CreateSnapshot() const override;

        // 仅逻辑线程：修改完成后同步通知；注册时不回调，销毁订阅即可解除。
        ResourceSubscription ObserveParameterChanges(std::function<void()> Callback);

        // ============ 底层资源访问（逻辑线程） ============
        MaterialInstanceStaticPtr GetSourceMaterial() const { return mSource; }
        const std::vector<MaterialParameterDesc>& GetParameterLayout() const
        {
            static const std::vector<MaterialParameterDesc> Empty;
            return mSource.isNull() ? Empty : mSource->GetParameterLayout();
        }

    private:
        void NotifyParameterChanges();
        struct ParameterListener
        {
            std::shared_ptr<std::atomic_bool> Active;
            std::function<void()> Callback;
        };
        std::vector<ParameterListener> mParameterListeners;
        MaterialInstanceStaticPtr mSource;
        std::string mPath;
        std::unordered_map<std::string, TexturePtr> mOverridedTextures;
        std::unordered_map<std::string, RHIBufferView> mReadOnlyBuffers;
        std::unordered_map<uint64_t, float> mOverridedScalars;
        std::unordered_map<uint64_t, Vector4> mOverridedVectors;
        std::unordered_map<uint64_t, Matrix4x4> mOverridedMatrices;
        std::unordered_map<uint64_t, int32_t> mOverridedInts;
        std::unordered_map<uint64_t, bool> mOverridedBools;
        uint64_t mRevision = 0;
    };
}
