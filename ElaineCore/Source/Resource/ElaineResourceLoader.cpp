#include "ElainePrecompiledHeader.h"
#include "Resource/ElaineResourceLoader.h"
#include "render/ElaineShaderParameters.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineWorker.h"
#include "render/common/ElaineRHI.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"
#include <unordered_set>
#include "Resource/ElaineResourceRegistry.h"

namespace Elaine
{
    using TaskGraph::TaskCompletionEvent;
    using TaskGraph::TaskCompletionEventPtr;
    using TaskGraph::TaskState;
    using TaskGraph::TaskCategory;
    using TaskGraph::TaskContinuationPolicy;
    using TaskGraph::GraphTaskPtr;

    struct ResourceLoader::State
    {
        std::mutex Mutex;
        bool Stopping = true;
        std::map<ResourceId, TaskCompletionEventPtr> LocationGates;
        uint64_t NextID = 0;
        RHICommandContext* UploadContext = nullptr;
        std::shared_ptr<std::atomic_bool> CallbacksActive;
        // These maps own/find operations; no frame entry iterates them.
        std::map<uint64_t, std::shared_ptr<ResourceLoadOperation>> Operations;
        std::map<ResourceBase*, std::shared_ptr<ResourceLoadOperation>> Current;
        struct PublishedRecord { ResourceBasePtr Owner; std::vector<ResourceBasePtr> Dependencies; };
        std::map<ResourceBase*, PublishedRecord> Published;
        std::map<ResourceBase*, std::set<ResourceBase*>> Dependents;
    };

