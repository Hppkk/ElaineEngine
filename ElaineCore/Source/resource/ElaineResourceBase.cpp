#include "ElainePrecompiledHeader.h"
#include "ElaineResourceBase.h"
#include "ElaineResourceManager.h"
#include "ElaineDataStream.h"
#include "Resource/ElaineResourcePipeline.h"
#include "ElaineThreadManager.h"

namespace Elaine
{
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

	}

	ResourceBase::~ResourceBase()
	{
	}


    void ResourceBase::UnloadResource()
    {
        if (!ThreadManager::instance()->CheckThread(NamedThread::GameThread))
            throw std::logic_error("Resource unload requires GameThread");
        ResourcePipeline::Get().Cancel(this);
        const bool WasLoaded = IsReady();
        UnloadImpl();
        SetMemoryUsage(0);
        mLoadState = Unloaded;
        if (WasLoaded && mOwner) mOwner->OnResourceLoaded(false);
        if (mOwner)
        {
            auto Resource = mOwner->FindCachedResource(mResourceName);
            if (Resource) NotifyChanged({Resource, ResourceRequestState::Cancelled, "Resource unloaded"}, mLoadSerial.load());
        }
    }
    void ResourceBase::ReloadResource()
    {
        if (mOwner) mOwner->ReloadResource(mResourceName);
    }


	void ResourceBase::SaveResource()
	{
		SaveResourceImpl();
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

    ResourceSubscription ResourceBase::RegisterLoadCompleteCallback(LoadCompleteCallback Callback)
    {
        if (!mOwner) throw std::logic_error("Resource has no manager");
        return mOwner->RequestResource(mResourceName).OnComplete(std::move(Callback));
    }
    ResourceSubscription ResourceBase::ObserveChanges(LoadCompleteCallback Callback)
    {
        if (!mOwner) throw std::logic_error("Resource has no manager");
        auto Request = mOwner->RequestResource(mResourceName);
        auto Active = std::make_shared<std::atomic_bool>(true);
        ResourceRequestData::Listener Listener{Active, std::move(Callback)};
        std::lock_guard<std::mutex> Lock(mChangeMutex);
        const auto Result = Request.GetResult();
        const bool Complete = Result.State == ResourceRequestState::Succeeded || Result.State == ResourceRequestState::Failed || Result.State == ResourceRequestState::Cancelled;
        const uint64_t Stamp = Request.mData->LoadSerial * 8 + static_cast<uint64_t>(Result.State) + 1;
        mChangeListeners.push_back({Listener, Complete ? Stamp : 0});
        if (Complete)
            QueueResourceCallback([Listener, Result] { if (Listener.Active->load()) Listener.Callback(Result); });
        return ResourceSubscription(Active);
    }
    void ResourceBase::NotifyChanged(const ResourceLoadResult& Result, uint64_t LoadSerial)
    {
        std::vector<ResourceRequestData::Listener> Listeners;
        const uint64_t Stamp = LoadSerial * 8 + static_cast<uint64_t>(Result.State) + 1;
        {
            std::lock_guard<std::mutex> Lock(mChangeMutex);
            std::erase_if(mChangeListeners, [](const auto& Entry) { return !Entry.Listener.Active->load(); });
            for (auto& Entry : mChangeListeners)
                if (Stamp > Entry.LastStamp) { Entry.LastStamp = Stamp; Listeners.push_back(Entry.Listener); }
        }
        for (auto& Listener : Listeners)
            QueueResourceCallback([Listener, Result] { if (Listener.Active->load()) Listener.Callback(Result); });
    }

}
