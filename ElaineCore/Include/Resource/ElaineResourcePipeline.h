#pragma once
#include "ElaineCorePrerequirements.h"
#include "Resource/ElaineResourceRequest.h"
#include <functional>

namespace Elaine
{
    class ResourceBase;
    class RHICommandContext;
    class ElaineCoreExport ResourcePipeline
    {
    public:
        static ResourcePipeline& Get();
        void Start(RHICommandContext* UploadContext = nullptr);
        ResourceRequestBase Request(ResourcePtr<ResourceBase> Resource, bool Reload = false);
        void Pump(bool DeliverCallbacks = true);
        void BeginFrame();
        void EndFrame();
        ResourceLoadResult LoadBlocking(const ResourceRequestBase& Request);
        void Shutdown();
        void Cancel(ResourceBase* Resource);
        void EnqueueCallback(std::function<void()> Callback);
    private:
        struct Operation;
        struct PublishedRecord
        {
            ResourcePtr<ResourceBase> Owner;
            std::vector<ResourcePtr<ResourceBase>> Dependencies;
        };
        std::vector<PublishedRecord> mPublished;
        std::mutex mMutex;
        std::mutex mCallbackMutex;
        std::vector<std::shared_ptr<Operation>> mOperations;
        std::vector<std::function<void()>> mCallbacks;
        std::atomic_bool mStopping{false};
        RHICommandContext* mUploadContext = nullptr;
        bool mInFrame = false;
        bool mAcceptCallbacks = true;
    };
}
