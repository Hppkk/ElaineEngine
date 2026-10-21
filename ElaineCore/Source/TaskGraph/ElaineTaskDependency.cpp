#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTaskDependency.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace TaskGraph
{
	TaskDependency::TaskDependency(GraphTask* InTask)
	{
		mTask = InTask;
	}
	TaskDependency::~TaskDependency()
	{

	}

	void TaskDependency::AddResourceEvent(GraphTaskPtr InTask)
	{
		mDependency.push_back(InTask);

	}

	bool TaskDependency::IsEmpty()
	{
		return mDependency.empty();
	}

	bool TaskDependency::DependencyReady()
	{
		for (auto&& DependencyTask : mDependency)
		{
			const auto State = DependencyTask->GetTaskState();
            if (State == TaskState::Failed || State == TaskState::Cancelled) { mTask->Cancel(); return false; }
            if (State != TaskState::Succeeded) return false;
		}

		return true;
	}

	void TaskDependency::DispatchSubsequents()
	{
		for (auto&& Iter = mSubsequentTasks.begin(); Iter != mSubsequentTasks.end();)
		{
			GraphTaskPtr& SubsequentTask = *Iter;
			if (SubsequentTask->CanExecute())
			{
				TaskGraph::instance()->Dispatch(SubsequentTask);
				Iter = mSubsequentTasks.erase(Iter);
			}
			else
			{
				++Iter;
			}
		}
	}
}
