#include "ElainePrecompiledHeader.h"
#include "ElaineResourceBase.h"
#include "ElaineResourceManager.h"
#include "ElaineDataStream.h"
#include "Resource/ElaineResourceLoader.h"
#include "ElaineThreadManager.h"
#include "Resource/ElaineResourceRegistry.h"
#include "TaskGraph/ElaineTask.h"

namespace Elaine
{
    ResourceBasePtr ResourceLoadData::GetDependency(const ResourceReference& Reference, const std::string& ExpectedType) const
    {
        auto It = RequiredResources.find(Reference.Id);
        if (It == RequiredResources.end() || !It->second || !It->second->IsReady())
            throw std::runtime_error("Required dependency was not provided: " + Reference.Id.ToString());
        if (It->second->GetDescriptor().Type != ExpectedType)
            throw std::runtime_error("Dependency type mismatch for " + Reference.Id.ToString() + ": expected " + ExpectedType);
        return It->second;
    }

	ResourceListener::ResourceListener()
	{

	}

	void ResourceListener::RequestArrived()
	{

	}

	void ResourceListener::RequestResource()
	{

	}

	ResourceBase::ResourceBase(ResourceManager* InManager, const std::string& InResourceName)
		: mResourceName(InResourceName)
		, mOwner(InManager)
	{
        const auto Extension = std::filesystem::path(InResourceName).extension();
        if (Extension == ".asset" || Extension == ".shader")
        {
            const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / InResourceName;
            if (std::filesystem::is_regular_file(File))
                mDescriptor = InManager ? ResourceRegistry::Get().FindByPath(InResourceName) : AssetFile::ReadDescriptor(File);
            else mDescriptor.Id = ResourceId::New();
        }
        else mDescriptor.Id = ResourceId::New();
        mId = mDescriptor.Id;
        mDescriptor.Path = InResourceName;
        if (mDescriptor.Type.empty() && InManager) mDescriptor.Type = ResourceTypeName(InManager->GetResourceType());
	}

	ResourceBase::~ResourceBase()
	{
	}


