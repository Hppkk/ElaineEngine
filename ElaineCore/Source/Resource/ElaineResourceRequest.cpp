#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceRequest.h"

namespace Elaine
{
    static bool IsTerminal(ResourceRequestState State)
    { return State == ResourceRequestState::Succeeded || State == ResourceRequestState::Failed || State == ResourceRequestState::Cancelled; }

    static void Deliver(ResourceRequestData::Listener Listener, ResourceLoadResult Result)
    {
        QueueResourceCallback([Listener = std::move(Listener), Result = std::move(Result)] {
            if (Listener.Active->exchange(false)) Listener.Callback(Result);
        });
    }

    ResourceLoadResult ResourceRequestBase::GetResult() const
    {
        if (!mData) return {{}, ResourceRequestState::Failed, "Invalid resource request"};
        std::lock_guard<std::mutex> Lock(mData->Mutex);
        return mData->Result;
    }
    ResourceRequestState ResourceRequestBase::GetState() const { return GetResult().State; }
    bool ResourceRequestBase::IsComplete() const { return IsTerminal(GetState()); }
    void ResourceRequestBase::Cancel()
    {
        if (!mData) return;
        auto Result = GetResult();
        Result.State = ResourceRequestState::Cancelled;
        Result.Error = "Resource request cancelled";
        mData->Finish(std::move(Result));
    }
    ResourceSubscription ResourceRequestBase::OnComplete(ResourceCompletion Callback) const
    {
        auto Active = std::make_shared<std::atomic_bool>(true);
        ResourceRequestData::Listener Listener{Active, std::move(Callback)};
        if (!mData) { Deliver(std::move(Listener), GetResult()); return ResourceSubscription(Active); }
        ResourceLoadResult Result;
        {
            std::lock_guard<std::mutex> Lock(mData->Mutex);
            if (!IsTerminal(mData->Result.State))
            { mData->Listeners.push_back(std::move(Listener)); return ResourceSubscription(Active); }
            Result = mData->Result;
        }
        Deliver(std::move(Listener), std::move(Result));
        return ResourceSubscription(Active);
    }
    void ResourceRequestData::Finish(ResourceLoadResult Completed)
    {
        std::vector<Listener> Pending;
        {
            std::lock_guard<std::mutex> Lock(Mutex);
            if (IsTerminal(Result.State)) return;
            Result = Completed;
            Pending.swap(Listeners);
        }
        for (auto& Listener : Pending) Deliver(std::move(Listener), Completed);
    }
    void ResourceRequestData::SetState(ResourceRequestState State)
    {
        std::lock_guard<std::mutex> Lock(Mutex);
        if (!IsTerminal(Result.State)) Result.State = State;
    }
}
