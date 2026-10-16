
#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineMaterialParamSnapshot.h"
#include "math/ElaineVector4.h"
#include "render/common/ElaineRHIProtocol.h"
#include <vector>

namespace Elaine
{
    class ShaderPass;
    class RHITexture;
    class RHIUniformBuffer;
    class RenderContext;

    struct alignas(16) MaterialUniformData
    {
        Vector4 BaseColorFactor = Vector4(1, 1, 1, 1);
        Vector4 EmissiveFactor = Vector4(0, 0, 0, 0);
        Vector4 PBRParams = Vector4(0.0f, 0.5f, 1.0f, 0.0f);
        Vector4 MaterialFlags = Vector4(0, 0, 0, 0);
        Vector4 GridParams = Vector4(1.0f, 0.1f, 20.0f, 150.0f);
        Vector4 GridColor = Vector4(0.3f, 0.3f, 0.3f, 1.0f);
        Vector4 XAxisColor = Vector4(1.0f, 0.2f, 0.2f, 1.0f);
        Vector4 ZAxisColor = Vector4(0.0f, 0.3f, 1.0f, 1.0f);
    };

    /**
     * RenderMaterialProxy — 渲染线程材质代理
     *
     * 设计原则：
     * - 仅在渲染线程创建、更新、访问
     * - 持有已解析的 RHI 资源（RHITexture*, ShaderPass*/RHIPipeline*）
     * - 通过 UpdateFromSnapshot() 从逻辑线程的 MaterialParamSnapshot 同步数据
     * - 逻辑线程对象（MaterialInstanceDynamic 等）不会直接引用此类
     *
     * 典型用法：
     *   // 逻辑线程：
     *   MaterialParamSnapshot Snap = mMaterial->CreateSnapshot();
     *   ENQUEUE_RENDER_COMMAND(UpdateMat)([Proxy, Snap = std::move(Snap)](RenderContext&) {
     *       Proxy->GetMaterialProxy().UpdateFromSnapshot(Snap);
     *   });
     *
     *   // 渲染线程（UpdateRenderQueue / Render）：
     *   ShaderPass* Pass = mMaterialProxy.GetPass(Name("GBuffer"));
     *   RHITexture* Albedo = mMaterialProxy.GetResolvedTexture(BaseColor);
     */
    class ElaineCoreExport RenderMaterialProxy
    {
    public:
        RenderMaterialProxy();
        ~RenderMaterialProxy();

        // ============ 从快照更新（渲染线程调用） ============

        /**
         * 从 MaterialParamSnapshot 同步所有数据。
         * 解析 ResourcePtr -> RHI 资源指针并缓存。
         * 必须在渲染线程调用。
         */
        void UpdateFromSnapshot(const MaterialParamSnapshot& InSnapshot);
        void UpdateFromSnapshot(const MaterialParamSnapshot& InSnapshot, RenderContext& InContext);

        // ============ 渲染线程查询接口 ============

        /** 获取默认 ShaderPass（通常是第一个 pass） */
        ShaderPass* GetShaderPass() const;

        /** 按名称获取指定 ShaderPass */
        ShaderPass* GetPass(const Name& InPassType) const;

        /** 获取已解析的 RHI 纹理（从 Texture Resource 解析出的 GPU 句柄） */
        RHITexture* GetResolvedTexture(TextureSemantics InSem) const;

        /** 获取标量参数 */
        float GetScalar(uint64_t InParamHash, float InDefault = 0.0f) const;

        /** 获取向量参数 */
        Vector4 GetVector(uint64_t InParamHash, const Vector4& InDefault = Vector4::ZERO) const;
        int32_t GetInt(uint64_t InParamHash, int32_t InDefault = 0) const;
        bool GetBool(uint64_t InParamHash, bool InDefault = false) const;
        RHIUniformBuffer* GetMaterialUniformBuffer() const { return mMaterialUniformBuffer; }
        void BindToResourceBinding(RHI_DRAW_RESOURCE_BINDING& OutBinding) { mBoundResourceBinding = &OutBinding; OutBinding.SetPSUniformBuffer(0, mMaterialUniformBuffer); }

        /** 是否已准备就绪（资源链全部加载 + RHI 资源已解析） */
        bool IsReady() const { return mReady; }

        /** 是否持有有效的快照源 */
        bool IsValid() const { return !mCachedSource.isNull(); }

        /** 获取底层 MaterialInstanceStatic 资源（渲染线程安全：ResourcePtr 引用计数） */
        MaterialInstanceStaticPtr GetSourceMaterial() const { return mCachedSource; }

    private:
        /// 解析纹理资源为 RHI 句柄
        void ResolveTextures(const MaterialParamSnapshot& InSnapshot);
        void BuildUniformData();

    private:
        // ---------- 缓存的资源引用（ResourcePtr，引用计数安全） ----------
        MaterialInstanceStaticPtr mCachedSource;

        // ---------- 已解析的 RHI 资源（仅渲染线程访问） ----------
        RHITexture* mResolvedTextures[Tex_Count] = {};

        // ---------- 参数副本 ----------
        std::unordered_map<uint64_t, float> mScalars;
        std::unordered_map<uint64_t, Vector4> mVectors;
        std::unordered_map<uint64_t, int32_t> mInts;
        std::unordered_map<uint64_t, bool> mBools;
        MaterialUniformData mUniformData;
        std::vector<uint8_t> mPackedUniformData;
        size_t mUniformDataSize = sizeof(MaterialUniformData);
        size_t mAllocatedUniformSize = 0;
        RHI_DRAW_RESOURCE_BINDING* mBoundResourceBinding = nullptr;
        RHIUniformBuffer* mMaterialUniformBuffer = nullptr;
        uint64_t mUploadedRevision = ~uint64_t(0);

        // ---------- 状态 ----------
        bool mReady = false;
    };
}
