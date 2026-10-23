#include "ElainePrecompiledHeader.h"
#include "ElaineResourceManager.h"
#include "ElaineResourceBase.h"
#include "Resource/ElaineResourceLoader.h"
#include "ElaineRoot.h"
#include "Resource/ElaineResourceRegistry.h"
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
        auto Key = CanonicalResourcePath(Path); auto Alias = mPaths.find(Key);
        auto It = mResources.find(Alias == mPaths.end() ? Key : Alias->second);
        return It == mResources.end() ? ResourceBasePtr() : It->second;
    }
    ResourceBasePtr ResourceManager::GetResource(const ResourceId& Id)
    { return GetResource(ResourceReference{Id, {}}); }
    ResourceBasePtr ResourceManager::GetResource(const ResourceReference& Ref)
    { return GetResource(ResourceRegistry::Get().Resolve(Ref, ResourceTypeName(mResourceType))); }
    ResourceBasePtr ResourceManager::GetResource(const std::string& Path)
    {
        ResourceBasePtr Resource = CreateEmptyResource(Path);
        ResourceLoader::instance()->StartLoad(Resource);
        return Resource;
    }
    ResourceBasePtr ResourceManager::ReloadResource(const std::string& Path)
    {
        ResourceBasePtr Resource = CreateEmptyResource(Path);
        ResourceLoader::instance()->StartLoad(Resource, true);
        return Resource;
    }
    ResourceBasePtr ResourceManager::CreateEmptyResource(const std::string& Path)
    {
        auto Key = CanonicalResourcePath(Path);
        std::string Identity = Key;
        const auto Extension = std::filesystem::path(Key).extension();
        if (!Extension.empty() && Extension != (mResourceType == RT_Shader ? ".shader" : ".asset"))
            throw std::runtime_error("Runtime resource path requires a typed asset container: " + Key);
        if (Extension == ".asset" || Extension == ".shader")
        {
            const auto File = std::filesystem::path(Root::instance()->GetResourcePath()) / Key;
            if (std::filesystem::is_regular_file(File))
            {
                const auto Descriptor = ResourceRegistry::Get().FindByPath(Key);
                if (Descriptor.Type != ResourceTypeName(mResourceType)) throw std::runtime_error("Wrong resource type: " + Key);
                Identity = Descriptor.Id.ToString(); Key = Descriptor.Path;
            }
        }
        std::lock_guard<std::mutex> Lock(mMtx);
        if (auto Alias = mPaths.find(Key); Alias != mPaths.end() && (Identity == Key || Identity == Alias->second)) return mResources.at(Alias->second);
        auto It = mResources.find(Identity);
        if (It == mResources.end())
        {
            auto Resource = CreateResourceImpl(Key);
            Identity = Resource->GetId().ToString();
            It = mResources.emplace(Identity, std::move(Resource)).first;
        }

        mPaths[Key] = Identity;
        return It->second;
    }
    void ResourceManager::Relocate(const ResourceId& Id, const std::string& Path)
    {
        std::lock_guard Lock(mMtx); auto It = mResources.find(Id.ToString()); if (It == mResources.end()) return;
        mPaths.erase(It->second->mResourceName); It->second->mResourceName = Path;
        It->second->mDescriptor.Path = Path; mPaths[Path] = It->first;
    }
    void ResourceManager::Unload(const ResourceId& Id)
    {
        ResourceBasePtr Resource;
        { std::lock_guard Lock(mMtx); auto It = mResources.find(Id.ToString()); if (It != mResources.end()) Resource = It->second; }
        if (Resource) Resource->UnloadResource();
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
