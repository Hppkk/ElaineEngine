#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTaskScheduler.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineWorker.h"
namespace TaskGraph
{
    TaskScheduler::TaskScheduler()
    { mThreadHandle = Elaine::ThreadManager::instance()->GetOrCreateThread(Elaine::NamedThread::ScheduleThread, ThreadMain, this); }
    TaskScheduler::~TaskScheduler() { Stop(); }
    void TaskScheduler::Stop()
    {
        std::map<TaskUUID, GraphTaskPtr> Active;
        { std::lock_guard<std::mutex> Lock(mMtx); mStopping = true; Active.swap(mActive); }
        mConditionVariable.notify_all();
        Elaine::ThreadManager::instance()->DestroyThread(mThreadHandle); mThreadHandle = nullptr;
        for (auto& Entry : Active) Entry.second->Cancel();
    }
    void TaskScheduler::CancelCategory(TaskCategory Category)
    {
        std::vector<GraphTaskPtr> Tasks;
        { std::lock_guard<std::mutex> Lock(mMtx); for (auto& Entry : mActive)
              if (Entry.second->GetCategory() == Category) Tasks.push_back(Entry.second); }
        for (auto& Task : Tasks) Task->Cancel();
    }
    void TaskScheduler::QueueTask(GraphTaskPtr Task)
    {
        bool Stopping;
        { std::lock_guard<std::mutex> Lock(mMtx); Stopping = mStopping;
          if (!Stopping && !mActive.emplace(Task->GetTaskID(), Task).second) return; }
        if (Stopping) { Task->Cancel(); return; }
        std::weak_ptr<TaskScheduler> Weak = shared_from_this();
        Task->GetCompletionEvent()->Subscribe([Weak, ID = Task->GetTaskID()](TaskState) {
            if (auto Scheduler = Weak.lock()) { std::lock_guard<std::mutex> Lock(Scheduler->mMtx); Scheduler->mActive.erase(ID); }
        });
        Task->Arm(shared_from_this());
    }
    void TaskScheduler::EnqueueReady(GraphTaskPtr Task)
    {
        bool Stopping;
        { std::lock_guard<std::mutex> Lock(mMtx); Stopping = mStopping; if (!Stopping) mReadyForDispatch.push(Task); }
        if (Stopping) Task->Cancel(); else mConditionVariable.notify_one();
    }
    void TaskScheduler::ThreadMain(void* Self) { static_cast<TaskScheduler*>(Self)->ScheduleTask(); }
    void TaskScheduler::ScheduleTask()
    {
        for (;;)
        {
            GraphTaskPtr Task;
            { std::unique_lock<std::mutex> Lock(mMtx);
              mConditionVariable.wait(Lock, [&] { return mStopping || !mReadyForDispatch.empty(); });
              if (mStopping) return;
              Task = mReadyForDispatch.front(); mReadyForDispatch.pop(); }
            DispatchTaskInThread(Task);
        }
    }
    void TaskScheduler::DispatchTaskInThread(GraphTaskPtr Task)
    {
        auto* Graph = TaskGraph::instance();
        auto* Target = Task->mExecutedThread == Elaine::NamedThread::AnyThread ? Graph->FindIdleWorker() : Graph->GetWorker(Task->mExecutedThread);
        if (Target) Target->EnqueueTask(Task); else Task->Cancel();
    }
}
