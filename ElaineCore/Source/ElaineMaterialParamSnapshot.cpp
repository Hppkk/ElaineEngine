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
        Scalars.clear(); Vectors.clear(); Ints.clear(); Bools.clear(); Passes.clear(); ParameterLayout.clear();
        TextureSources = {}; ResolvedTextures = {}; OverrideTextureHandles = {};
        if (!Source || !Source->IsReady()) return;
        auto Origin = Source->mOriginMaterialResource;
        if (!Origin || !Origin->IsReady()) return;
        Scalars = Origin->mScalarDefaults; Vectors = Origin->mVectorDefaults;
        Ints = Origin->mIntDefaults; Bools = Origin->mBoolDefaults;
        for (const auto& Pair : Source->mOverridedScalars) Scalars[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedVectors) Vectors[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedInts) Ints[Pair.first] = Pair.second;
        for (const auto& Pair : Source->mOverridedBools) Bools[Pair.first] = Pair.second;
        ParameterLayout = Origin->mParameterLayout;
        for (const auto& Pair : Origin->mPassMap) Passes.push_back(Pair.second->CreateSnapshot());
        MaterialReady = true;
        for (size_t i = 0; i < Tex_Count; ++i)
        {
            auto Texture = Source->GetTexture(static_cast<TextureSemantics>(i));
            TextureSources[i] = Texture;
            if (Texture)
            {
                if (!Texture->IsReady()) Ready = false;
                else ResolvedTextures[i] = Texture->GetTextureRHI();
            }
        }
        for (const auto& Pair : OverridedTextures)
            if (Pair.second && Pair.first >= 0 && Pair.first < Tex_Count && Pair.second->IsReady())
                OverrideTextureHandles[Pair.first] = Pair.second->GetTextureRHI();
        ApplyOverrides();
    }

    void MaterialParamSnapshot::ApplyOverrides()
    {
        Ready = MaterialReady;
        for (const auto& Pair : OverridedScalars) Scalars[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedVectors) Vectors[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedInts) Ints[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedBools) Bools[Pair.first] = Pair.second;
        for (const auto& Pair : OverridedTextures)
            if (Pair.second && Pair.first >= 0 && Pair.first < Tex_Count)
            {
                TextureSources[Pair.first] = Pair.second;
                ResolvedTextures[Pair.first] = OverrideTextureHandles[Pair.first];
                if (!ResolvedTextures[Pair.first]) Ready = false;
            }
        for (size_t i = 0; i < Tex_Count; ++i)
            if (TextureSources[i] && !ResolvedTextures[i]) Ready = false;
    }
}
