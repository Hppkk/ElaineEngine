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

	void TaskDependency::AddDependency(GraphTaskPtr InTask)
	{
		mDependency.push_back(InTask);
		InTask->mDependency->mSubsequentTasks.push_back(mTask->shared_from_this());
	}

	bool TaskDependency::IsEmpty()
	{
		return mDependency.empty();
	}

	bool TaskDependency::DependencyReady()
	{
		for (auto&& DependencyTask : mDependency)
		{
			if (DependencyTask->GetTaskState() != TaskState::Succeeded)
				return false;
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