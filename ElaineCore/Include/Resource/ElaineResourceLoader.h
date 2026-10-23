#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "Resource/ElaineResourceManager.h"
#include "TaskGraph/ElaineTask.h"
namespace Elaine
{
    class RHICommandContext;
    class ResourceLoadOperation;
    class ElaineCoreExport ResourceLoader : public Singleton<ResourceLoader>
    {
    public:
        ResourceLoader();
        ~ResourceLoader();
        void RegisterManager(ResourceType Type, ResourceManager* Manager);
        ResourceBasePtr GetResource(const std::string& Path, ResourceType Type);
        ResourceBasePtr GetResource(const std::string& Path);
        ResourceBasePtr GetResource(const ResourceReference& Reference);
        std::vector<TaskGraph::TaskCompletionEventPtr> GateResources(const std::vector<ResourceId>& IDs, const TaskGraph::TaskCompletionEventPtr& Gate);
        void ReleaseResources(const TaskGraph::TaskCompletionEventPtr& Gate);
        void RelocateResource(const ResourceId& Id, const std::string& Path);
        void UnloadResource(const ResourceId& Id);
        ResourceLoadResult LoadBlocking(const std::string& Path, ResourceType Type);
        ResourceLoadResult LoadBlocking(ResourceBasePtr Resource);
        static ResourceType InferTypeFromPath(const std::string& Path);
        ResourceManager* GetManager(ResourceType Type) const;
        void Start(RHICommandContext* UploadContext = nullptr);
        void Shutdown();
    private:
        friend class ResourceManager;
        friend class ResourceBase;
        friend class ResourceLoadOperation;
        void StartLoad(ResourceBasePtr Resource, bool Reload = false);
        void Cancel(ResourceBase* Resource);
        ResourceSubscription Subscribe(ResourceBase* Resource, ResourceCompletion Callback);
        void QueueCallback(std::function<void()> Callback);
        friend void ElaineCoreExport QueueResourceCallback(std::function<void()> Callback);
        struct State;
        std::unique_ptr<State> mState;
        ResourceManager* mManagers[RT_Count] = {};
    };
}
