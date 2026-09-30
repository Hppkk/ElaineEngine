#include "ElainePrecompiledHeader.h"
#include "ElaineResourceBase.h"
#include "ElaineResourceManager.h"
#include "ElaineDataStream.h"
#include "TaskGraph/ElaineTaskGraph.h"

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
		UnloadResource();
	}

	void ResourceBase::LoadResource(bool InAsync/* = true*/)
	{
		if (mLoadState == Loaded)
			return;

		mLoadState = Loading;
		if (!LoadImpl())
		{
			mLoadState = Failed;
			SetMemoryUsage(0);
			LOG_ERROR("Failed to load resource.");
		}
		else
		{
			mLoadState = Loaded;
			if (mOwner) mOwner->OnResourceLoaded(true);
		}
	}

	void ResourceBase::UnloadResource()
	{
		const bool WasLoaded = mLoadState == Loaded;
		UnloadImpl();
		SetMemoryUsage(0);
		if (WasLoaded && mOwner) mOwner->OnResourceLoaded(false);
		mLoadState = Unloaded;
	}

	void ResourceBase::ReloadResource()
	{
		UnloadResource();
		LoadResource();
	}

	void ResourceBase::SaveResource()
	{
		SaveResourceImpl();
	}

	void ResourceBase::ResourceArrived()
	{
		ResourceArrivedImpl();
		mResourceArrived.store(true);
		NotifyLoadComplete();
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

	void ResourceBase::RegisterLoadCompleteCallback(LoadCompleteCallback InCallback)
	{
		std::lock_guard<std::mutex> Lock(mCallbackMutex);

		if (mResourceArrived.load())
		{
			// 资源已到达，立即执行回调
			InCallback(this);
		}
		else
		{
			// 资源未到达，加入待执行列表
			mLoadCompleteCallbacks.push_back(std::move(InCallback));
		}
	}

	void ResourceBase::NotifyLoadComplete()
	{
		std::vector<LoadCompleteCallback> Callbacks;
		{
			std::lock_guard<std::mutex> Lock(mCallbackMutex);
			Callbacks = std::move(mLoadCompleteCallbacks);
			mLoadCompleteCallbacks.clear();
		}

		for (auto& Callback : Callbacks)
		{
			Callback(this);
		}
	}
}
