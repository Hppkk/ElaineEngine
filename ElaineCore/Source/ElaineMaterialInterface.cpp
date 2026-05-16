
#include "ElainePrecompiledHeader.h"
#include "ElaineMaterialInterface.h"
#include "ElaineMaterialParamSnapshot.h"

namespace Elaine
{
    MaterialParamSnapshot MaterialInterface::CreateSnapshot() const
    {
        // 基类默认返回空快照
        return MaterialParamSnapshot();
    }
}
