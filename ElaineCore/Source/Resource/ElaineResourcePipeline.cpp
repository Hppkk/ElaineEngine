#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourcePipeline.h"
#include "Resource/ElaineResourceBase.h"
#include "Resource/ElaineResourceManager.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineTask.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"
#include <thread>
#include <unordered_set>

namespace Elaine
{
    struct ResourcePipeline::Operation
    {
        ResourceBasePtr Owner;
        std::unique_ptr<ResourceLoadData> Data;
        uint64_t LoadSerial = 0;
        std::atomic<ResourceRequestState> State{ResourceRequestState::LoadingCPU};
        std::atomic_bool CPUComplete{false};
        bool CPUSucceeded = false;
        std::weak_ptr<TaskGraph::GraphTask> CPUTask;
        std::string Error;
        std::shared_future<void> Upload;
        std::vector<std::shared_ptr<ResourceRequestData>> Observers;
        std::vector<ResourceBasePtr> Dependencies;
    };

    ResourcePipeline& ResourcePipeline::Get() { static ResourcePipeline Pipeline; return Pipeline; }
    void ResourcePipeline::Start(RHICommandContext* UploadContext) { mUploadContext = UploadContext; mStopping = false; mInFrame = false; std::lock_guard<std::mutex> Lock(mCallbackMutex); mAcceptCallbacks = true; }
    void QueueResourceCallback(std::function<void()> Callback) { ResourcePipeline::Get().EnqueueCallback(std::move(Callback)); }
    void ResourcePipeline::EnqueueCallback(std::function<void()> Callback)
    { std::lock_guard<std::mutex> Lock(mCallbackMutex); if (mAcceptCallbacks) mCallbacks.push_back(std::move(Callback)); }

    ResourceRequestBase ResourcePipeline::Request(ResourceBasePtr Owner, bool Reload)
    {
        auto Observer = std::make_shared<ResourceRequestData>();
        Observer->Result.Resource = Owner;
        std::lock_guard<std::mutex> Lock(mMutex);
        if (mStopping || !Owner)
        {
            Observer->Finish({Owner, ResourceRequestState::Failed, "Resource service unavailable"});
            return ResourceRequestBase(Observer);
        }
        for (auto& Operation : mOperations)
        {
            if (Operation->Owner.get() != Owner.get()) continue;
            if (!Reload && Operation->LoadSerial == Owner->mLoadSerial.load())
            {
                Observer->LoadSerial = Operation->LoadSerial;
                Observer->Result.State = Operation->State.load();
                Operation->Observers.push_back(Observer);
                return ResourceRequestBase(Observer);
            }
        }
        if (!Reload && Owner->IsReady())
        {
            Observer->LoadSerial = Owner->mLoadSerial;
            Observer->Finish({Owner, ResourceRequestState::Succeeded, {}});
            return ResourceRequestBase(Observer);
        }
        auto Work = std::make_shared<Operation>();
        Work->Owner = Owner;
        Work->LoadSerial = ++Owner->mLoadSerial;
        Owner->mLastAttemptState = ResourceRequestState::LoadingCPU;
        if (!Owner->IsReady()) Owner->mLoadState = ResourceBase::Pending;
        Observer->LoadSerial = Work->LoadSerial;
        Observer->Result.State = ResourceRequestState::LoadingCPU;
        Work->Observers.push_back(Observer);
        mOperations.push_back(Work);
        Work->CPUTask = TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady([Work] {
            try { Work->Data = Work->Owner->LoadImpl(); Work->CPUSucceeded = Work->Data != nullptr; }
            catch (const std::exception& Exception) { Work->Error = Exception.what(); }
            catch (...) { Work->Error = "Unknown resource loading exception"; }
            if (!Work->CPUSucceeded && Work->Error.empty()) Work->Error = "Failed to read or parse resource";
            Work->CPUComplete.store(true, std::memory_order_release);
        });
        return ResourceRequestBase(Observer);
    }

