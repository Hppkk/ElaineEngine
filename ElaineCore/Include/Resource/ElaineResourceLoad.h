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
    enum class ResourceLoadState { Pending, LoadingCPU, WaitingDependencies, Uploading, Succeeded, Failed, Cancelled };
    struct ResourceLoadResult
    {
        // The manager's stable object; getters always return its current data.
        ResourcePtr<ResourceBase> Resource;
        ResourceLoadState State = ResourceLoadState::Pending;
        std::string Error;
        bool Succeeded() const { return State == ResourceLoadState::Succeeded; }
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
    struct ResourceLoadListener
    {
        std::shared_ptr<std::atomic_bool> Active;
        ResourceCompletion Callback;
    };
    void ElaineCoreExport QueueResourceCallback(std::function<void()> Callback);
}
