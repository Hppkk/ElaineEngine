#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourcePtr.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace Elaine
{
    class ResourceBase;
    enum class ResourceRequestState { Pending, LoadingCPU, WaitingDependencies, Uploading, Succeeded, Failed, Cancelled };
    struct ResourceLoadResult
    {
        // The manager's stable object; getters always return its current data.
        ResourcePtr<ResourceBase> Resource;
        ResourceRequestState State = ResourceRequestState::Pending;
        std::string Error;
        bool Succeeded() const { return State == ResourceRequestState::Succeeded; }
    };

    // Destroying a subscription also invalidates notifications already queued for delivery.
    class ResourceSubscription
    {
    public:
        ResourceSubscription() = default;
        explicit ResourceSubscription(std::shared_ptr<std::atomic_bool> Active) : mActive(std::move(Active)) {}
        ResourceSubscription(ResourceSubscription&&) noexcept = default;
        ResourceSubscription& operator=(ResourceSubscription&& Other) noexcept
        { if (this != &Other) { Reset(); mActive = std::move(Other.mActive); } return *this; }
        ResourceSubscription(const ResourceSubscription&) = delete;
        ResourceSubscription& operator=(const ResourceSubscription&) = delete;
        ~ResourceSubscription() { Reset(); }
        void Reset() { if (mActive) mActive->store(false); mActive.reset(); }
    private:
        std::shared_ptr<std::atomic_bool> mActive;
    };

    using ResourceCompletion = std::function<void(const ResourceLoadResult&)>;
    struct ResourceRequestData;
    void ElaineCoreExport QueueResourceCallback(std::function<void()> Callback);

    // A request observes one operation; cancellation never cancels other observers.
    class ElaineCoreExport ResourceRequestBase
    {
    public:
        ResourceRequestBase() = default;
        explicit ResourceRequestBase(std::shared_ptr<ResourceRequestData> Data) : mData(std::move(Data)) {}
        ResourceLoadResult GetResult() const;
        ResourceRequestState GetState() const;
        bool IsComplete() const;
        void Cancel();
        ResourceSubscription OnComplete(ResourceCompletion Callback) const;
        explicit operator bool() const { return mData != nullptr; }
    protected:
        friend class ResourceBase;
        std::shared_ptr<ResourceRequestData> mData;
    };

    template<class T>
    class ResourceRequest : public ResourceRequestBase
    {
    public:
        ResourceRequest() = default;
        explicit ResourceRequest(const ResourceRequestBase& Request) : ResourceRequestBase(Request) {}
        ResourcePtr<T> GetResource() const { auto Result = GetResult(); return static_pointer_cast<T>(Result.Resource); }
    };

    struct ElaineCoreExport ResourceRequestData
    {
        mutable std::mutex Mutex;
        ResourceLoadResult Result;
        uint64_t LoadSerial = 0;
        struct Listener { std::shared_ptr<std::atomic_bool> Active; ResourceCompletion Callback; };
        std::vector<Listener> Listeners;
        void Finish(ResourceLoadResult Completed);
        void SetState(ResourceRequestState State);
    };
}
