#ifndef ELAINE_SKY_RESOURCES
#define ELAINE_SKY_RESOURCES
#include "Uniform_CommonVS.inl"
ELAINE_UNIFORM(PerMaterial, Uniform0, MaterialParams)
{
    float Exposure;
};
ELAINE_TEXTURE_CUBE(PerMaterial, Texture0, CubeMap)
#endif
