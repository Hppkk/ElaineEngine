#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceDependencyProvider.h"
#include "Resource/ElaineResourceBase.h"

namespace Elaine
{
    bool IResourceDependencyProvider::AreDependenciesReady() const
    {
        std::vector<ResourceBasePtr> Deps;
        GetDependentResources(Deps);
        for (const auto& Dep : Deps)
        {
            if (Dep.isNull() || !Dep->IsLoaded())
                return false;
        }
        return true;
    }
}
