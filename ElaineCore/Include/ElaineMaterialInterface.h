#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
    class ShaderPass;

    enum MaterialType
    {
        MaterialResource,
        MaterialStatic,
        MaterialDynamic
    };

    class ElaineCoreExport MaterialInterface
    {
    public:
        virtual ShaderPass* GetShaderPass() { return nullptr; }
        const MaterialType GetMaterialType() const { return mMaterialType; }
    protected:
        MaterialType mMaterialType = MaterialResource;
    };
}