#include "ElainePrecompiledHeader.h"
#include "ElaineResourceManager.h"
#include "ElaineResourceBase.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "ElaineRoot.h"
#include <filesystem>
#include <array>

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
		bool ShouldStartLoad = false;
		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			auto [It, Inserted] = mResources.emplace(InPath, RB);
			if (!Inserted)
			{
				RB = It->second;
				if (!RB.isNull() && (RB->IsLoaded() || RB->GetLoadState() == ResourceBase::Pending || RB->GetLoadState() == ResourceBase::Loading))
					return RB;
			}
			if (!RB.isNull())
			{
				RB->mLoadState = ResourceBase::Pending;
				ShouldStartLoad = true;
			}
		}
		if (!ShouldStartLoad) return RB;

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

	static bool SameResourcePath(const std::string& A, const std::string& B)
	{
		std::error_code EA, EB;
		const auto PA = std::filesystem::weakly_canonical(std::filesystem::path(A), EA);
		const auto PB = std::filesystem::weakly_canonical(std::filesystem::path(B), EB);
		if (!EA && !EB) return PA == PB;
		return std::filesystem::path(A).lexically_normal() == std::filesystem::path(B).lexically_normal();
	}

	bool ResourceManager::ReloadIfFileChanged(const std::string& InFullPath)
	{
		ResourceBasePtr Target;
		std::string RootPath;
		RootPath = Root::instance()->GetResourcePath();
		{
			std::lock_guard<std::mutex> Lock(mMtx);
			for (const auto& Pair : mResources)
			{
				if (Pair.second.isNull() || !Pair.second->IsLoaded()) continue;
				std::vector<std::string> Candidates;
				Candidates.push_back(RootPath + Pair.second->GetPath());
				Candidates.push_back(RootPath + "shader/vulkan/" + Pair.second->GetPath());
				Candidates.push_back(RootPath + "shader/" + Pair.second->GetPath());
				for (const auto& Candidate : Candidates)
					if (SameResourcePath(Candidate, InFullPath)) { Target = Pair.second; break; }
				if (!Target.isNull()) break;
			}
		}
		if (Target.isNull()) return false;
		Target->ReloadResource();
		return true;
	}

	uint32_t ResourceManager::ReloadResourcesDependingOn(const std::string& InFullPath)
	{
		std::vector<ResourceBasePtr> Targets;
		{
			std::lock_guard<std::mutex> Lock(mMtx);
			for (const auto& Pair : mResources)
			{
				if (Pair.second.isNull() || !Pair.second->IsLoaded()) continue;
				std::vector<ResourceBasePtr> Dependencies;
				Pair.second->GetDirectDependencies(Dependencies);
				for (const auto& Dependency : Dependencies)
				{
					if (Dependency.isNull()) continue;
					const std::string RootPath = Root::instance()->GetResourcePath();
					const std::array<std::string, 3> Candidates = { RootPath + Dependency->GetPath(), RootPath + "shader/vulkan/" + Dependency->GetPath(), RootPath + "shader/" + Dependency->GetPath() };
					bool Matches = false;
					for (const auto& Candidate : Candidates)
					{
						Matches = SameResourcePath(Candidate, InFullPath);
						if (Matches) break;
					}
					if (Matches) { Targets.push_back(Pair.second); break; }
				}
			}
		}
		for (const auto& Target : Targets) Target->ReloadResource();
		return static_cast<uint32_t>(Targets.size());
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
