#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineWorker.h"
#include "ElaineThread.h"

namespace TaskGraph
{
    Worker::Worker(Elaine::NamedThread InThreadDesc)
        : mThreadDesc(InThreadDesc)
    {
       
    }

    Worker::~Worker() { Destroy(); }

    void Worker::Initialize()
    {
        mThreadHandle = Elaine::ThreadManager::instance()->GetOrCreateThread(mThreadDesc, Worker::ThreadMain, this);
        mbInitialized = true;
    }

    void Worker::EnqueueTask(GraphTaskPtr InTask)
    {
        if (InTask == nullptr)
            return;

        std::function<void()> WakeCallback;
        bool Stopping;
        {
            std::lock_guard<std::mutex> LockGuard(mMtx);
            Stopping = mStopping;
            if (!Stopping) { mTaskQueue.push(InTask); ++mTaskCount; if (InTask->GetCategory() == TaskCategory::ResourceInternal) ++mResourceTaskCount; WakeCallback = mWakeCallback; }
        }
        if (Stopping) { InTask->Cancel(); return; }
        mConditionVariable.notify_all();
        if (WakeCallback) WakeCallback();

    }

    void Worker::Destroy()
    {
        { std::lock_guard<std::mutex> Lock(mMtx); mStopping = true; }
        mConditionVariable.notify_all();
        Elaine::ThreadManager::instance()->DestroyThread(mThreadHandle);
        mThreadHandle = nullptr;
        mbInitialized = false;
        TaskPriorityQueue Abandoned;
        { std::lock_guard<std::mutex> Lock(mMtx); Abandoned.swap(mTaskQueue); mTaskCount = 0; mResourceTaskCount = 0; mWakeCallback = {}; }
        while (!Abandoned.empty()) { Abandoned.front()->Cancel(); Abandoned.pop(); }
    }

    uint32_t Worker::GetTaskCount()
    {
        return mTaskCount;
    }

    void Worker::ThreadMain(void* InThis)
    {
        Worker* This = static_cast<Worker*>(InThis);
        while (true)
        {
            GraphTaskPtr CurrentTask = This->RequestTask();

            if (!CurrentTask) break;
            if (CurrentTask->Cancelled())
                continue;

            CurrentTask->Execute();
            CurrentTask->DispatchSubsequents();
        }
    }

    void Worker::ExecuteAllTask(bool FilterCategory, TaskCategory Category)
    {
        TaskPriorityQueue Batch;
        {
            std::lock_guard<std::mutex> LockGuard(mMtx);
            TaskPriorityQueue Deferred;
            while (!mTaskQueue.empty())
            {
                auto Task = mTaskQueue.front(); mTaskQueue.pop();
                if (!FilterCategory || Task->GetCategory() == Category)
                { Batch.push(Task); if (Task->GetCategory() == TaskCategory::ResourceInternal) --mResourceTaskCount; }
                else Deferred.push(Task);
            }
            mTaskQueue.swap(Deferred);
        }
        while (!Batch.empty())
        {
            GraphTaskPtr CurrentTask = Batch.front();
            Batch.pop();
            --mTaskCount;

            if (!CurrentTask) break;
            if (CurrentTask->Cancelled())
                continue;

            CurrentTask->Execute();
            CurrentTask->DispatchSubsequents();
        }
    }

    void Worker::SetWakeCallback(std::function<void()> Callback)
    { std::lock_guard<std::mutex> Lock(mMtx); mWakeCallback = std::move(Callback); }
    void Worker::Wake()
    { std::lock_guard<std::mutex> Lock(mMtx); mConditionVariable.notify_all(); }
    void Worker::WaitForResourceTask(const TaskCompletionEventPtr& Event)
    {
        std::unique_lock<std::mutex> Lock(mMtx);
        mConditionVariable.wait(Lock, [&] {
            if (mStopping || Event->IsComplete()) return true;
            return mResourceTaskCount != 0;
        });
    }

    GraphTaskPtr Worker::RequestTask()
    {
        std::unique_lock<std::mutex> UniqueLock(mMtx);
        mConditionVariable.wait(UniqueLock, [&]() { return mStopping || !mTaskQueue.empty(); });
        if (mTaskQueue.empty()) return {};
        GraphTaskPtr PopTask = mTaskQueue.front();
        mTaskQueue.pop();
        if (PopTask->GetCategory() == TaskCategory::ResourceInternal) --mResourceTaskCount;
        UniqueLock.unlock();
        --mTaskCount;
        return PopTask;
    }
}