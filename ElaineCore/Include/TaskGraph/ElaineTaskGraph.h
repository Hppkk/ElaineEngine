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


	/* TaskGraph
		Experimental testing, further improvements are needed for the scheduling notification mechanism.
		TODO: 
			1. Event notification mechanism.
			2. Convenient creation mechanism.
			3. Optimize thread locks.
			4. A mechanism to notify the client whether the current task chain has been executed.
			5. More convenient to create interfaces.
	*/
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
	private:
		GraphTaskPtr CreateTaskRecursively(const GraphTaskDesc& InDesc);
	private:
		TaskScheduler* mTaskScheduler = nullptr;
		//std::set<GraphTaskPtr> mTasks;
		Worker* mWorkers[(size_t)Elaine::NamedThread::ThreadCount] = { };
	};

	void ElaineCoreExport QueueTask();
}