#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineRenderProxy.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
	RenderProxy::RenderProxy()
	{
	}

	void RenderProxy::TrackResource(ResourceBasePtr InResource)
	{
		if (InResource.isNull())
			return;

		TrackResourceRecursive(InResource);
	}

	void RenderProxy::TrackResourceRecursive(ResourceBasePtr InResource)
	{
		if (InResource.isNull())
			return;

		ResourceBase* RawPtr = InResource.get();

		{
			std::lock_guard<std::mutex> Lock(mTrackMutex);

			// 检查是否已追踪
			if (mTrackedResources.find(RawPtr) != mTrackedResources.end())
				return;

			mTrackedResources.insert(RawPtr);
		}

		// 增加待完成计数
		mPendingResourceCount.fetch_add(1);

		// 注册加载完成回调
		RenderProxy* Self = this;
		ResourceBasePtr HoldRef = InResource;  // 防止资源在回调前被释放

		InResource->RegisterLoadCompleteCallback(
			[Self, HoldRef](ResourceBase* LoadedResource)
			{
				Self->OnResourceLoaded(LoadedResource);
			}
		);
	}

	void RenderProxy::OnResourceLoaded(ResourceBase* InResource)
	{
		// 当资源加载完成后，追踪其子依赖
		std::vector<ResourceBasePtr> Dependencies;
		InResource->GetDirectDependencies(Dependencies);

		for (auto& Dep : Dependencies)
		{
			TrackResourceRecursive(Dep);
		}

		// 递减待完成计数
		int32_t Remaining = mPendingResourceCount.fetch_sub(1) - 1;

		// 如果所有资源已加载完成且已开始初始化流程，触发初始化
		if (Remaining == 0 && mInitializationStarted.load())
		{
			TryInitializeBindings();
		}
	}

	void RenderProxy::BeginInitialization()
	{
		mInitializationStarted.store(true);

		// 如果所有资源已加载完成，立即初始化
		if (mPendingResourceCount.load() == 0)
		{
			TryInitializeBindings();
		}
	}

	void RenderProxy::TryInitializeBindings()
	{
		// 使用 exchange 确保只初始化一次
		if (mBindingsInitialized)
			return;

		// 在渲染线程执行初始化
		RenderProxy* Self = this;
		auto InitTask = TaskGraph::TaskGraph::instance()->CreateTask([Self]()
		{
			try
			{
				Self->InitializeResourceBinding();
				Self->OnResourceInitialized();
				Self->mBindingsInitialized = true;
			}
			catch (...)
			{
				Self->OnResourceFailed();
			}
		}, NamedThread::RenderThread);

		TaskGraph::TaskGraph::instance()->Dispatch(InitTask);
	}
}