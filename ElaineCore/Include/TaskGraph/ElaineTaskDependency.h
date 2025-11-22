#pragma once
#include "ElaineCorePrerequirements.h"
#include "TaskGraph/ElaineTask.h"

namespace TaskGraph
{
	class GraphTask;

	class ElaineCoreExport TaskDependency
	{
	public:
		TaskDependency(GraphTask* InTask);
		~TaskDependency();
		void AddDependency(GraphTaskPtr InTask);
		bool IsEmpty();
		bool DependencyReady();
		void DispatchSubsequents();
	private:
		std::vector<GraphTaskPtr> mDependency;
		std::vector<GraphTaskPtr> mSubsequentTasks;
		GraphTask* mTask = nullptr;
	};
}