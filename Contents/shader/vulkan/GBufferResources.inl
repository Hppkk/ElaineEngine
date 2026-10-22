#ifndef ELAINE_GBUFFER_RESOURCES
#define ELAINE_GBUFFER_RESOURCES
#include "Uniform_CommonVS.inl"
#include "Generated/ObjectParameters.inl"
ELAINE_UNIFORM(PerMaterial, Uniform0, MaterialParams)
{
    vec4 BaseColorFactor;
    vec4 EmissiveFactor;
    float Metallic;
    float Roughness;
    float Occlusion;
    float AlphaCutoff;
    int HasNormal;
    int HasMetallicRoughness;
    int HasEmissive;
    int HasAO;
};
ELAINE_TEXTURE2D(PerMaterial, Texture0, BaseColor)
ELAINE_TEXTURE2D(PerMaterial, Texture1, Normal)
ELAINE_TEXTURE2D(PerMaterial, Texture2, MetallicRoughness)
ELAINE_TEXTURE2D(PerMaterial, Texture3, Emissive)
ELAINE_TEXTURE2D(PerMaterial, Texture4, AO)
#endif
