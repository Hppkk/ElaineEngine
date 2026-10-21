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

        {
            std::lock_guard<std::mutex> LockGuard(mMtx);
            if (mStopping) { InTask->Cancel(); return; }
            mTaskQueue.push(InTask);
            ++mTaskCount;
            mConditionVariable.notify_one();
        }

    }

    void Worker::Destroy()
    {
        { std::lock_guard<std::mutex> Lock(mMtx); mStopping = true; }
        mConditionVariable.notify_all();
        Elaine::ThreadManager::instance()->DestroyThread(mThreadHandle);
        mThreadHandle = nullptr;
        mbInitialized = false;
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
            //TODO: Add event notice.
            CurrentTask->DispatchSubsequents();
        }
    }

    void Worker::ExecuteAllTask()
    {
        TaskPriorityQueue Batch;
        { std::lock_guard<std::mutex> LockGuard(mMtx); Batch.swap(mTaskQueue); }
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

    GraphTaskPtr Worker::RequestTask()
    {
        std::unique_lock<std::mutex> UniqueLock(mMtx);
        mConditionVariable.wait(UniqueLock, [&]() { return mStopping || !mTaskQueue.empty(); });
        if (mTaskQueue.empty()) return {};
        GraphTaskPtr PopTask = mTaskQueue.front();
        mTaskQueue.pop();
        UniqueLock.unlock();
        --mTaskCount;
        return PopTask;
    }
}