#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineThreadManager.h"

namespace TaskGraph
{
	class TaskDependency;
	class GraphTask;

	using GraphTaskPtr = std::shared_ptr<GraphTask>;

	enum class TaskState : int
	{
		None,
		Pending,
		Ready,
		Executing,
		Succeeded,
		Failed,
		Cancelled,
	};

	using TaskUUID = size_t;
	using TaskFunction = std::function<void()>;
	using TaskPriority = size_t;

	class ElaineCoreExport GraphTask : public std::enable_shared_from_this<GraphTask>
	{
	public:
		GraphTask(const TaskFunction& InFunction, Elaine::NamedThread InExecutedThread = Elaine::NamedThread::AnyThread,
			const std::string& InName = "");
		virtual ~GraphTask();
		void Dependency(GraphTaskPtr InTask);
		void Subsequent(GraphTaskPtr InTask);
		void Cancel();
		virtual void Execute();
		TaskUUID GetTaskID() const { return mTaskID; }
		TaskState GetTaskState() const { return mTaskState; }
		bool CanExecute();
		bool Cancelled();
		void DispatchSubsequents();
		void WhenAll(const std::vector<GraphTaskPtr>& InTasks);
		void ContinueWith(GraphTaskPtr InTask);
		void ContinueWith(const TaskFunction& InFunction, Elaine::NamedThread InExecutedThread = Elaine::NamedThread::AnyThread);
	private:
		TaskUUID mTaskID;
		size_t mPriority = 0;
		std::string mTaskName;
		std::atomic<TaskState> mTaskState = TaskState::None;
		std::mutex mMtx;
		TaskDependency* mDependency = nullptr;
		TaskFunction mTaskFunction;
		Elaine::NamedThread mExecutedThread = Elaine::NamedThread::AnyThread;
		friend class TaskScheduler;
		friend class TaskDependency;
	};
}