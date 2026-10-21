#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineUniformGPUManager.h"
#include "ElaineMaterialInstanceStatic.h"
#include <array>

namespace Elaine
{
    // Captured on GameThread. RenderThread reads only the owned values and RHI handles.
    struct ElaineCoreExport MaterialParamSnapshot
    {
        MaterialInstanceStaticPtr Source; // Identity and change subscriptions only.
        std::unordered_map<TextureSemantics, TexturePtr> OverridedTextures;
        std::unordered_map<uint64_t, float> OverridedScalars;
        std::unordered_map<uint64_t, Vector4> OverridedVectors;
        std::unordered_map<uint64_t, int32_t> OverridedInts;
        std::unordered_map<uint64_t, bool> OverridedBools;
        uint64_t Revision = 0;

        std::unordered_map<uint64_t, float> Scalars;
        std::unordered_map<uint64_t, Vector4> Vectors;
        std::unordered_map<uint64_t, int32_t> Ints;
        std::unordered_map<uint64_t, bool> Bools;
        std::vector<MaterialParameterDesc> ParameterLayout;
        std::vector<ShaderPass> Passes;
        std::array<TexturePtr, Tex_Count> TextureSources;
        std::array<RHITextureHandle, Tex_Count> ResolvedTextures;
        std::array<RHITextureHandle, Tex_Count> OverrideTextureHandles;
        bool MaterialReady = false;
        bool Ready = false;

        void CaptureResources(); // GameThread only.
        void ApplyOverrides(); // Operates on captured values only.
        bool IsValid() const { return bool(Source); }
        bool IsFullyLoaded() const { return Ready; }
    };
}
