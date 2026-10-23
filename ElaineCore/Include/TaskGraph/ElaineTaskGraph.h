#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "TaskGraph/ElaineTask.h"

namespace TaskGraph
{
	class TaskScheduler;
	class Worker;

	struct GraphTaskDesc
	{
		void DependencyTask(GraphTaskDesc& InTask);
		void SubsequentTask(GraphTaskDesc& InTask);

		GraphTaskDesc DependencyTask(TaskFunction&& InTask, bool InAsync = true);
		void SubsequentTask(TaskFunction&& InTask, bool InAsync = true);

		TaskFunction mTaskFunction;
		Elaine::NamedThread mExecutedThread = Elaine::NamedThread::AnyThread;
		std::vector<GraphTaskDesc> mSubsequentTasks;
	};

	struct GraphTaskCreateDesc
	{
		std::vector<GraphTaskDesc> mDirectTasks;
	};

	struct ElaineCoreExport TaskDependencyGroupDesc
	{
		void Add(GraphTaskDesc&& InDesc);
		std::vector<GraphTaskDesc> mTaskDependencyList;
	};


	class ElaineCoreExport TaskGraph : public Elaine::Singleton<TaskGraph>
	{
	public:
		TaskGraph();
		~TaskGraph();
		void Initialize();

		GraphTaskPtr CreateAndDispatchWhenReady(const GraphTaskCreateDesc& InTask);
		GraphTaskPtr CreateAndDispatchWhenReady(const TaskFunction& InTask, Elaine::NamedThread InExecutedThread = Elaine::NamedThread::AnyThread);
		GraphTaskPtr CreateTask(const TaskFunction& InTask, Elaine::NamedThread InExecutedThread = Elaine::NamedThread::AnyThread);
		void Dispatch(GraphTaskPtr InTask);
		Worker* GetWorker(Elaine::NamedThread InNamedThread) const;
		Worker* FindIdleWorker();
		void ExecuteInThread(Elaine::NamedThread InNamedThread);
        void ExecuteResourceTasks();
        void ExecuteResourceCallbacks();
        void CancelTasks(TaskCategory Category);
        void WaitForResourceTasks(const TaskCompletionEventPtr& Event);
        void SetThreadWakeCallback(Elaine::NamedThread Thread, std::function<void()> Callback);
        void BeginGameThreadFrame();
        void EndGameThreadFrame() { mInGameThreadFrame = false; }
        bool IsInGameThreadFrame() const { return mInGameThreadFrame; }
	private:
		GraphTaskPtr CreateTaskRecursively(const GraphTaskDesc& InDesc);
	private:
		std::mutex mDispatchMutex;
        std::shared_ptr<TaskScheduler> mTaskScheduler;
        bool mInGameThreadFrame = false;
		//std::set<GraphTaskPtr> mTasks;
		Worker* mWorkers[(size_t)Elaine::NamedThread::ThreadCount] = { };
	};

	void ElaineCoreExport QueueTask();
}