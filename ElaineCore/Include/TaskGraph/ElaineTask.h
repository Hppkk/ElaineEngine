#pragma once
#include "ElaineCorePrerequirements.h"
#include <string>
#include <functional>

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
		GraphTask(const TaskFunction& InFunction, NamedThread InExecutedThread = NamedThread::AnyThread,
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
	private:
		TaskUUID mTaskID;
		size_t mPriority = 0;
		std::string mTaskName;
		std::atomic<TaskState> mTaskState = TaskState::None;
		TaskDependency* mDependency = nullptr;
		TaskFunction mTaskFunction;
		NamedThread mExecutedThread = NamedThread::AnyThread;
		friend class TaskScheduler;
		friend class TaskDependency;
	};
}