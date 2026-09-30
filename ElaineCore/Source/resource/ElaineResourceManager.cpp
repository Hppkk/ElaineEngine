#include "ElainePrecompiledHeader.h"
#include "ElaineResourceManager.h"
#include "ElaineResourceBase.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
	ResourceManager::ResourceManager(ResourceType InType)
		: mResourceType(InType)
	{
	}

	ResourceManager::~ResourceManager()
	{
		//for (auto&& iter : mResources)
		//{
		//	delete iter.second;
		//}
		mResources.clear();

	}

	ResourceBasePtr ResourceManager::GetResource(const std::string& InPath, bool InAsync)
	{
		MemoryScope Scope({0, static_cast<uint16_t>(mResourceType), 0});
		ResourceBasePtr RB = nullptr;

		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			auto Iter = mResources.find(InPath);
			if (Iter != mResources.end())
			{
				RB = Iter->second;

				if (!RB.isNull() && (RB->IsLoaded() || RB->GetLoadState() == ResourceBase::Pending 
					|| RB->GetLoadState() == ResourceBase::Loading))
				{
					return RB;
				}
			}
		}

		RB = CreateResourceImpl(InPath);
		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			mResources.emplace(InPath, RB);
		}

		RB->mLoadState = ResourceBase::Pending;

		if (InAsync)
		{
			struct ResourceHolder
			{
				ResourceHolder(ResourceBasePtr InPtr) : mPtr(InPtr) {}
				ResourceBasePtr mPtr;
			};
			TaskGraph::GraphTaskDesc taskDesc;
			std::shared_ptr<ResourceHolder> resourceHolder = std::make_shared<ResourceHolder>(RB);
			taskDesc.mTaskFunction = [resourceHolder, Type = mResourceType] { MemoryScope Scope({0, static_cast<uint16_t>(Type), 0}); resourceHolder->mPtr->LoadResource(); };
			taskDesc.SubsequentTask([resourceHolder] { resourceHolder->mPtr->ResourceArrived(); });
			TaskGraph::GraphTaskCreateDesc createDesc;
			createDesc.mDirectTasks.push_back(taskDesc);
			TaskGraph::GraphTaskPtr LoadTaskPtr = TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady(createDesc);
			//RB->SetLoadTask(LoadTaskPtr);
		}
		else
		{
			RB->LoadResource();
			RB->ResourceArrived();
		}
		
		return RB;
	}

	ResourceBasePtr ResourceManager::CreateEmptyResource(const std::string& InPath)
	{
		MemoryScope Scope({0, static_cast<uint16_t>(mResourceType), 0});
		ResourceBasePtr RB = CreateResourceImpl(InPath);
		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			mResources.emplace(InPath, RB);
		}
		return RB;
	}

	ResourceMemoryStats ResourceManager::GetMemoryStats() const
	{
		ResourceMemoryStats Result;
		Result.CurrentBytes = mCurrentBytes.load(std::memory_order_relaxed);
		Result.PeakBytes = mPeakBytes.load(std::memory_order_relaxed);
		Result.LoadedResources = mLoadedResources.load(std::memory_order_relaxed);
		Result.TotalAllocatedBytes = mTotalAllocatedBytes.load(std::memory_order_relaxed);
		return Result;
	}

	void ResourceManager::OnResourceMemoryChanged(int64_t Delta, size_t AllocatedBytes)
	{
		if (Delta >= 0) mCurrentBytes.fetch_add(static_cast<uint64_t>(Delta), std::memory_order_relaxed);
		else mCurrentBytes.fetch_sub(static_cast<uint64_t>(-Delta), std::memory_order_relaxed);
		if (AllocatedBytes) mTotalAllocatedBytes.fetch_add(AllocatedBytes, std::memory_order_relaxed);
		const uint64_t Current = mCurrentBytes.load(std::memory_order_relaxed);
		uint64_t Peak = mPeakBytes.load(std::memory_order_relaxed);
		while (Current > Peak && !mPeakBytes.compare_exchange_weak(Peak, Current, std::memory_order_relaxed)) {}
	}

	void ResourceManager::OnResourceLoaded(bool Loaded)
	{
		if (Loaded) mLoadedResources.fetch_add(1, std::memory_order_relaxed);
		else mLoadedResources.fetch_sub(1, std::memory_order_relaxed);
	}


}
