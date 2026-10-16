
#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRenderMaterialProxy.h"
#include "ElainePass.h"
#include "Resource/ElaineTextureResource.h"
#include "Resource/ElaineMaterial.h"
#include "ElaineRenderContext.h"
#include <cstring>

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
        mInts = InSnapshot.OverridedInts;
        mBools = InSnapshot.OverridedBools;
        // 解析纹理资源 -> RHI 句柄
        ResolveTextures(InSnapshot);
        BuildUniformData();

        // 检查就绪状态
        mReady = InSnapshot.IsFullyLoaded();
    }

    void RenderMaterialProxy::UpdateFromSnapshot(const MaterialParamSnapshot& InSnapshot, RenderContext& InContext)
    {
        UpdateFromSnapshot(InSnapshot);
        if (InContext.mCommandCtxRHI == nullptr)
            return;

        if (mMaterialUniformBuffer == nullptr || mAllocatedUniformSize != mUniformDataSize)
        {
            mMaterialUniformBuffer = InContext.mCommandCtxRHI->RHICreateUniformBuffer(mUniformDataSize, mPackedUniformData.data());
            mAllocatedUniformSize = mUniformDataSize;
            if (mBoundResourceBinding != nullptr)
                mBoundResourceBinding->SetPSUniformBuffer(0, mMaterialUniformBuffer);
        }
        else if (mUploadedRevision != InSnapshot.Revision)
            InContext.mCommandCtxRHI->RHIUpdateUniformBuffer(mMaterialUniformBuffer, mPackedUniformData.data(), mUniformDataSize);

        if (mMaterialUniformBuffer != nullptr)
            mUploadedRevision = InSnapshot.Revision;
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
        if (!mCachedSource.isNull())
            return mCachedSource->GetScalar(InParamHash, InDefault);
        return InDefault;
    }

    Vector4 RenderMaterialProxy::GetVector(uint64_t InParamHash, const Vector4& InDefault) const
    {
        auto It = mVectors.find(InParamHash);
        if (It != mVectors.end())
            return It->second;
        if (!mCachedSource.isNull())
            return mCachedSource->GetVector(InParamHash, InDefault);
        return InDefault;
    }

    int32_t RenderMaterialProxy::GetInt(uint64_t InParamHash, int32_t InDefault) const
    {
        auto It = mInts.find(InParamHash);
        if (It != mInts.end()) return It->second;
        return mCachedSource.isNull() ? InDefault : mCachedSource->GetInt(InParamHash, InDefault);
    }

    bool RenderMaterialProxy::GetBool(uint64_t InParamHash, bool InDefault) const
    {
        auto It = mBools.find(InParamHash);
        if (It != mBools.end()) return It->second;
        return mCachedSource.isNull() ? InDefault : mCachedSource->GetBool(InParamHash, InDefault);
    }

    void RenderMaterialProxy::BuildUniformData()
    {
        const uint64_t BaseColorHash = Name("BaseColorFactor").GetHash();
        const uint64_t EmissiveHash = Name("EmissiveFactor").GetHash();
        const uint64_t RoughnessHash = Name("Roughness").GetHash();
        const uint64_t MetallicHash = Name("Metallic").GetHash();
        const uint64_t SmallGridHash = Name("SmallGridScale").GetHash();
        const uint64_t LargeGridHash = Name("LargeGridScale").GetHash();
        const uint64_t FadeNearHash = Name("FadeNear").GetHash();
        const uint64_t FadeFarHash = Name("FadeFar").GetHash();
        const uint64_t GridColorHash = Name("GridColor").GetHash();
        const uint64_t XAxisHash = Name("XAxisColor").GetHash();
        const uint64_t ZAxisHash = Name("ZAxisColor").GetHash();

        mUniformData.BaseColorFactor = GetVector(BaseColorHash, mUniformData.BaseColorFactor);
        mUniformData.EmissiveFactor = GetVector(EmissiveHash, mUniformData.EmissiveFactor);
        mUniformData.PBRParams.x = GetScalar(MetallicHash, mUniformData.PBRParams.x);
        mUniformData.PBRParams.y = GetScalar(RoughnessHash, mUniformData.PBRParams.y);
        mUniformData.GridParams.x = GetScalar(SmallGridHash, mUniformData.GridParams.x);
        mUniformData.GridParams.y = GetScalar(LargeGridHash, mUniformData.GridParams.y);
        mUniformData.GridParams.z = GetScalar(FadeNearHash, mUniformData.GridParams.z);
        mUniformData.GridParams.w = GetScalar(FadeFarHash, mUniformData.GridParams.w);
        mUniformData.GridColor = GetVector(GridColorHash, mUniformData.GridColor);
        mUniformData.XAxisColor = GetVector(XAxisHash, mUniformData.XAxisColor);
        mUniformData.ZAxisColor = GetVector(ZAxisHash, mUniformData.ZAxisColor);
        mUniformData.MaterialFlags.x = mResolvedTextures[Normal] != nullptr ? 1.0f : 0.0f;
        mUniformData.MaterialFlags.y = mResolvedTextures[Metallic] != nullptr ? 1.0f : 0.0f;
        mUniformData.MaterialFlags.z = mResolvedTextures[Specular] != nullptr ? 1.0f : 0.0f;
        mUniformData.MaterialFlags.w = mResolvedTextures[Ambient] != nullptr ? 1.0f : 0.0f;

        mUniformDataSize = sizeof(MaterialUniformData);
        const MaterialPtr OriginMaterial = mCachedSource.isNull() ? MaterialPtr() : mCachedSource->GetOriginMaterial();
        if (!OriginMaterial.isNull())
        {
            for (const auto& Parameter : OriginMaterial->GetParameterLayoutSnapshot())
                if (Parameter.Reflected)
                    mUniformDataSize = std::max(mUniformDataSize, static_cast<size_t>(Parameter.Offset + Parameter.Size));
        }
        mPackedUniformData.assign(mUniformDataSize, 0);
        std::memcpy(mPackedUniformData.data(), &mUniformData, std::min(mUniformDataSize, sizeof(MaterialUniformData)));
        if (!OriginMaterial.isNull())
        {
            for (const auto& Parameter : OriginMaterial->GetParameterLayoutSnapshot())
            {
                if (!Parameter.Reflected || !Parameter.HasDefault || Parameter.Offset + Parameter.Size > mPackedUniformData.size()) continue;
                if (Parameter.Kind == MaterialParameterKind::Scalar)
                {
                    const float Value = GetScalar(Parameter.Hash, Parameter.DefaultValue.x);
                    std::memcpy(mPackedUniformData.data() + Parameter.Offset, &Value, std::min<size_t>(sizeof(Value), Parameter.Size));
                }
                else if (Parameter.Kind == MaterialParameterKind::Vector || Parameter.Kind == MaterialParameterKind::Color)
                {
                    const Vector4 Value = GetVector(Parameter.Hash, Parameter.DefaultValue);
                    std::memcpy(mPackedUniformData.data() + Parameter.Offset, Value.ptr(), std::min<size_t>(sizeof(Value), Parameter.Size));
                }
                else if (Parameter.Kind == MaterialParameterKind::Integer)
                {
                    const int32_t Value = GetInt(Parameter.Hash, static_cast<int32_t>(Parameter.DefaultValue.x));
                    std::memcpy(mPackedUniformData.data() + Parameter.Offset, &Value, std::min<size_t>(sizeof(Value), Parameter.Size));
                }
                else if (Parameter.Kind == MaterialParameterKind::Boolean)
                {
                    const int32_t Value = GetBool(Parameter.Hash, Parameter.DefaultValue.x != 0.0f) ? 1 : 0;
                    std::memcpy(mPackedUniformData.data() + Parameter.Offset, &Value, std::min<size_t>(sizeof(Value), Parameter.Size));
                }
            }
        }
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
