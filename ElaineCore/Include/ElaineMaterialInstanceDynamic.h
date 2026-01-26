#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterial.h"
#include "ElaineMaterialInstanceStaticManager.h"
#include "ElaineUniformGPUManager.h"

namespace Elaine
{
    /**
     * MaterialInstanceDynamic 线程模型说明：
     * - 创建和修改：仅在逻辑线程
     * - 读取：渲染线程通过 ENQUEUE_RENDER_COMMAND 接收指针后读取
     * - 同步机制：依赖 RenderCommandQueue 的双缓冲实现线程安全
     * - 注意：逻辑线程修改后需通过 RenderCommand 通知渲染线程
     */
    class ElaineCoreExport MaterialInstanceDynamic : public MaterialInterface
    {
    public:
        MaterialInstanceDynamic();
        virtual ~MaterialInstanceDynamic();

        // 材质资源管理（逻辑线程调用）
        void ChangeMaterial(const std::string& InPath);
        const std::string& GetMaterialPath() const { return mPath; }
        
        // 资源状态检查
        bool IsValid() const { return !mSource.isNull(); }
        bool IsReady() const;  // 是否完全加载完成可使用

        // ShaderPass获取（渲染线程调用，内部检查加载状态）
        ShaderPass* GetShaderPass() override;
        ShaderPass* GetPass(const Name& InPassType);

        // 纹理参数覆盖（逻辑线程调用）
        void SetTexture(TextureSemantics InSemantics, const TexturePtr& InTexture);
        TexturePtr GetTexture(TextureSemantics InSemantics) const;
        bool HasTextureOverride(TextureSemantics InSemantics) const;

        // 标量参数覆盖（逻辑线程调用）
        void SetScalar(const Name& InParamName, float InValue);
        float GetScalar(const Name& InParamName, float InDefault = 0.0f) const;
        bool HasScalarOverride(const Name& InParamName) const;

        // 向量参数覆盖（逻辑线程调用）
        void SetVector(const Name& InParamName, const Vector4& InValue);
        Vector4 GetVector(const Name& InParamName, const Vector4& InDefault = Vector4::ZERO) const;
        bool HasVectorOverride(const Name& InParamName) const;

        // 获取底层材质资源
        MaterialInstanceStaticPtr GetSourceMaterial() const { return mSource; }

    private:
        MaterialInstanceStaticPtr mSource;
        std::string mPath;
        std::unordered_map<TextureSemantics, TexturePtr> mOverridedTextures;
        std::unordered_map<uint64_t, float> mOverridedScalars;
        std::unordered_map<uint64_t, Vector4> mOverridedVectors;
    };
}