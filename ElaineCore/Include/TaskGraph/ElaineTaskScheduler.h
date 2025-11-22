#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineThreadManager.h"
#include "TaskGraph/ElaineTask.h"


namespace TaskGraph
{
	class WorkerPool;
	class GraphTask;

	class ElaineCoreExport TaskScheduler
	{
	public:
		TaskScheduler();
		~TaskScheduler();
		void QueueTask(GraphTaskPtr InTask);
	private:
		void ScheduleTask();
		static void ThreadMain(void* InThis);
		void DispatchTaskInThread(GraphTaskPtr InTask);
		GraphTaskPtr PrepareNextTask();
	private:
		std::queue<GraphTaskPtr> mReadyForDispatch;
		std::vector<GraphTaskPtr> mWaitForDispatch;
		Elaine::ThreadWrap* mThreadHandle;
		std::mutex mMtx;
		std::condition_variable mConditionVariable;
	};
}