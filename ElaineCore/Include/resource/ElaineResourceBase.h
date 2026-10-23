#pragma once
#include <set>
#include <mutex>
#include "ElaineCorePrerequirements.h"
#include "ElaineResourcePtr.h"
#include "Resource/ElaineResourceLoad.h"
#include "Resource/ElaineAsset.h"

namespace TaskGraph { class GraphTask; }

namespace Elaine
{
	enum ResourceType : uint8_t;  // 前向声明

	enum ResLoadPriority
	{
		ResLoadPriority_Low,
		ResLoadPriority_Mid,
		ResLoadPriority_High,
	};

	class ResourceBase;
	using ResourceBasePtr = ResourcePtr<ResourceBase>;
	// Private working storage for one load. It is never a cached resource.
    struct ElaineCoreExport ResourceLoadData
    {
        virtual ~ResourceLoadData() = default;
        std::map<ResourceId, ResourceBasePtr> RequiredResources;
        ResourceBasePtr GetDependency(const ResourceReference& Reference, const std::string& ExpectedType) const;
        size_t MemoryUsage = 0;
        ResourceDescriptor Descriptor;
    };

    class ElaineCoreExport ResourceListener
	{
	public:
		ResourceListener();
		~ResourceListener() = default;
		void RequestArrived();
		void		RequestResource();
	private:
		ResourceBasePtr		mResource;
	};

	using ResourceListenerSet = std::set<ResourceListener*>;

	class ElaineCoreExport ResourceBase 
	{
		friend class ResourceManager;
        friend class ResourceLoader;
        friend class ResourceLoadOperation;
	public:
		enum LoadState : uint8_t
		{
			Unloaded, //未加载
			Pending,  //等待加载
			Loading,  //正在加载
            Finalizing,
			Loaded,   //加载完成
			Unloading, //卸载中
			Failed,  //失败
			Missing, //资源不存在
		};

		using LoadCompleteCallback = ResourceCompletion;

	public:
		ResourceBase() = default;
		ResourceBase(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~ResourceBase();
		void					UnloadResource();
		virtual void				ReloadResource();
		void					SaveResource();
		LoadState				GetLoadState() const;
		bool                    IsLoaded() const;
        bool IsReady() const { return IsLoaded(); }
        std::atomic<ResourceLoadState> mLastAttemptState{ResourceLoadState::Pending};
		size_t				GetMemoryUsage() const;
		void					SetMemoryUsage(size_t Bytes);
		void					AddMemoryUsage(size_t Bytes);
		void					ReleaseMemoryUsage(size_t Bytes);
		const std::string&		GetPath() const { return mResourceName; }
        const ResourceId& GetId() const { return mId; }
        ResourceReference GetReference() const { return {GetId(), mResourceName}; }
        ResourceDescriptor GetDescriptor() const { auto Result = mDescriptor; Result.Path = mResourceName; return Result; }


		//const std::vector<ResourceEvent>& GetResourceEvents() const { return mResourceEvents; }
		//void GetResourceEvents(std::vector<ResourceEvent>& OutEvents);
		//void AddResourceEvent(const ResourceEvent& InEvent);
		//void AddResourceEvent(const std::vector<ResourceEvent>& InEvents);
		//void ClearDependencies() { mResourceEvents.clear(); }

		ResourceSubscription OnLoaded(LoadCompleteCallback Callback);
        ResourceLoadResult GetLastLoadResult() const;
        ResourceLoadState GetLoadingState() const { return mLastAttemptState.load(); }
        ResourceSubscription ObserveChanges(LoadCompleteCallback Callback);

		virtual void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const {}
        virtual std::vector<std::string> GetSourceFiles() const { return {mResourceName}; }

	protected:
        void SaveAssetJson(JsonCpp Json, const std::string& Type);
        void NotifyChanged(const ResourceLoadResult& Result, uint64_t LoadSerial);
        // Worker -> GameThread -> RenderThread -> GameThread.
        // All fallible preparation is completed before the final noexcept swap.
        virtual std::unique_ptr<ResourceLoadData> LoadImpl() = 0;
        virtual void PrepareDependencies(ResourceLoadData& Data) {}
        virtual bool InitializeGPU(ResourceLoadData& Data) { return true; }
        virtual void ApplyLoadedData(ResourceLoadData& Data) noexcept = 0;
		virtual	void			UnloadImpl() = 0;
		virtual void			SaveResourceImpl() = 0;


	protected:
		std::string				mResourceName;
        ResourceDescriptor mDescriptor;
        ResourceId mId = ResourceId::New();
		ResourceManager* mOwner = nullptr;
		std::atomic<size_t>		mMemoryUsage{0};
		std::atomic<LoadState> mLoadState = Unloaded;
        std::atomic<uint64_t> mLoadSerial{0};
        struct ChangeListener
        {
            ResourceLoadListener Listener;
            uint64_t LastStamp = 0;
        };
        mutable std::mutex mChangeMutex;
        std::vector<ChangeListener> mChangeListeners;
        std::string mLastLoadError;
        uint64_t mLastResultSerial = 0;
		//std::vector<ResourceEvent> mResourceEvents;

	};
}
