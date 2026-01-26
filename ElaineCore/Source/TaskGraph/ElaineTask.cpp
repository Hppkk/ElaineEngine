#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskDependency.h"

namespace TaskGraph
{
	class TaskJoinCounter
	{
	public:
		TaskJoinCounter(int count, GraphTaskPtr onFinish)
			: mRemaining(count), mOnFinish(onFinish), mFired(false)
		{
		}

		void SignalOneTaskDone()
		{
			int left = --mRemaining;

			if (left == 0 && !mFired)
			{
				mFired = true;
				TaskGraph::instance()->Dispatch(mOnFinish);
			}
		}

	private:
		std::atomic<int> mRemaining;
		GraphTaskPtr mOnFinish;
		std::atomic<bool> mFired;
	};

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

		std::lock_guard<std::mutex> Lock_Guard(mMtx);

		if (mDependency == nullptr)
		{
			mDependency = new TaskDependency(this);
		}

		mDependency->AddResourceEvent(InTask);
	}

	void GraphTask::Subsequent(GraphTaskPtr InTask)
	{
		if (InTask == nullptr)
			return;

		std::lock_guard<std::mutex> Lock_Guard(mMtx);

		if (InTask->mDependency == nullptr)
		{
			InTask->mDependency = new TaskDependency(this);
		}

		InTask->mDependency->AddResourceEvent(this->shared_from_this());
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

		std::lock_guard<std::mutex> Lock_Guard(mMtx);

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

	void GraphTask::WhenAll(const std::vector<GraphTaskPtr>& InTasks)
	{
		auto JoinCounter = std::make_shared<TaskJoinCounter>((int)InTasks.size(), shared_from_this());

		for (auto& Tsk : InTasks)
		{
			Tsk->ContinueWith([JoinCounter]
			{
				JoinCounter->SignalOneTaskDone();
			});
		}
	}

	void GraphTask::ContinueWith(GraphTaskPtr InTask)
	{
		if (InTask == nullptr)
			return;

		bool CanDispatch = false;

		{
			std::lock_guard<std::mutex> Lock_Guard(mMtx);
			if (mTaskState == TaskState::Succeeded)
			{
				CanDispatch = true;
			}
			else
			{
				InTask->Dependency(shared_from_this());
			}
		}

		if (CanDispatch)
		{
			TaskGraph::instance()->Dispatch(InTask);
		}
	}

	void GraphTask::ContinueWith(const TaskFunction& InFunction, NamedThread InExecutedThread)
	{
		auto NewTask = TaskGraph::instance()->CreateTask(InFunction, InExecutedThread);
		ContinueWith(NewTask);
	}
}