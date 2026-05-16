
#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRenderMaterialProxy.h"
#include "ElainePass.h"
#include "Resource/ElaineTextureResource.h"
#include "Resource/ElaineMaterial.h"

namespace Elaine
{
    RenderMaterialProxy::RenderMaterialProxy()
    {
        for (int i = 0; i < Tex_Count; ++i)
            mResolvedTextures[i] = nullptr;
    }

    RenderMaterialProxy::~RenderMaterialProxy()
    {
        // RHI 资源由 Texture/Material 资源管理器负责销毁，这里不 delete
    }

    void RenderMaterialProxy::UpdateFromSnapshot(const MaterialParamSnapshot& InSnapshot)
    {
        // 缓存源材质引用（ResourcePtr，引用计数安全）
        mCachedSource = InSnapshot.Source;

        // 复制参数
        mScalars = InSnapshot.OverridedScalars;
        mVectors = InSnapshot.OverridedVectors;

        // 解析纹理资源 -> RHI 句柄
        ResolveTextures(InSnapshot);

        // 检查就绪状态
        mReady = InSnapshot.IsFullyLoaded();
    }

    ShaderPass* RenderMaterialProxy::GetShaderPass() const
    {
        if (mCachedSource.isNull())
            return nullptr;
        if (!mCachedSource->IsLoaded())
            return nullptr;
        return mCachedSource->GetShaderPass();
    }

    ShaderPass* RenderMaterialProxy::GetPass(const Name& InPassType) const
    {
        if (mCachedSource.isNull())
            return nullptr;
        if (!mCachedSource->IsLoaded())
            return nullptr;
        return mCachedSource->GetPass(InPassType);
    }

    RHITexture* RenderMaterialProxy::GetResolvedTexture(TextureSemantics InSem) const
    {
        if (InSem < 0 || InSem >= Tex_Count)
            return nullptr;
        return mResolvedTextures[InSem];
    }

    float RenderMaterialProxy::GetScalar(uint64_t InParamHash, float InDefault) const
    {
        auto It = mScalars.find(InParamHash);
        if (It != mScalars.end())
            return It->second;
        return InDefault;
    }

    Vector4 RenderMaterialProxy::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
    {
        auto It = mVectors.find(InParamHash);
        if (It != mVectors.end())
            return It->second;
        return InDefault;
    }

    void RenderMaterialProxy::ResolveTextures(const MaterialParamSnapshot& InSnapshot)
    {
        // 清空之前的解析结果
        for (int i = 0; i < Tex_Count; ++i)
            mResolvedTextures[i] = nullptr;

        // 优先解析覆盖的纹理
        for (const auto& [Sem, TexRes] : InSnapshot.OverridedTextures)
        {
            if (!TexRes.isNull() && TexRes->IsLoaded())
            {
                mResolvedTextures[Sem] = TexRes->GetTextureRHI();
            }
        }

        // 对于未覆盖的纹理，从 MaterialInstanceStatic -> Material 的默认纹理中获取
        if (!InSnapshot.Source.isNull() && InSnapshot.Source->IsLoaded())
        {
            const MaterialPtr& OriginMat = InSnapshot.Source->GetOriginMaterial();
            if (!OriginMat.isNull() && OriginMat->IsLoaded())
            {
                for (int i = 0; i < Tex_Count; ++i)
                {
                    if (mResolvedTextures[i] != nullptr)
                        continue; // 已被覆盖

                    // 先检查 MaterialInstanceStatic 的覆盖
                    TexturePtr MISTexture = InSnapshot.Source->GetTexture(static_cast<TextureSemantics>(i));
                    if (!MISTexture.isNull() && MISTexture->IsLoaded())
                    {
                        mResolvedTextures[i] = MISTexture->GetTextureRHI();
                    }
                }
            }
        }
    }
}
