#pragma once
#include "ElaineCorePrerequirements.h"
#include "TaskGraph/ElaineTask.h"
#include "ElaineThreadManager.h"


namespace TaskGraph
{
	class WorkerPool;
	//todo lock-free queue
	using TaskPriorityQueue = std::queue<GraphTaskPtr>;

	class ElaineCoreExport Worker
	{
	public:
		Worker(Elaine::NamedThread InThreadDesc);
		~Worker();
		void Initialize();
		void EnqueueTask(GraphTaskPtr InTask);
		void Destroy();
		uint32_t GetTaskCount();
		static void ThreadMain(void* InThis);
		void ExecuteAllTask();
		bool IsInitialized() const { return mbInitialized; }
	private:
		GraphTaskPtr RequestTask();

	private:
		//WorkerPool* mOwner;
		Elaine::ThreadWrap* mThreadHandle = nullptr;
		TaskPriorityQueue mTaskQueue;
		std::mutex mMtx;
		std::condition_variable mConditionVariable;
		std::atomic_uint mTaskCount = 0;
		Elaine::NamedThread mThreadDesc;
		bool mbInitialized = false;
	};
}