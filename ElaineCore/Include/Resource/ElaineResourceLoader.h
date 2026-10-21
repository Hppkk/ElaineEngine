#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "Resource/ElaineResourceManager.h"
namespace Elaine
{
    class ElaineCoreExport ResourceLoader : public Singleton<ResourceLoader>
    {
    public:
        ResourceLoader() = default;
        ~ResourceLoader() = default;
        void RegisterManager(ResourceType Type, ResourceManager* Manager);
        ResourceRequestBase RequestResource(const std::string& Path, ResourceType Type);
        ResourceRequestBase RequestResource(const std::string& Path);
        template<class T> ResourceRequest<T> RequestResource(const std::string& Path, ResourceType Type)
        { return ResourceRequest<T>(RequestResource(Path, Type)); }
        ResourceLoadResult LoadBlocking(const std::string& Path, ResourceType Type);
        static ResourceType InferTypeFromPath(const std::string& Path);
        ResourceManager* GetManager(ResourceType Type) const;
    private:
        ResourceManager* mManagers[RT_Count] = {};
    };
}
