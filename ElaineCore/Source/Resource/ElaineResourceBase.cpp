#include "ElainePrecompiledHeader.h"
#include "ElaineResourceBase.h"
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
			LOG_ERROR("Failed to load resource.");
		}
		mLoadState = Loaded;
	}

	void ResourceBase::UnloadResource()
	{
		UnloadImpl();
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