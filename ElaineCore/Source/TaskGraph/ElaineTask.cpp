#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskGraph.h"
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
		static std::atomic<TaskUUID> TaskID{0};
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

    void GraphTask::Dependency(GraphTaskPtr Task)
    {
        if (!Task || Task.get() == this) throw std::logic_error("Invalid task dependency");
        std::lock_guard<std::mutex> Lock(mMtx);
        if (mTaskState != TaskState::None) throw std::logic_error("Dependencies must precede dispatch");
        mDependency->AddResourceEvent(Task);
    }
    void GraphTask::Subsequent(GraphTaskPtr Task)
    {
        if (!Task) return;
        Task->Dependency(shared_from_this());
        TaskGraph::instance()->Dispatch(Task);
    }
    void GraphTask::Cancel()
    {
        auto State = mTaskState.load();
        while (State == TaskState::None || State == TaskState::Pending || State == TaskState::Ready)
            if (mTaskState.compare_exchange_weak(State, TaskState::Cancelled)) return;
    }
    void GraphTask::Execute()
    {
        auto Expected = TaskState::Ready;
        if (!mTaskState.compare_exchange_strong(Expected, TaskState::Executing)) return;
        try { mTaskFunction(); mTaskState = TaskState::Succeeded; }
        catch (...) { mTaskState = TaskState::Failed; }
        mTaskFunction = {};
    }
    bool GraphTask::CanExecute()
    {
        std::lock_guard<std::mutex> Lock(mMtx);
        return mDependency->DependencyReady();
    }
    bool GraphTask::Cancelled() { return mTaskState == TaskState::Cancelled; }
    void GraphTask::DispatchSubsequents() {}
    void GraphTask::WhenAll(const std::vector<GraphTaskPtr>& Tasks)
    {
        for (auto& Task : Tasks) Dependency(Task);
        TaskGraph::instance()->Dispatch(shared_from_this());
    }
    void GraphTask::ContinueWith(GraphTaskPtr Task) { Subsequent(Task); }
    void GraphTask::ContinueWith(const TaskFunction& Function, NamedThread Thread)
    { ContinueWith(TaskGraph::instance()->CreateTask(Function, Thread)); }
}
