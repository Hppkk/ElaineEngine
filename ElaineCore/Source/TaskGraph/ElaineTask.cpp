#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskDependency.h"

namespace TaskGraph
{
	static TaskUUID GenerateTaskUUID()
	{
		static TaskUUID TaskID = 0u;
		return TaskID++;
	}

	GraphTask::GraphTask(const TaskFunction& InFunction, NamedThread InExecutedThread, const std::string& InName)
		: mTaskName(InName)
		, mTaskFunction(InFunction)
		, mExecutedThread(InExecutedThread)
	{
		mTaskID = GenerateTaskUUID();
		mDependency = new TaskDependency(this);
	}

	GraphTask::~GraphTask()
	{
		SAFE_DELETE(mDependency);
	}

	void GraphTask::Dependency(GraphTaskPtr InTask)
	{
		if (InTask == nullptr)
			return;

		if (mDependency == nullptr)
		{
			mDependency = new TaskDependency(this);
		}

		mDependency->AddDependency(InTask);
	}

	void GraphTask::Subsequent(GraphTaskPtr InTask)
	{
		if (InTask == nullptr)
			return;

		if (InTask->mDependency == nullptr)
		{
			InTask->mDependency = new TaskDependency(this);
		}

		InTask->mDependency->AddDependency(this->shared_from_this());
	}

	void GraphTask::Cancel()
	{
		mTaskState = TaskState::Cancelled;
	}

	void GraphTask::Execute()
	{
		if (mTaskState != TaskState::Ready)
		{
			LOG_ERROR("TaskGraph: GraphTask is not ready.");
			return;
		}

		mTaskState = TaskState::Executing;

		mTaskFunction();

		mTaskState = TaskState::Succeeded;
	}

	bool GraphTask::CanExecute()
	{
		if (mDependency == nullptr)
			return true;

		return mDependency->IsEmpty() || mDependency->DependencyReady();
	}

	bool GraphTask::Cancelled()
	{
		return mTaskState == TaskState::Cancelled;
	}

	void GraphTask::DispatchSubsequents()
	{
		if (mDependency == nullptr)
			return;

		mDependency->DispatchSubsequents();
	}
}