    void ResourcePipeline::Pump(bool DeliverCallbacks)
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread))
            throw std::logic_error("Resource publication must run on GameThread");
        std::vector<std::function<void()>> Callbacks;
        if (DeliverCallbacks)
        { std::lock_guard<std::mutex> Lock(mCallbackMutex); Callbacks.swap(mCallbacks); }
        std::vector<std::shared_ptr<Operation>> Work;
        { std::lock_guard<std::mutex> Lock(mMutex); Work = mOperations; }
        for (auto& Op : Work)
        {
            if (!Op->CPUComplete.load(std::memory_order_acquire))
            {
                auto Task = Op->CPUTask.lock();
                if (Task && !Task->Cancelled()) continue;
                Op->Error = "CPU resource task cancelled before execution";
                Op->CPUSucceeded = false;
            }
            bool Terminal = false;
            ResourceRequestState Result = ResourceRequestState::Failed;
            if (Op->Upload.valid() && Op->Upload.wait_for(std::chrono::seconds(0)) != std::future_status::ready) continue;
            if (Op->LoadSerial != Op->Owner->mLoadSerial.load() || mStopping)
            { Result = ResourceRequestState::Cancelled; Op->Error = "Resource operation superseded or stopped"; Terminal = true; }
            else if (!Op->CPUSucceeded) Terminal = true;
            else if (!Op->Upload.valid())
            {
                try
                {
                Op->State = ResourceRequestState::WaitingDependencies;
                std::vector<ResourceBasePtr> Dependencies;
                Dependencies = Op->Data->Dependencies;
                Op->Dependencies = Dependencies;
                bool Waiting = false;
                std::unordered_set<ResourceBase*> Visiting;
                std::function<bool(ResourceBase*)> HasCycle = [&](ResourceBase* Current) {
                    if (!Visiting.insert(Current).second) return true;
                    for (const auto& Other : Work)
                    {
                        if (Other->Owner.get() != Current || !Other->CPUComplete.load() ||
                            Other->LoadSerial != Other->Owner->mLoadSerial.load()) continue;
                        std::vector<ResourceBasePtr> Children;
                        if (Other->Data) Children = Other->Data->Dependencies;
                        for (auto& Child : Children) if (Child && HasCycle(Child.get())) return true;
                    }
                    Visiting.erase(Current); return false;
                };
                if (HasCycle(Op->Owner.get())) { Op->Error = "Cyclic resource dependency"; Terminal = true; }
                for (const auto& Dependency : Dependencies)
                {
                    if (!Dependency || Dependency->GetLoadState() == ResourceBase::Failed)
                    { Op->Error = "Required resource dependency failed"; Terminal = true; }
                    else
                    {
                        bool Active = false;
                        for (const auto& Other : Work)
                            if (Other->Owner.get() == Dependency.get() && Other->LoadSerial == Dependency->mLoadSerial.load()) Active = true;
                        if (Active) Waiting = true;
                        else if (!Dependency->IsReady())
                        { Op->Error = "Required resource dependency cancelled or unloaded"; Terminal = true; }
                        else if (Dependency->mLastAttemptState == ResourceRequestState::Failed)
                        { Op->Error = "Required dependency reload failed"; Terminal = true; }
                    }
                }
                if (!Terminal && !Waiting)
                {
                    Op->Owner->PrepareDependencies(*Op->Data);
                    Op->State = ResourceRequestState::Uploading;
                    auto Submitted = std::make_shared<std::promise<void>>();
                    Op->Upload = Submitted->get_future().share();
                    TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady([Op, Submitted, Context = mUploadContext] {
                        RHICommandList* List = nullptr;
                        try
                        {
                            CheckRHICreationThread();
                            auto* UploadContext = Context ? Context : GetDynamicRHI() ? GetDynamicRHI()->GetDefaultCommandContext() : nullptr;
                            if (!UploadContext) throw std::runtime_error("Resource upload context unavailable");
                            auto* Manager = UploadContext->GetRHICommandListMgr();
                            List = Manager->CreateCommandList();
                            List->SetCompletion(Submitted);
                            if (!Op->Owner->InitializeGPU(*Op->Data)) throw std::runtime_error("GPU resource initialization failed");
                            List->SubmitUploads();
                        }
                        catch (...)
                        {
                            if (List) List->DiscardCommands(std::current_exception());
                            else Submitted->set_exception(std::current_exception());
                        }
                    }, NamedThread::RenderThread);
                }
                }
                catch (const std::exception& Exception) { Op->Error = Exception.what(); Terminal = true; }
            }
            else
            {
                Terminal = true;
                try
                {
                    Op->Upload.get();
                    {
                        std::lock_guard<std::mutex> ServiceLock(mMutex);
                        if (Op->LoadSerial != Op->Owner->mLoadSerial.load()) throw std::runtime_error("Resource load superseded before delivery");
                        const bool WasReady = Op->Owner->IsReady();
                        Op->Owner->ApplyLoadedData(*Op->Data);
                        Op->Owner->SetMemoryUsage(Op->Data->MemoryUsage);
                        Op->Owner->mLoadState = ResourceBase::Loaded;
                        if (!WasReady && Op->Owner->mOwner) Op->Owner->mOwner->OnResourceLoaded(true);
                    }
                    Result = ResourceRequestState::Succeeded;
                }
                catch (const std::exception& Exception) { Op->Error = Exception.what(); if (Op->LoadSerial != Op->Owner->mLoadSerial.load()) Result = ResourceRequestState::Cancelled; }
            }
            std::vector<std::shared_ptr<ResourceRequestData>> Observers;
            {
                std::lock_guard<std::mutex> Lock(mMutex);
                if (Terminal)
                {
                    Observers.swap(Op->Observers);
                    mOperations.erase(std::remove(mOperations.begin(), mOperations.end(), Op), mOperations.end());
                }
                else Observers = Op->Observers;
            }
            if (Terminal)
            {
                if (Op->LoadSerial == Op->Owner->mLoadSerial.load())
                    Op->Owner->mLastAttemptState = Result;
                if (Result == ResourceRequestState::Failed && !Op->Owner->IsReady()) Op->Owner->mLoadState = ResourceBase::Failed;
                if (Result == ResourceRequestState::Succeeded)
                {
                    std::vector<PublishedRecord> Reloads;
                    {
                        std::lock_guard<std::mutex> Lock(mMutex);
                        auto Record = std::find_if(mPublished.begin(), mPublished.end(), [&](const PublishedRecord& R) { return R.Owner == Op->Owner; });
                        PublishedRecord Updated{Op->Owner, Op->Dependencies};
                        if (Record == mPublished.end()) mPublished.push_back(std::move(Updated)); else *Record = std::move(Updated);
                        for (const auto& R : mPublished)
                            for (const auto& Dependency : R.Dependencies)
                                if (Dependency == Op->Owner) { Reloads.push_back(R); break; }
                    }
                    // Reload dependent resources only after the new dependency has published.
                    for (auto& R : Reloads)
                    {
                        bool Active = false;
                        { std::lock_guard<std::mutex> Lock(mMutex); for (auto& Pending : mOperations) if (Pending->Owner == R.Owner && Pending->LoadSerial == R.Owner->mLoadSerial.load()) Active = true; }
                        if (!Active) Request(R.Owner, true);
                    }
                }
                ResourceLoadResult Completed{Op->Owner, Result, Op->Error};
                for (auto& Observer : Observers) Observer->Finish(Completed);
                if (Op->LoadSerial == Op->Owner->mLoadSerial.load()) Op->Owner->NotifyChanged(Completed, Op->LoadSerial);
            }
            else for (auto& Observer : Observers) Observer->SetState(Op->State.load());
        }
        for (auto& Callback : Callbacks) { try { Callback(); } catch (...) { LOG_ERROR("Resource completion callback threw"); } }
    }
    void ResourcePipeline::BeginFrame() { if (!mInFrame) Pump(); mInFrame = true; }
    void ResourcePipeline::EndFrame() { mInFrame = false; }
    ResourceLoadResult ResourcePipeline::LoadBlocking(const ResourceRequestBase& Request)
    {
        if (mInFrame || !ThreadManager::instance()->CheckThread(NamedThread::GameThread))
            throw std::logic_error("Blocking resource load is limited to startup/tool GameThread scopes");
        while (!Request.IsComplete()) { Pump(false); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
        return Request.GetResult();
    }
    void ResourcePipeline::Cancel(ResourceBase* Resource) { std::lock_guard<std::mutex> Lock(mMutex); ++Resource->mLoadSerial; Resource->mLastAttemptState = ResourceRequestState::Cancelled; std::erase_if(mPublished, [Resource](const PublishedRecord& Record) { return Record.Owner.get() == Resource; }); }
    void ResourcePipeline::Shutdown()
    {
        { std::lock_guard<std::mutex> Lock(mMutex); mStopping = true; }
        for (;;)
        {
            Pump(false);
            { std::lock_guard<std::mutex> Lock(mMutex); if (mOperations.empty()) break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        Pump(true);
        { std::lock_guard<std::mutex> Lock(mCallbackMutex); mAcceptCallbacks = false; mCallbacks.clear(); }
        { std::lock_guard<std::mutex> Lock(mMutex); mPublished.clear(); }
    }
}
