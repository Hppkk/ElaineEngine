#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialParamSnapshot.h"
#include "ElaineThreadManager.h"

namespace Elaine
{
    void MaterialParamSnapshot::CaptureResources()
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread))
            throw std::logic_error("Material data must be captured on GameThread");
        MaterialReady = Ready = false;
        Scalars.clear(); Vectors.clear(); Matrices.clear(); Ints.clear(); Bools.clear(); Passes.clear(); ParameterLayout.clear();
        TextureSources = {}; ResolvedTextures = {}; OverrideTextureHandles = {}; Buffers.clear();
        if (!Source || !Source->IsReady()) return;
        auto Origin = Source->mOriginMaterialResource;
        if (!Origin || !Origin->IsReady()) return;
        Scalars = Origin->mScalarDefaults; Vectors = Origin->mVectorDefaults;
        Matrices = Origin->mMatrixDefaults;
        Ints = Origin->mIntDefaults; Bools = Origin->mBoolDefaults;
        for (const auto& Pair : Source->mOverridedScalars) Scalars[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedVectors) Vectors[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedMatrices) Matrices[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedInts) Ints[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedBools) Bools[Pair.first] = Pair.second;
        ParameterLayout = Origin->mParameterLayout;
        for (const auto& Pair : Origin->mPassMap) Passes.push_back(Pair.second->CreateSnapshot());
        MaterialReady = true;
        TextureSources = Origin->mTextures;
        for (const auto& Pair : Source->mOverridedTextures) TextureSources[Pair.first] = Pair.second;
        Buffers = Origin->mReadOnlyBuffers;
        for (const auto& Pair : OverridedBuffers) Buffers[Pair.first] = Pair.second;
        for (const auto& Pair : TextureSources)
            if (Pair.second && Pair.second->IsReady()) ResolvedTextures[Pair.first] = Pair.second->GetTextureRHI();
        for (const auto& Pair : OverridedTextures)
            if (Pair.second && Pair.second->IsReady()) OverrideTextureHandles[Pair.first] = Pair.second->GetTextureRHI();
        ApplyOverrides();
    }

    void MaterialParamSnapshot::ApplyOverrides()
    {
        Ready = MaterialReady;
        for (const auto& Pair : OverridedScalars) Scalars[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedVectors) Vectors[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedMatrices) Matrices[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedInts) Ints[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedBools) Bools[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedTextures)
        {
            TextureSources[Pair.first] = Pair.second;
            ResolvedTextures[Pair.first] = Pair.second ? OverrideTextureHandles[Pair.first] : RHITextureHandle{};
            if (Pair.second && !ResolvedTextures[Pair.first]) Ready = false;
        }
        for (const auto& Pair : TextureSources)
            if (Pair.second && !ResolvedTextures[Pair.first]) Ready = false;
        for (const auto& Pair : OverridedBuffers) Buffers[Pair.first] = Pair.second;
    }
}
