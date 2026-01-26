#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineResourcePtr.h"
#include "TaskGraph/ElaineTaskGraph.h"

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

	//using ResourceDependency = std::pair<std::string, ResourceType>;

	//struct ResourceEvent
	//{
	//	ResourceEvent(Name InName, ResourceType InType, TaskGraph::GraphTaskPtr InLoadTask)
	//		: mResourceName(InName)
	//		, mType(InType)
	//		, mLoadTask(InLoadTask)
	//		//, mResource(InPtr)
	//	{

	//	}

	//	Name mResourceName;
	//	ResourceType mType;
	//	TaskGraph::GraphTaskPtr mLoadTask;
	//	//ResourceBasePtr mResource;
	//};

	class ResourceBase;
	using ResourceBasePtr = ResourcePtr<ResourceBase>;
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
	public:
		enum LoadState : uint8_t
		{
			Unloaded, //未加载
			Pending,  //等待加载
			Loading,  //正在加载
			Loaded,   //加载完成
			Unloading, //卸载中
			Failed,  //失败
			Missing, //资源不存在
		};

		using LoadCompleteCallback = std::function<void(ResourceBase*)>;

	public:
		ResourceBase() = default;
		ResourceBase(ResourceManager* InManager, const std::string& InResourceName);
		virtual ~ResourceBase();
		void					LoadResource(bool async = true);
		void					UnloadResource();
		void					ReloadResource();
		void					SaveResource();
		void					ResourceArrived();
		LoadState				GetLoadState() const;
		bool					IsLoaded() const;
		const std::string&		GetPath() const { return mResourceName; }

		TaskGraph::GraphTaskPtr GetLoadTask() const { return mLoadTask; }
		void SetLoadTask(TaskGraph::GraphTaskPtr InTask) { mLoadTask = InTask; }

		//const std::vector<ResourceEvent>& GetResourceEvents() const { return mResourceEvents; }
		//void GetResourceEvents(std::vector<ResourceEvent>& OutEvents);
		//void AddResourceEvent(const ResourceEvent& InEvent);
		//void AddResourceEvent(const std::vector<ResourceEvent>& InEvents);
		//void ClearDependencies() { mResourceEvents.clear(); }

		void RegisterLoadCompleteCallback(LoadCompleteCallback InCallback);

		virtual void GetDirectDependencies(std::vector<ResourceBasePtr>& OutDependencies) const {}

	protected:
		virtual bool			LoadImpl() = 0;
		virtual	void			UnloadImpl() = 0;
		virtual void			SaveResourceImpl() = 0;
		virtual void			ResourceArrivedImpl() = 0;

		void NotifyLoadComplete();

	protected:
		std::string				mResourceName;
		ResourceManager* mOwner = nullptr;
		unsigned long			mMemoryUsage = 0;
		std::atomic<LoadState> mLoadState = Unloaded;
		TaskGraph::GraphTaskPtr mLoadTask;
		//std::vector<ResourceEvent> mResourceEvents;

		// 加载完成回调列表
		std::vector<LoadCompleteCallback> mLoadCompleteCallbacks;
		std::mutex mCallbackMutex;
		std::atomic<bool> mResourceArrived{false};  // 标记 ResourceArrived 是否已调用
	};
}