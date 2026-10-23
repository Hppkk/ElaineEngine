#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineThreadManager.h"
namespace TaskGraph
{
    class GraphTask;
    class TaskScheduler;
    using GraphTaskPtr = std::shared_ptr<GraphTask>;
    enum class TaskState : int { None, Pending, Ready, Executing, Succeeded, Failed, Cancelled };
    enum class TaskContinuationPolicy { OnSuccess, Always };
    enum class TaskCategory { General, ResourceInternal, ResourceCallback };
    using TaskUUID = size_t;
    using TaskFunction = std::function<void()>;
    using TaskPriority = size_t;
    // An external event occupies no worker. Listeners run outside its lock.
    class ElaineCoreExport TaskCompletionEvent
    {
    public:
        using Listener = std::function<void(TaskState)>;
        bool Succeed();
        bool Fail(std::exception_ptr Error);
        bool Cancel();
        TaskState GetState() const;
        bool IsComplete() const;
        std::exception_ptr GetException() const;
        void RethrowIfFailed() const;
        void Subscribe(Listener Callback);
        void Wait();
    private:
        bool Complete(TaskState State, std::exception_ptr Error = {});
        mutable std::mutex mMutex;
        std::condition_variable mCondition;
        TaskState mState = TaskState::Pending;
        std::exception_ptr mException;
        std::vector<Listener> mListeners;
    };
    using TaskCompletionEventPtr = std::shared_ptr<TaskCompletionEvent>;
    class ElaineCoreExport GraphTask : public std::enable_shared_from_this<GraphTask>
    {
    public:
        GraphTask(const TaskFunction& Function, Elaine::NamedThread Thread = Elaine::NamedThread::AnyThread, const std::string& Name = "");
        virtual ~GraphTask() = default;
        void Dependency(GraphTaskPtr Task);
        void Dependency(TaskCompletionEventPtr Event);
        void Subsequent(GraphTaskPtr Task);
        void Cancel();
        virtual void Execute();
        TaskUUID GetTaskID() const { return mTaskID; }
        TaskState GetTaskState() const { return mTaskState.load(); }
        TaskCompletionEventPtr GetCompletionEvent() const { return mCompletion; }
        bool CanExecute() { return mTaskState == TaskState::Ready; }
        bool Cancelled() { return mTaskState == TaskState::Cancelled; }
        void DispatchSubsequents() {} // Execute/Cancel signal completion exactly once.
        void WhenAll(const std::vector<GraphTaskPtr>& Tasks);
        void ContinueWith(GraphTaskPtr Task);
        void ContinueWith(const TaskFunction& Function, Elaine::NamedThread Thread = Elaine::NamedThread::AnyThread);
        void SetContinuationPolicy(TaskContinuationPolicy Policy);
        void SetCategory(TaskCategory Category);
        TaskCategory GetCategory() const { return mCategory; }
    private:
        void Arm(const std::shared_ptr<TaskScheduler>& Scheduler);
        void DependencyFinished(TaskState State);
        void Finish(TaskState State, std::exception_ptr Error = {});
        TaskUUID mTaskID;
        std::atomic<TaskState> mTaskState{TaskState::None};
        std::atomic_bool mCancelRequested{false};
        std::mutex mMtx;
        TaskFunction mTaskFunction;
        Elaine::NamedThread mExecutedThread;
        TaskCategory mCategory = TaskCategory::General;
        TaskContinuationPolicy mPolicy = TaskContinuationPolicy::OnSuccess;
        std::vector<TaskCompletionEventPtr> mDependencies;
        TaskCompletionEventPtr mCompletion = std::make_shared<TaskCompletionEvent>();
        std::weak_ptr<TaskScheduler> mScheduler;
        std::atomic_size_t mRemaining{0};
        std::atomic_bool mDependencyFailed{false};
        friend class TaskScheduler;
    };
}