    class ResourceLoadOperation : public std::enable_shared_from_this<ResourceLoadOperation>
    {
    public:
        ResourceLoader& Loader;
        ResourceBasePtr Owner;
        uint64_t ID = 0, Serial = 0;
        std::unique_ptr<ResourceLoadData> Data;
        ResourceDescriptor Descriptor;
        std::vector<ResourceBasePtr> Dependencies;
        TaskCompletionEventPtr Published = std::make_shared<TaskCompletionEvent>();
        std::atomic_bool Cancelled{false};
        bool PreparationStarted = false;
        TaskCompletionEventPtr LocationGate;
        bool DependencyChanged = false;
        std::vector<uint64_t> CapturedDependencySerials;
        ResourceLoadResult Result; // Written before Published; read by callback tasks after that event.
        std::mutex TaskMutex;
        std::vector<std::weak_ptr<TaskGraph::GraphTask>> Tasks;
        ResourceLoadOperation(ResourceLoader& InLoader, ResourceBasePtr Resource) : Loader(InLoader), Owner(std::move(Resource)) {}
        void Check() const
        {
            if (Cancelled || Serial != Owner->mLoadSerial.load()) throw std::runtime_error("Resource operation superseded or cancelled");
        }
        GraphTaskPtr Task(std::function<void()> Function, NamedThread Thread, bool Always = false)
        {
            auto Result = TaskGraph::TaskGraph::instance()->CreateTask(std::move(Function), Thread);
            Result->SetCategory(TaskCategory::ResourceInternal);
            if (Always) Result->SetContinuationPolicy(TaskContinuationPolicy::Always);
            return Result;
        }
        void Track(const GraphTaskPtr& Task)
        {
            bool Cancel;
            { std::lock_guard<std::mutex> Lock(TaskMutex); Tasks.push_back(Task); Cancel = Cancelled; }
            if (Cancel) Task->Cancel();
        }
        void CancelWork()
        {
            std::vector<GraphTaskPtr> Pending;
            { std::lock_guard<std::mutex> Lock(TaskMutex); Cancelled = true;
              for (auto& Weak : Tasks) if (auto Task = Weak.lock()) Pending.push_back(Task); }
            for (auto& Task : Pending) Task->Cancel();
        }
        void Start()
        {
            auto Self = shared_from_this();
            auto Metadata = Task([Self] {
                Self->Check();
                const auto Extension = std::filesystem::path(Self->Owner->GetPath()).extension();
                if (Extension == ".asset" || Extension == ".shader")
                {
                    Self->Descriptor = AssetFile::ReadDescriptor(std::filesystem::path(Root::instance()->GetResourcePath()) / Self->Owner->GetPath());
                    if (Self->Descriptor.Id != Self->Owner->GetId()) throw std::runtime_error("Resource identity changed on disk");
                    Self->Descriptor.Path = Self->Owner->GetPath();
                }
            }, NamedThread::AnyThread);
            if (LocationGate) Metadata->Dependency(LocationGate);
            auto MetadataEvent = Metadata->GetCompletionEvent();
            auto Connect = Task([Self, MetadataEvent] {
                try { MetadataEvent->RethrowIfFailed(); Self->Check(); Self->ConnectDependencies(); }
                catch (...) { Self->Finish(std::current_exception()); }
            }, NamedThread::GameThread, true);
            Connect->Dependency(Metadata);
            Track(Metadata);
            auto* Graph = TaskGraph::TaskGraph::instance();
            Graph->Dispatch(Connect); Graph->Dispatch(Metadata);
        }
        // Only GameThread builds resource edges. DFS follows active and published edges by lookup.
        void ConnectDependencies()
        {
            if (Descriptor.Id.IsValid())
            {
                for (const auto& Dependency : Descriptor.Dependencies)
                {
                    if (Dependency.Kind == ResourceDependencyKind::Soft) continue;
                    try
                    {
                        auto Resource = Loader.GetResource(Dependency.Reference);
                        if (!Resource) throw std::runtime_error("No manager for required resource type");
                        Dependencies.push_back(std::move(Resource));
                    }
                    catch (const std::exception& Error)
                    { throw std::runtime_error("Required dependency " + Dependency.Reference.Path + " (" + Dependency.Reference.Id.ToString() + "): " + Error.what()); }
                }
            }
            else Owner->GetDirectDependencies(Dependencies); // In-memory resources have no serialized descriptor.
            std::vector<std::pair<ResourceBasePtr, TaskCompletionEventPtr>> Waits;
            {
                std::lock_guard<std::mutex> Lock(Loader.mState->Mutex);
                Check();
                std::unordered_set<ResourceBase*> Visiting;
                std::function<bool(ResourceBase*)> ReachesOwner = [&](ResourceBase* Resource) {
                    if (Resource == Owner.get()) return true;
                    if (!Visiting.insert(Resource).second) return false;
                    auto Active = Loader.mState->Current.find(Resource);
                    if (Active != Loader.mState->Current.end())
                    { for (const auto& Child : Active->second->Dependencies) if (Child && ReachesOwner(Child.get())) return true; }
                    else
                    {
                        auto Ready = Loader.mState->Published.find(Resource);
                        if (Ready != Loader.mState->Published.end())
                            for (const auto& Child : Ready->second.Dependencies) if (Child && ReachesOwner(Child.get())) return true;
                    }
                    return false;
                };
                for (auto& Dependency : Dependencies)
                {
                    if (!Dependency) throw std::runtime_error("Null required resource dependency: " + Owner->GetPath());
                    Visiting.clear();
                    if (ReachesOwner(Dependency.get())) throw std::runtime_error("Cyclic resource dependency: " + Owner->GetPath() + " -> " + Dependency->GetPath());
                    auto Active = Loader.mState->Current.find(Dependency.get());
                    if (Active != Loader.mState->Current.end()) Waits.emplace_back(Dependency, Active->second->Published);
                    else
                    {
                        auto Event = std::make_shared<TaskCompletionEvent>();
                        if (Dependency->IsReady() && Dependency->GetLoadingState() == ResourceLoadState::Succeeded) Event->Succeed();
                        else Event->Fail(std::make_exception_ptr(std::runtime_error(Dependency->GetLastLoadResult().Error.empty() ? "Dependency unavailable" : Dependency->GetLastLoadResult().Error)));
                        Waits.emplace_back(Dependency, Event);
                    }
                }
                Owner->mLastAttemptState = ResourceLoadState::WaitingDependencies;
            }
            auto Self = shared_from_this();
            auto CPU = Task([Self] {
                Self->Check();
                Self->Data = Self->Owner->LoadImpl();
                if (!Self->Data) throw std::runtime_error("Failed to read or parse resource: " + Self->Owner->GetPath());
                if (Self->Descriptor.Id.IsValid())
                {
                    const auto Current = AssetFile::ReadDescriptor(std::filesystem::path(Root::instance()->GetResourcePath()) / Self->Owner->GetPath());
                    if (Current.ToJson() != Self->Descriptor.ToJson()) throw std::runtime_error("Resource metadata changed during loading");
                }
            }, NamedThread::AnyThread);
            auto CPUEvent = CPU->GetCompletionEvent();
            auto Prepare = Task([Self, Waits, CPUEvent] {
                Self->Check(); CPUEvent->RethrowIfFailed();
                for (auto& Wait : Waits)
                {
                    try { Wait.second->RethrowIfFailed(); }
                    catch (const std::exception& Error) { throw std::runtime_error("Required dependency " + Wait.first->GetPath() + ": " + Error.what()); }
                }
                {
                    std::lock_guard<std::mutex> Lock(Self->Loader.mState->Mutex);
                    Self->Check(); Self->PreparationStarted = true;
                    for (auto& Dependency : Self->Dependencies) Self->CapturedDependencySerials.push_back(Dependency->mLoadSerial.load());
                }
                Self->Data->Descriptor = Self->Descriptor;
                for (const auto& Dependency : Self->Dependencies)
                    Self->Data->RequiredResources.emplace(Dependency->GetId(), Dependency);
                Self->Owner->PrepareDependencies(*Self->Data);
                { std::lock_guard<std::mutex> Lock(Self->Loader.mState->Mutex); Self->Check(); Self->Owner->mLastAttemptState = ResourceLoadState::Uploading; }
            }, NamedThread::GameThread, true);
            Prepare->Dependency(CPU);
            for (auto& Wait : Waits) Prepare->Dependency(Wait.second);
            auto Prepared = Prepare->GetCompletionEvent();
            auto Submitted = std::make_shared<TaskCompletionEvent>();
            auto Render = Task([Self, Submitted] {
                RHICommandList* List = nullptr;
                try
                {
                    Self->Check(); CheckRHICreationThread();
                    auto* Context = Self->Loader.mState->UploadContext;
                    if (!Context && GetDynamicRHI()) Context = GetDynamicRHI()->GetDefaultCommandContext();
                    if (!Context) throw std::runtime_error("Resource upload context unavailable");
                    List = Context->GetRHICommandListMgr()->CreateCommandList();
                    List->SetSubmissionEvent(Submitted);
                    if (!Self->Owner->InitializeGPU(*Self->Data)) throw std::runtime_error("GPU resource initialization failed");
                    List->SubmitUploads();
                }
                catch (...)
                {
                    if (List) List->DiscardCommands(std::current_exception());
                    else Submitted->Fail(std::current_exception());
                    throw;
                }
            }, NamedThread::RenderThread);
            Render->Dependency(Prepare);
            // A cancelled/failed prerequisite can prevent recording the list entirely.
            Render->GetCompletionEvent()->Subscribe([Submitted, Prepared](TaskState State) {
                if (State != TaskState::Succeeded)
                {
                    if (auto Error = Prepared->GetException()) Submitted->Fail(Error);
                    else Submitted->Cancel();
                }
            });
            auto Recorded = Render->GetCompletionEvent();
            auto Publish = Task([Self, Prepared, Recorded, Submitted] {
                try
                {
                    Self->Check(); Prepared->RethrowIfFailed(); Recorded->RethrowIfFailed(); Submitted->RethrowIfFailed();
                    Self->Finish({});
                }
                catch (...)
                {
                    if (Submitted->GetState() == TaskState::Cancelled && Prepared->GetState() == TaskState::Succeeded)
                        Self->Cancelled = true;
                    Self->Finish(std::current_exception());
                }
            }, NamedThread::GameThread, true);
            Publish->Dependency(CPUEvent); // Cancellation can finish Prepare before a running CPU task ends.
            Publish->Dependency(Submitted);
            Publish->Dependency(Recorded); // Never dispose temporary data while recording still uses it.
            Track(CPU); Track(Prepare); Track(Render);
            auto* Graph = TaskGraph::TaskGraph::instance();
            Graph->Dispatch(Publish); Graph->Dispatch(Render); Graph->Dispatch(Prepare); Graph->Dispatch(CPU);
        }
        void Finish(std::exception_ptr Error)
        {
            ResourceLoadState ResultState = Error ? ResourceLoadState::Failed : ResourceLoadState::Succeeded;
            std::string Message;
            if (Error) { try { std::rethrow_exception(Error); } catch (const std::exception& E) { Message = E.what(); } catch (...) { Message = "Unknown resource loading failure"; } }
            std::vector<ResourceBasePtr> Reloads;
            bool Current;
            {
                std::lock_guard<std::mutex> Lock(Loader.mState->Mutex);
                if (Published->IsComplete()) return;
                Current = Serial == Owner->mLoadSerial.load();
                if (!Current || Cancelled || Loader.mState->Stopping)
                { ResultState = ResourceLoadState::Cancelled; Message = "Resource operation superseded or stopped"; }
                if (ResultState == ResourceLoadState::Succeeded)
                {
                    for (size_t Index = 0; Index < Dependencies.size(); ++Index)
                        if (CapturedDependencySerials[Index] != Dependencies[Index]->mLoadSerial.load() &&
                            Dependencies[Index]->GetLoadingState() == ResourceLoadState::Succeeded) DependencyChanged = true;
                    const bool WasReady = Owner->IsReady();
                    Owner->ApplyLoadedData(*Data);
                    if (Data->Descriptor.Id.IsValid()) Owner->mDescriptor = std::move(Data->Descriptor);
                    Owner->SetMemoryUsage(Data->MemoryUsage); Owner->mLoadState = ResourceBase::Loaded;
                    if (!WasReady && Owner->mOwner) Owner->mOwner->OnResourceLoaded(true);
                    auto Previous = Loader.mState->Published.find(Owner.get());
                    if (Previous != Loader.mState->Published.end())
                        for (auto& Dependency : Previous->second.Dependencies) Loader.mState->Dependents[Dependency.get()].erase(Owner.get());
                    Loader.mState->Published[Owner.get()] = {Owner, Dependencies};
                    for (auto& Dependency : Dependencies) Loader.mState->Dependents[Dependency.get()].insert(Owner.get());
                    auto Dependent = Loader.mState->Dependents.find(Owner.get());
                    if (Dependent != Loader.mState->Dependents.end())
                        for (auto* Resource : Dependent->second)
                        {
                            auto Active = Loader.mState->Current.find(Resource);
                            if (Active == Loader.mState->Current.end()) Reloads.push_back(Loader.mState->Published.at(Resource).Owner);
                            else if (Active->second->PreparationStarted) Active->second->DependencyChanged = true;
                        }
                }
                if (Current)
                {
                    if (ResultState == ResourceLoadState::Failed && !Owner->IsReady()) Owner->mLoadState = ResourceBase::Failed;
                    std::lock_guard<std::mutex> ChangeLock(Owner->mChangeMutex);
                    Owner->mLastAttemptState = ResultState; Owner->mLastLoadError = Message; Owner->mLastResultSerial = Serial;
                    Loader.mState->Current.erase(Owner.get());
                }
                if (Current && DependencyChanged && !Loader.mState->Stopping) Reloads.push_back(Owner);
                Result = {Owner, ResultState, Message};
                Loader.mState->Operations.erase(ID);
            }
            // Recording/submission and CPU parsing have ended; observers retain only the stable resource.
            Data.reset();
            if (Current) Owner->NotifyChanged(Result, Serial);
            // Events release only internal dependents. Business delivery remains a separate task.
            if (ResultState == ResourceLoadState::Succeeded) Published->Succeed();
            else if (ResultState == ResourceLoadState::Cancelled) Published->Cancel();
            else Published->Fail(std::make_exception_ptr(std::runtime_error(Message)));
            for (auto& Resource : Reloads) Loader.StartLoad(Resource, true);
        }
    };

