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
			taskDesc.mTaskFunction = [resourceHolder] { resourceHolder->mPtr->LoadResource(); };
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
		ResourceBasePtr RB = CreateResourceImpl(InPath);
		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			mResources.emplace(InPath, RB);
		}
		return RB;
	}

}