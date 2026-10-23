#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineTaskScheduler.h"
namespace TaskGraph
{
    static bool Terminal(TaskState State)
    { return State == TaskState::Succeeded || State == TaskState::Failed || State == TaskState::Cancelled; }
    bool TaskCompletionEvent::Complete(TaskState State, std::exception_ptr Error)
    {
        std::vector<Listener> Listeners;
        { std::lock_guard<std::mutex> Lock(mMutex); if (Terminal(mState)) return false;
          mState = State; mException = Error; Listeners.swap(mListeners); }
        mCondition.notify_all();
        for (auto& Callback : Listeners) { try { Callback(State); } catch (...) { LOG_ERROR("Task completion listener threw"); } }
        return true;
    }
    bool TaskCompletionEvent::Succeed() { return Complete(TaskState::Succeeded); }
    bool TaskCompletionEvent::Fail(std::exception_ptr Error) { return Complete(TaskState::Failed, Error); }
    bool TaskCompletionEvent::Cancel() { return Complete(TaskState::Cancelled); }
    TaskState TaskCompletionEvent::GetState() const { std::lock_guard<std::mutex> Lock(mMutex); return mState; }
    bool TaskCompletionEvent::IsComplete() const { return Terminal(GetState()); }
    std::exception_ptr TaskCompletionEvent::GetException() const { std::lock_guard<std::mutex> Lock(mMutex); return mException; }
    void TaskCompletionEvent::RethrowIfFailed() const
    {
        if (auto Error = GetException()) std::rethrow_exception(Error);
        if (GetState() != TaskState::Succeeded) throw std::runtime_error("Task cancelled or incomplete");
    }
    void TaskCompletionEvent::Subscribe(Listener Callback)
    {
        TaskState State;
        { std::lock_guard<std::mutex> Lock(mMutex); State = mState;
          if (!Terminal(State)) { mListeners.push_back(std::move(Callback)); return; } }
        Callback(State);
    }
    void TaskCompletionEvent::Wait()
    { std::unique_lock<std::mutex> Lock(mMutex); mCondition.wait(Lock, [&] { return Terminal(mState); }); }
    GraphTask::GraphTask(const TaskFunction& Function, Elaine::NamedThread Thread, const std::string& Name)
        : mTaskFunction(Function), mExecutedThread(Thread)
    { static std::atomic<TaskUUID> Next{0}; mTaskID = Next++; }
    void GraphTask::Dependency(GraphTaskPtr Task)
    { if (!Task || Task.get() == this) throw std::logic_error("Invalid task dependency"); Dependency(Task->GetCompletionEvent()); }
    void GraphTask::Dependency(TaskCompletionEventPtr Event)
    {
        std::lock_guard<std::mutex> Lock(mMtx);
        if (!Event || Event == mCompletion || mTaskState != TaskState::None) throw std::logic_error("Dependencies must precede dispatch");
        mDependencies.push_back(std::move(Event));
    }
    void GraphTask::SetContinuationPolicy(TaskContinuationPolicy Policy)
    { std::lock_guard<std::mutex> Lock(mMtx); if (mTaskState != TaskState::None) throw std::logic_error("Task already dispatched"); mPolicy = Policy; }
    void GraphTask::SetCategory(TaskCategory Category)
    { std::lock_guard<std::mutex> Lock(mMtx); if (mTaskState != TaskState::None) throw std::logic_error("Task already dispatched"); mCategory = Category; }
    void GraphTask::Arm(const std::shared_ptr<TaskScheduler>& Scheduler)
    {
        std::vector<TaskCompletionEventPtr> Dependencies;
        { std::lock_guard<std::mutex> Lock(mMtx); if (mTaskState != TaskState::None) return;
          mTaskState = TaskState::Pending; mScheduler = Scheduler; Dependencies.swap(mDependencies);
          // Sentinel prevents completed prerequisites dispatching during registration.
          mRemaining = Dependencies.size() + 1; }
        auto Weak = weak_from_this();
        for (auto& Event : Dependencies)
            Event->Subscribe([Weak](TaskState State) { if (auto Task = Weak.lock()) Task->DependencyFinished(State); });
        DependencyFinished(TaskState::Succeeded);
    }
    void GraphTask::DependencyFinished(TaskState State)
    {
        if (State != TaskState::Succeeded) mDependencyFailed = true;
        if (mRemaining.fetch_sub(1) != 1) return;
        if (mDependencyFailed && mPolicy == TaskContinuationPolicy::OnSuccess) { Cancel(); return; }
        auto Expected = TaskState::Pending;
        if (!mTaskState.compare_exchange_strong(Expected, TaskState::Ready)) return;
        if (auto Scheduler = mScheduler.lock()) Scheduler->EnqueueReady(shared_from_this()); else Cancel();
    }
    void GraphTask::Cancel()
    {
        { std::lock_guard<std::mutex> Lock(mMtx); auto State = mTaskState.load(); if (Terminal(State)) return;
          mCancelRequested = true;
          if (State == TaskState::Executing) return;
          mTaskState = TaskState::Cancelled; mTaskFunction = {}; }
        mCompletion->Cancel();
    }
    void GraphTask::Finish(TaskState State, std::exception_ptr Error)
    {
        { std::lock_guard<std::mutex> Lock(mMtx); mTaskFunction = {}; mTaskState = mCancelRequested ? TaskState::Cancelled : State; State = mTaskState; }
        if (State == TaskState::Succeeded) mCompletion->Succeed();
        else if (State == TaskState::Cancelled) mCompletion->Cancel(); else mCompletion->Fail(Error);
    }
    void GraphTask::Execute()
    {
        { std::lock_guard<std::mutex> Lock(mMtx); if (mTaskState != TaskState::Ready) return; mTaskState = TaskState::Executing; }
        try { mTaskFunction(); Finish(TaskState::Succeeded); } catch (...) { Finish(TaskState::Failed, std::current_exception()); }
    }
    void GraphTask::Subsequent(GraphTaskPtr Task)
    { if (Task) { Task->Dependency(shared_from_this()); TaskGraph::instance()->Dispatch(Task); } }
    void GraphTask::WhenAll(const std::vector<GraphTaskPtr>& Tasks)
    { for (auto& Task : Tasks) Dependency(Task); TaskGraph::instance()->Dispatch(shared_from_this()); }
    void GraphTask::ContinueWith(GraphTaskPtr Task) { Subsequent(Task); }
    void GraphTask::ContinueWith(const TaskFunction& Function, Elaine::NamedThread Thread)
    { ContinueWith(TaskGraph::instance()->CreateTask(Function, Thread)); }
}