    ResourceLoader::ResourceLoader() : mState(std::make_unique<State>()) {}
    ResourceLoader::~ResourceLoader() { Shutdown(); }
    void ResourceLoader::Start(RHICommandContext* UploadContext)
    {
        InitializeShaderParameterRegistry();
        std::lock_guard<std::mutex> Lock(mState->Mutex);
        if (!mState->Operations.empty()) throw std::logic_error("Resource loader still has active operations");
        mState->UploadContext = UploadContext; mState->Stopping = false;
        mState->CallbacksActive = std::make_shared<std::atomic_bool>(true);
    }
    void ResourceLoader::RegisterManager(ResourceType Type, ResourceManager* Manager)
    { if (Type < RT_Count) mManagers[Type] = Manager; }
    ResourceManager* ResourceLoader::GetManager(ResourceType Type) const
    { return Type < RT_Count ? mManagers[Type] : nullptr; }
    ResourceBasePtr ResourceLoader::GetResource(const std::string& Path, ResourceType Type)
    { auto* Manager = GetManager(Type); return Manager ? Manager->GetResource(Path) : ResourceBasePtr{}; }
    ResourceBasePtr ResourceLoader::GetResource(const std::string& Path) { return GetResource(Path, InferTypeFromPath(Path)); }
    ResourceBasePtr ResourceLoader::GetResource(const ResourceReference& Ref)
    { const auto Descriptor = ResourceRegistry::Get().Find(Ref.Id); return GetResource(Descriptor.Path, static_cast<ResourceType>(ResourceTypeFromName(Descriptor.Type))); }
    std::vector<TaskCompletionEventPtr> ResourceLoader::GateResources(const std::vector<ResourceId>& IDs, const TaskCompletionEventPtr& Gate)
    {
        std::vector<TaskCompletionEventPtr> Waits; std::lock_guard Lock(mState->Mutex);
        for (const auto& Id : IDs) if (mState->LocationGates.contains(Id)) throw std::runtime_error("Resource already has a pending file operation");
        for (const auto& Id : IDs)
        {
            mState->LocationGates[Id] = Gate;
            for (const auto& Pair : mState->Operations) if (Pair.second->Owner->GetId() == Id) Waits.push_back(Pair.second->Published);
        }
        return Waits;
    }
    void ResourceLoader::ReleaseResources(const TaskCompletionEventPtr& Gate)
    { std::lock_guard Lock(mState->Mutex); std::erase_if(mState->LocationGates, [&](const auto& Pair) { return Pair.second == Gate; }); }
    void ResourceLoader::RelocateResource(const ResourceId& Id, const std::string& Path)
    { for (auto* Manager : mManagers) if (Manager) Manager->Relocate(Id, Path); }
    void ResourceLoader::UnloadResource(const ResourceId& Id)
    { for (auto* Manager : mManagers) if (Manager) Manager->Unload(Id); }
    void ResourceLoader::StartLoad(ResourceBasePtr Resource, bool Reload)
    {
        if (!Resource) return;
        std::shared_ptr<ResourceLoadOperation> Previous, Work;
        {
            std::lock_guard<std::mutex> Lock(mState->Mutex);
            if (mState->Stopping) return;
            auto Active = mState->Current.find(Resource.get());
            if (!Reload && (Active != mState->Current.end() || Resource->IsReady() ||
                (Resource->GetLoadingState() == ResourceLoadState::Failed && Resource->GetLoadState() != ResourceBase::Unloaded))) return;
            if (Active != mState->Current.end()) Previous = Active->second;
            Work = std::make_shared<ResourceLoadOperation>(*this, Resource);
            Work->ID = ++mState->NextID;
            if (auto Gate = mState->LocationGates.find(Resource->GetId()); Gate != mState->LocationGates.end()) Work->LocationGate = Gate->second;
            { std::lock_guard<std::mutex> ChangeLock(Resource->mChangeMutex);
              Work->Serial = ++Resource->mLoadSerial; Resource->mLastAttemptState = ResourceLoadState::LoadingCPU; }
            if (!Resource->IsReady()) Resource->mLoadState = ResourceBase::Pending;
            mState->Current[Resource.get()] = Work; mState->Operations.emplace(Work->ID, Work);
        }
        if (Previous) Previous->CancelWork();
        Work->Start();
    }
    void ResourceLoader::QueueCallback(std::function<void()> Callback)
    {
        std::shared_ptr<std::atomic_bool> Active;
        { std::lock_guard<std::mutex> Lock(mState->Mutex); Active = mState->CallbacksActive; }
        if (!Active || !Active->load()) return;
        auto Task = TaskGraph::TaskGraph::instance()->CreateTask([Active, Callback = std::move(Callback)] { if (Active->load()) Callback(); }, NamedThread::GameThread);
        Task->SetCategory(TaskCategory::ResourceCallback);
        TaskGraph::TaskGraph::instance()->Dispatch(Task);
    }
    void QueueResourceCallback(std::function<void()> Callback) { ResourceLoader::instance()->QueueCallback(std::move(Callback)); }
    ResourceSubscription ResourceLoader::Subscribe(ResourceBase* Resource, ResourceCompletion Callback)
    {
        auto Active = std::make_shared<std::atomic_bool>(true);
        std::shared_ptr<std::atomic_bool> ServiceActive;
        std::shared_ptr<ResourceLoadOperation> Operation;
        ResourceLoadResult Recent;
        {
            std::lock_guard<std::mutex> Lock(mState->Mutex);
            if (mState->Stopping) return {};
            ServiceActive = mState->CallbacksActive;
            auto Current = mState->Current.find(Resource);
            if (Current != mState->Current.end()) Operation = Current->second;
            else Recent = Resource->GetLastLoadResult();
        }
        if (!ServiceActive || !ServiceActive->load()) return ResourceSubscription(Active);
        auto Task = TaskGraph::TaskGraph::instance()->CreateTask(
            [Active, ServiceActive, Operation, Recent, Callback = std::move(Callback)] {
                if (ServiceActive->load() && Active->exchange(false)) Callback(Operation ? Operation->Result : Recent);
            }, NamedThread::GameThread);
        Task->SetCategory(TaskCategory::ResourceCallback);
        Task->SetContinuationPolicy(TaskContinuationPolicy::Always);
        if (Operation) Task->Dependency(Operation->Published);
        TaskGraph::TaskGraph::instance()->Dispatch(Task);
        return ResourceSubscription(Active);
    }
    ResourceLoadResult ResourceLoader::LoadBlocking(const std::string& Path, ResourceType Type)
    { return LoadBlocking(GetResource(Path, Type)); }
    ResourceLoadResult ResourceLoader::LoadBlocking(ResourceBasePtr Resource)
    {
        auto* Graph = TaskGraph::TaskGraph::instance();
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread) || Graph->IsInGameThreadFrame())
            throw std::logic_error("Blocking resource load is limited to startup/tool GameThread scopes");
        if (!Resource) return {{}, ResourceLoadState::Failed, "Unsupported or missing resource"};
        std::shared_ptr<ResourceLoadOperation> Operation;
        { std::lock_guard<std::mutex> Lock(mState->Mutex); auto Current = mState->Current.find(Resource.get());
          if (Current != mState->Current.end()) Operation = Current->second; }
        if (Operation)
        {
            auto Event = Operation->Published;
            auto* Worker = Graph->GetWorker(NamedThread::GameThread);
            Event->Subscribe([Worker](TaskState) { Worker->Wake(); });
            while (!Event->IsComplete()) { Graph->ExecuteResourceTasks(); if (!Event->IsComplete()) Graph->WaitForResourceTasks(Event); }
        }
        return Operation ? Operation->Result : Resource->GetLastLoadResult();
    }
    void ResourceLoader::Cancel(ResourceBase* Resource)
    {
        std::shared_ptr<ResourceLoadOperation> Work;
        {
            std::lock_guard<std::mutex> Lock(mState->Mutex);
            { std::lock_guard<std::mutex> ChangeLock(Resource->mChangeMutex);
              ++Resource->mLoadSerial; Resource->mLastAttemptState = ResourceLoadState::Cancelled; }
            auto Current = mState->Current.find(Resource);
            if (Current != mState->Current.end()) { Work = Current->second; mState->Current.erase(Current); }
            auto Previous = mState->Published.find(Resource);
            if (Previous != mState->Published.end())
            {
                for (auto& Dependency : Previous->second.Dependencies) mState->Dependents[Dependency.get()].erase(Resource);
                mState->Published.erase(Previous);
            }
        }
        if (Work) Work->CancelWork();
    }
    void ResourceLoader::Shutdown()
    {
        std::vector<std::shared_ptr<ResourceLoadOperation>> Pending;
        {
            std::lock_guard<std::mutex> Lock(mState->Mutex);
            if (mState->Stopping && mState->Operations.empty()) return;
            mState->Stopping = true;
            for (auto& Entry : mState->Operations) Pending.push_back(Entry.second);
        }
        auto* Graph = TaskGraph::TaskGraph::instance();
        for (auto& Work : Pending) Work->CancelWork();
        for (auto& Work : Pending)
        {
            auto Event = Work->Published;
            auto* Worker = Graph->GetWorker(NamedThread::GameThread);
            Event->Subscribe([Worker](TaskState) { Worker->Wake(); });
            while (!Event->IsComplete()) { Graph->ExecuteResourceTasks(); if (!Event->IsComplete()) Graph->WaitForResourceTasks(Event); }
        }
        // Dispatch all preceding completion tasks, then deliver one fixed callback batch
        // while World and devices are still alive. New subscriptions are already refused.
        Graph->CreateAndDispatchWhenReady([] {})->GetCompletionEvent()->Wait();
        Graph->ExecuteResourceCallbacks();
        { std::lock_guard<std::mutex> Lock(mState->Mutex);
          if (mState->CallbacksActive) mState->CallbacksActive->store(false); }
        Graph->CancelTasks(TaskCategory::ResourceCallback);
        std::lock_guard<std::mutex> Lock(mState->Mutex);
        mState->Current.clear(); mState->Published.clear(); mState->Dependents.clear();
    }
    ResourceType ResourceLoader::InferTypeFromPath(const std::string& Path)
    {
        try
        {
            const auto Descriptor = AssetFile::ReadDescriptor(std::filesystem::path(Root::instance()->GetResourcePath()) / CanonicalResourcePath(Path));
            return static_cast<ResourceType>(ResourceTypeFromName(Descriptor.Type));
        }
        catch (...) { return RT_Count; }
    }
}
