
#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterialParamSnapshot.h"

namespace Elaine
{
    const Name& MaterialParameterNames::SkyTexture() { static const Name Parameter("CubeMap"); return Parameter; }
    const Name& MaterialParameterNames::SkyExposure() { static const Name Parameter("Exposure"); return Parameter; }

    MaterialParamSnapshot MaterialInterface::CreateSnapshot() const
    {
        // 基类默认返回空快照
        return MaterialParamSnapshot();
    }
}
