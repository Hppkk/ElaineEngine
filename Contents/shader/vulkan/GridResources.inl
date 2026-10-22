#ifndef ELAINE_GRID_RESOURCES
#define ELAINE_GRID_RESOURCES
#include "Uniform_CommonVS.inl"
ELAINE_UNIFORM(PerMaterial, Uniform0, MaterialParams)
{
    float SmallGridScale;
    float LargeGridScale;
    float FadeNear;
    float FadeFar;
    vec4 GridColor;
    vec4 XAxisColor;
    vec4 ZAxisColor;
};
#endif
