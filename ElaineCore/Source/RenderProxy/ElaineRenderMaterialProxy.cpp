
#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRenderMaterialProxy.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "ElainePass.h"
#include "Resource/ElaineTextureResource.h"
#include "Resource/ElaineMaterial.h"
#include "ElaineRenderContext.h"
#include <cstring>
#include "render/common/ElaineRHICommandContext.h"

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
        // The render thread only reads its own captured data.
        mSnapshot = InSnapshot;
        mCachedSource = InSnapshot.Source;
        mPasses = InSnapshot.Passes;

        // 复制参数
        mScalars = InSnapshot.Scalars;
        mVectors = InSnapshot.Vectors;
        mInts = InSnapshot.Ints;
        mBools = InSnapshot.Bools;
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

        if (mMaterialUniformBuffer == nullptr || mAllocatedUniformSize != mUniformDataSize || mUploadedRevision != InSnapshot.Revision)
        {
            RHIUniformBufferDesc Desc; Desc.Size = mUniformDataSize; Desc.InitialData = mPackedUniformData.data(); Desc.Slot = RHIUniformSlot::PerMaterial;
            mMaterialUniformBuffer = GetDynamicRHI()->GetDefaultCommandContext()->RHICreateUniformBufferWithSlot(Desc);
            mAllocatedUniformSize = mUniformDataSize;

        }


        if (mMaterialUniformBuffer != nullptr)
            mUploadedRevision = InSnapshot.Revision;
    }

    ShaderPass* RenderMaterialProxy::GetShaderPass() const
    {
        return mReady && !mPasses.empty() ? &mPasses.front() : nullptr;
    }

    ShaderPass* RenderMaterialProxy::GetPass(const Name& InPassType) const
    {
        if (mReady) for (auto& Pass : mPasses) if (Pass.GetPassName() == InPassType) return &Pass;
        return nullptr;
    }

    RHITexture* RenderMaterialProxy::GetResolvedTexture(TextureSemantics InSem) const
    {
        if (InSem < 0 || InSem >= Tex_Count)
            return nullptr;
        return mResolvedTextures[InSem].get();
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

    int32_t RenderMaterialProxy::GetInt(uint64_t InParamHash, int32_t InDefault) const
    {
        auto It = mInts.find(InParamHash);
        if (It != mInts.end()) return It->second;
        return InDefault;
    }

    bool RenderMaterialProxy::GetBool(uint64_t InParamHash, bool InDefault) const
    {
        auto It = mBools.find(InParamHash);
        if (It != mBools.end()) return It->second;
        return InDefault;
    }

    void RenderMaterialProxy::BuildUniformData()
    {
        const uint64_t BaseColorHash = Name("BaseColorFactor").GetHashValue();
        const uint64_t EmissiveHash = Name("EmissiveFactor").GetHashValue();
        const uint64_t RoughnessHash = Name("Roughness").GetHashValue();
        const uint64_t MetallicHash = Name("Metallic").GetHashValue();
        const uint64_t SmallGridHash = Name("SmallGridScale").GetHashValue();
        const uint64_t LargeGridHash = Name("LargeGridScale").GetHashValue();
        const uint64_t FadeNearHash = Name("FadeNear").GetHashValue();
        const uint64_t FadeFarHash = Name("FadeFar").GetHashValue();
        const uint64_t GridColorHash = Name("GridColor").GetHashValue();
        const uint64_t XAxisHash = Name("XAxisColor").GetHashValue();
        const uint64_t ZAxisHash = Name("ZAxisColor").GetHashValue();

        mUniformData = MaterialUniformData{};
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
        if (!mSnapshot.ParameterLayout.empty())
        {
            for (const auto& Parameter : mSnapshot.ParameterLayout)
                if (Parameter.Reflected)
                    mUniformDataSize = std::max(mUniformDataSize, static_cast<size_t>(Parameter.Offset + Parameter.Size));
        }
        mPackedUniformData.assign(mUniformDataSize, 0);
        std::memcpy(mPackedUniformData.data(), &mUniformData, std::min(mUniformDataSize, sizeof(MaterialUniformData)));
        if (!mSnapshot.ParameterLayout.empty())
        {
            for (const auto& Parameter : mSnapshot.ParameterLayout)
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

    void RenderMaterialProxy::BindToResourceBinding(RHI_DRAW_RESOURCE_BINDING& OutBinding)
    {
        OutBinding.SetPSUniformBuffer(0, mMaterialUniformBuffer.get());
        for (int i = 0; i < Tex_Count; ++i) OutBinding.SetTexture(i, mResolvedTextures[i].get());
        OutBinding.ResourceLifetimes.clear();
    }
    void RenderMaterialProxy::ApplyResourceUpdate(const RenderResourceUpdate& Update)
    {
        auto Snapshot = mSnapshot;
        bool Changed = false;
        if (Update.IsMaterial && Snapshot.Source.get() == Update.Resource.get())
        {
            Snapshot = Update.Material;
            Snapshot.OverridedScalars = mSnapshot.OverridedScalars;
            Snapshot.OverridedVectors = mSnapshot.OverridedVectors;
            Snapshot.OverridedInts = mSnapshot.OverridedInts;
            Snapshot.OverridedBools = mSnapshot.OverridedBools;
            Snapshot.OverridedTextures = mSnapshot.OverridedTextures;
            Snapshot.OverrideTextureHandles = mSnapshot.OverrideTextureHandles;
            Snapshot.ApplyOverrides();
            Changed = true;
        }
        if (Update.IsTexture)
        {
            for (size_t i = 0; i < Tex_Count; ++i)
                if (Snapshot.TextureSources[i].get() == Update.Resource.get())
                { Snapshot.ResolvedTextures[i] = Update.Texture; Changed = true; }
            for (const auto& Pair : Snapshot.OverridedTextures)
                if (Pair.second.get() == Update.Resource.get())
                { Snapshot.OverrideTextureHandles[Pair.first] = Update.Texture; Changed = true; }
            if (Changed) Snapshot.ApplyOverrides();
        }
        if (!Changed) return;
        mUploadedRevision = ~Snapshot.Revision;
        RenderContext Context; Context.mCommandCtxRHI = GetDynamicRHI()->GetDefaultCommandContext();
        UpdateFromSnapshot(Snapshot, Context);
    }
    void RenderMaterialProxy::ResolveTextures(const MaterialParamSnapshot& InSnapshot)
    {
        for (size_t i = 0; i < Tex_Count; ++i) mResolvedTextures[i] = InSnapshot.ResolvedTextures[i];
    }
}
