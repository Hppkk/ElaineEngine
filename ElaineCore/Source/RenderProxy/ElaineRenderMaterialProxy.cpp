
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
        auto It = mResolvedTextures.find(SemanticsRegister::CanonicalTextureName(SemanticsRegister::GetName(InSem).ToString()));
        return It == mResolvedTextures.end() ? nullptr : It->second.get();
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
        mParameters = {};
        for (const auto& Parameter : mSnapshot.ParameterLayout)
        {
            const auto& N = Parameter.Name;
            if (!Parameter.TexturePresence.empty())
            {
                const auto It = mResolvedTextures.find(Parameter.TexturePresence);
                mParameters.SetBool(N, It != mResolvedTextures.end() && bool(It->second));
                continue;
            }
            if (Parameter.Kind == MaterialParameterKind::Scalar) mParameters.SetFloat(N, GetScalar(Parameter.Hash, Parameter.DefaultValue.x));
            else if (Parameter.Kind == MaterialParameterKind::Vector || Parameter.Kind == MaterialParameterKind::Color)
            {
                auto V = GetVector(Parameter.Hash, Parameter.DefaultValue);
                mParameters.SetValue(N, V.ptr(), sizeof(float)*4, RHIShaderValueType::Float);
            }
            else if (Parameter.Kind == MaterialParameterKind::Integer) mParameters.SetInt(N, GetInt(Parameter.Hash, static_cast<int32>(Parameter.DefaultValue.x)));
            else if (Parameter.Kind == MaterialParameterKind::Boolean) mParameters.SetBool(N, GetBool(Parameter.Hash, Parameter.DefaultValue.x != 0));
            else if (Parameter.Kind == MaterialParameterKind::Matrix)
            {
                auto It = mSnapshot.Matrices.find(Parameter.Hash);
                const auto& M = It == mSnapshot.Matrices.end() ? Parameter.DefaultMatrix : It->second;
                mParameters.SetValue(N, M[0], sizeof(Matrix4x4), RHIShaderValueType::Matrix);
            }
            else if (Parameter.Kind == MaterialParameterKind::Texture && Parameter.DefaultTexture != RHIDefaultTexture::None) mParameters.TextureDefaults[N] = Parameter.DefaultTexture;
        }
        for (const auto& Pair : mResolvedTextures) mParameters.SetTexture(Pair.first, Pair.second.get());
        mParameters.Buffers = mSnapshot.Buffers;

    }

    void RenderMaterialProxy::BindToResourceBinding(RHI_DRAW_RESOURCE_BINDING& OutBinding)
    {
        OutBinding.GetParameters(RHIResourceGroup::PerMaterial) = mParameters;
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
            Snapshot.OverridedMatrices = mSnapshot.OverridedMatrices;
            Snapshot.OverridedInts = mSnapshot.OverridedInts;
            Snapshot.OverridedBools = mSnapshot.OverridedBools;
            Snapshot.OverridedTextures = mSnapshot.OverridedTextures;
            Snapshot.OverridedBuffers = mSnapshot.OverridedBuffers;
            Snapshot.OverrideTextureHandles = mSnapshot.OverrideTextureHandles;
            Snapshot.ApplyOverrides();
            Changed = true;
        }
        if (Update.IsTexture)
        {
            for (const auto& Pair : Snapshot.TextureSources)
                if (Pair.second.get() == Update.Resource.get())
                { Snapshot.ResolvedTextures[Pair.first] = Update.Texture; Changed = true; }
            for (const auto& Pair : Snapshot.OverridedTextures)
                if (Pair.second.get() == Update.Resource.get())
                { Snapshot.OverrideTextureHandles[Pair.first] = Update.Texture; Changed = true; }
            if (Changed) Snapshot.ApplyOverrides();
        }
        if (!Changed) return;
        RenderContext Context; Context.mCommandCtxRHI = GetDynamicRHI()->GetDefaultCommandContext();
        UpdateFromSnapshot(Snapshot, Context);
    }
    void RenderMaterialProxy::ResolveTextures(const MaterialParamSnapshot& InSnapshot)
    {
        mResolvedTextures = InSnapshot.ResolvedTextures;
    }
}
