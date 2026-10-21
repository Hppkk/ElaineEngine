#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
    RenderProxy::RenderProxy() : mLifetime(std::make_shared<Lifetime>()) { mLifetime->Proxy = this; }
    RenderProxy::~RenderProxy() { CancelResourceNotifications(); }
    void RenderProxy::CancelResourceNotifications()
    {
        std::lock_guard<std::mutex> Lock(mLifetime->Mutex);
        mLifetime->Proxy = nullptr;
        mSubscriptions.clear();
    }
    void RenderProxy::TrackResource(ResourceBasePtr Resource) { TrackResourceRecursive(Resource); }
    void RenderProxy::TrackResourceRecursive(ResourceBasePtr Resource)
    {
        if (!Resource) return;
        std::lock_guard<std::mutex> Lock(mTrackMutex);
        if (!mTrackedResources.insert(std::string(typeid(*Resource).name()) + ":" + Resource->GetPath()).second) return;
        ++mPendingResourceCount;
        std::weak_ptr<Lifetime> Weak = mLifetime;
        auto First = std::make_shared<std::atomic_bool>(true);
        mSubscriptions.push_back(Resource->ObserveChanges([Weak, First](const ResourceLoadResult& Result) {
            RenderResourceUpdate Update;
            Update.Resource = Result.Resource;
            const bool Refresh = Result.Succeeded() || (Result.Resource && !Result.Resource->IsReady());
            if (Refresh && Result.Resource)
            {
                if (auto* Material = dynamic_cast<MaterialInstanceStatic*>(Result.Resource.get()))
                {
                    Update.IsMaterial = true;
                    Update.Material.Source = static_pointer_cast<MaterialInstanceStatic>(Result.Resource);
                    Update.Material.Revision = Material->GetLayoutRevision();
                    Update.Material.CaptureResources();
                }
                if (auto* Mesh = dynamic_cast<Elaine::Mesh*>(Result.Resource.get()))
                { Update.IsMesh = true; if (Mesh->IsReady()) Update.Mesh = Mesh->CreateRenderData(); }
                if (auto* Texture = dynamic_cast<Elaine::Texture*>(Result.Resource.get()))
                { Update.IsTexture = true; if (Texture->IsReady()) Update.Texture = Texture->GetTextureRHI(); }
            }
            const bool Initial = First->exchange(false);
            TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady([Weak, Result, Initial, Update = std::move(Update)] {
                if (auto Life = Weak.lock())
                {
                    std::lock_guard<std::mutex> Lock(Life->Mutex);
                    auto* Proxy = Life->Proxy;
                    if (!Proxy) return;
                    Proxy->ApplyResourceUpdate(Update);
                    if (Initial) Proxy->OnResourceLoaded(Result);
                }
            }, NamedThread::RenderThread);
        }));
    }
    void RenderProxy::OnResourceLoaded(const ResourceLoadResult& Result)
    {
        if (!Result.Succeeded()) mResourceFailed = true;
        if (--mPendingResourceCount == 0 && mInitializationStarted) TryInitializeBindings();
    }

    void RenderProxy::BeginInitialization()
    {
        mInitializationStarted = true;
        if (mPendingResourceCount == 0) TryInitializeBindings();
    }
    void RenderProxy::TryInitializeBindings()
    {
        if (mInitializationQueued.exchange(true)) return;
        std::weak_ptr<Lifetime> Weak = mLifetime;
        TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady([Weak] {
            if (auto Life = Weak.lock())
            {
                std::lock_guard<std::mutex> Lock(Life->Mutex);
                auto* Proxy = Life->Proxy;
                if (!Proxy) return;
                if (Proxy->mResourceFailed) { Proxy->OnResourceFailed(); return; }
                try
                {
                    Proxy->InitializeResourceBinding();
                    Proxy->OnResourceInitialized();
                    Proxy->mBindingsInitialized = true;
                }
                catch (...) { Proxy->OnResourceFailed(); }
            }
        }, NamedThread::RenderThread);
    }
}