    void ResourceBase::UnloadResource()
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread))
            throw std::logic_error("Resource unload requires GameThread");
        ResourceLoader::instance()->Cancel(this);
        const bool WasLoaded = IsReady();
        UnloadImpl();
        SetMemoryUsage(0);
        mLoadState = Unloaded;
        if (WasLoaded && mOwner) mOwner->OnResourceLoaded(false);
        if (mOwner)
        {
            auto Resource = mOwner->FindCachedResource(mResourceName);
            if (Resource) NotifyChanged({Resource, ResourceLoadState::Cancelled, "Resource unloaded"}, mLoadSerial.load());
        }
    }
    void ResourceBase::ReloadResource()
    {
        if (mOwner) mOwner->ReloadResource(mResourceName);
    }


	void ResourceBase::SaveResource()
	{
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread)) throw std::logic_error("Resource save requires GameThread");
        auto Gate = std::make_shared<TaskGraph::TaskCompletionEvent>();
        const auto Waits = ResourceLoader::instance()->GateResources({GetId()}, Gate);
        try
        {
            for (const auto& Event : Waits) if (!Event->IsComplete()) throw std::runtime_error("Save requires the current load to finish: " + mResourceName);
            SaveResourceImpl();
            mDescriptor = AssetFile::ReadDescriptor(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName);
            ResourceRegistry::Get().Refresh({mResourceName});
            ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed();
        }
        catch (...) { ResourceLoader::instance()->ReleaseResources(Gate); Gate->Succeed(); throw; }
	}



    void ResourceBase::SaveAssetJson(JsonCpp Json, const std::string& Type)
    {
        std::function<void(JsonCpp&)> UpdatePaths = [&](JsonCpp& Node) {
            if (Node.is_object())
            {
                if (Node.contains("id") && Node.contains("path"))
                {
                    auto Ref = ResourceReference::FromJson(Node);
                    try { Node["path"] = ResourceRegistry::Get().Find(Ref.Id).Path; } catch (...) {}
                    return;
                }
                for (auto& Entry : Node.items()) UpdatePaths(Entry.value());
            }
            else if (Node.is_array()) for (auto& Entry : Node) UpdatePaths(Entry);
        };
        UpdatePaths(Json);
        mDescriptor.Type = Type; mDescriptor.Path = mResourceName;
        mDescriptor.Id = mId;
        if (mDescriptor.DisplayName.empty()) mDescriptor.DisplayName = std::filesystem::path(mResourceName).stem().string();
        AssetFile::WriteJson(std::filesystem::path(Root::instance()->GetResourcePath()) / mResourceName, mDescriptor, Json);
    }

	ResourceBase::LoadState ResourceBase::GetLoadState() const
	{
		return mLoadState;
	}

	bool ResourceBase::IsLoaded() const
	{
		return mLoadState == Loaded;
	}

	size_t ResourceBase::GetMemoryUsage() const
	{
		return mMemoryUsage.load(std::memory_order_relaxed);
	}

	void ResourceBase::SetMemoryUsage(size_t Bytes)
	{
		const size_t Previous = mMemoryUsage.exchange(Bytes, std::memory_order_relaxed);
		if (mOwner && Previous != Bytes)
			mOwner->OnResourceMemoryChanged(static_cast<int64_t>(Bytes) - static_cast<int64_t>(Previous), Bytes > Previous ? Bytes - Previous : 0);
	}

	void ResourceBase::AddMemoryUsage(size_t Bytes)
	{
		mMemoryUsage.fetch_add(Bytes, std::memory_order_relaxed);
		if (mOwner) mOwner->OnResourceMemoryChanged(static_cast<int64_t>(Bytes), Bytes);
	}

	void ResourceBase::ReleaseMemoryUsage(size_t Bytes)
	{
		size_t Current = mMemoryUsage.load(std::memory_order_relaxed);
		for (;;)
		{
			const size_t Released = Bytes > Current ? Current : Bytes;
			if (mMemoryUsage.compare_exchange_weak(Current, Current - Released, std::memory_order_relaxed))
			{
				if (mOwner && Released) mOwner->OnResourceMemoryChanged(-static_cast<int64_t>(Released));
				return;
			}
		}
	}

	//void ResourceBase::GetResourceEvents(std::vector<ResourceEvent>& OutEvents)
	//{
	//	for (auto&& ResEvt : mResourceEvents)
	//	{
	//
	//	}
	//}

	//void ResourceBase::AddResourceEvent(const ResourceEvent& InEvent)
	//{
	//	mResourceEvents.emplace_back(InEvent);
	//}

	//void ResourceBase::AddResourceEvent(const std::vector<ResourceEvent>& InEvents)
	//{
	//	for (auto&& Evt : InEvents)
	//	{
	//		mResourceEvents.emplace_back(Evt);
	//	}
	//}

    ResourceLoadResult ResourceBase::GetLastLoadResult() const
    {
        std::lock_guard<std::mutex> Lock(mChangeMutex);
        return {mOwner ? mOwner->FindCachedResource(mResourceName) : ResourceBasePtr{}, mLastAttemptState.load(), mLastLoadError};
    }
    ResourceSubscription ResourceBase::OnLoaded(LoadCompleteCallback Callback)
    { return ResourceLoader::instance()->Subscribe(this, std::move(Callback)); }
    ResourceSubscription ResourceBase::ObserveChanges(LoadCompleteCallback Callback)
    {
        auto Active = std::make_shared<std::atomic_bool>(true);
        ResourceLoadListener Listener{Active, std::move(Callback)};
        ResourceLoadResult Result;
        bool Complete;
        {
            std::lock_guard<std::mutex> Lock(mChangeMutex);
            Result = {mOwner ? mOwner->FindCachedResource(mResourceName) : ResourceBasePtr{}, mLastAttemptState.load(), mLastLoadError};
            Complete = Result.State == ResourceLoadState::Succeeded || Result.State == ResourceLoadState::Failed || Result.State == ResourceLoadState::Cancelled;
            const uint64_t Stamp = mLastResultSerial * 8 + static_cast<uint64_t>(Result.State) + 1;
            mChangeListeners.push_back({Listener, Complete ? Stamp : 0});
        }
        if (Complete) QueueResourceCallback([Listener, Result] { if (Listener.Active->load()) Listener.Callback(Result); });
        return ResourceSubscription(Active);
    }
    void ResourceBase::NotifyChanged(const ResourceLoadResult& Result, uint64_t LoadSerial)
    {
        std::vector<ResourceLoadListener> Listeners;
        const uint64_t Stamp = LoadSerial * 8 + static_cast<uint64_t>(Result.State) + 1;
        {
            std::lock_guard<std::mutex> Lock(mChangeMutex);
            if (LoadSerial != mLoadSerial.load()) return;
            mLastAttemptState = Result.State;
            mLastLoadError = Result.Error;
            mLastResultSerial = LoadSerial;
            std::erase_if(mChangeListeners, [](const auto& Entry) { return !Entry.Listener.Active->load(); });
            for (auto& Entry : mChangeListeners)
                if (Stamp > Entry.LastStamp) { Entry.LastStamp = Stamp; Listeners.push_back(Entry.Listener); }
        }
        for (auto& Listener : Listeners)
            QueueResourceCallback([Listener, Result] { if (Listener.Active->load()) Listener.Callback(Result); });
    }

}
