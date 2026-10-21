#include "ElainePrecompiledHeader.h"
#include "ElaineResourceManager.h"
#include "ElaineResourceBase.h"
#include "Resource/ElaineResourcePipeline.h"
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

    ResourceBasePtr ResourceManager::FindCachedResource(const std::string& Path)
    {
        std::lock_guard<std::mutex> Lock(mMtx);
        auto It = mResources.find(std::filesystem::path(Path).lexically_normal().generic_string());
        return It == mResources.end() ? ResourceBasePtr() : It->second;
    }
    ResourceRequestBase ResourceManager::RequestResource(const std::string& Path)
    {
        ResourceBasePtr Resource;
        const std::string Key = std::filesystem::path(Path).lexically_normal().generic_string();
        {
            std::lock_guard<std::mutex> Lock(mMtx);
            auto It = mResources.find(Key);
            if (It == mResources.end()) It = mResources.emplace(Key, CreateResourceImpl(Key)).first;
            Resource = It->second;
        }
        return ResourcePipeline::Get().Request(Resource);
    }
    ResourceRequestBase ResourceManager::ReloadResource(const std::string& Path)
    {
        ResourceBasePtr Resource;
        const std::string Key = std::filesystem::path(Path).lexically_normal().generic_string();
        {
            std::lock_guard<std::mutex> Lock(mMtx);
            auto It = mResources.find(Key);
            if (It == mResources.end()) It = mResources.emplace(Key, CreateResourceImpl(Key)).first;
            Resource = It->second;
        }
        return ResourcePipeline::Get().Request(Resource, true);
    }

	ResourceBasePtr ResourceManager::CreateEmptyResource(const std::string& InPath)
    {
        MemoryScope Scope({0, static_cast<uint16_t>(mResourceType), 0});
        const auto Key = std::filesystem::path(InPath).lexically_normal().generic_string();
        std::lock_guard<std::mutex> Lock(mMtx);
        auto It = mResources.find(Key);
        if (It == mResources.end()) It = mResources.emplace(Key, CreateResourceImpl(Key)).first;
        return It->second;
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
        std::vector<ResourceBasePtr> Targets;
        const auto RootPath = Root::instance()->GetResourcePath();
        {
            std::lock_guard<std::mutex> Lock(mMtx);
            for (const auto& Pair : mResources)
            {
                if (!Pair.second || !Pair.second->IsReady()) continue;
                for (const auto& Source : Pair.second->GetSourceFiles())
                {
                    const auto Path = std::filesystem::path(Source);
                    if (SameResourcePath(Path.is_absolute() ? Source : RootPath + Source, InFullPath))
                    { Targets.push_back(Pair.second); break; }
                }
            }
        }
        for (auto& Target : Targets) Target->ReloadResource();
        return !Targets.empty();
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
