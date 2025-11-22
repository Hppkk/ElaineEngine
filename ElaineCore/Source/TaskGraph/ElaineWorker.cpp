#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineWorker.h"
#include "ElaineThread.h"

namespace TaskGraph
{
    Worker::Worker(Elaine::NamedThread InThreadDesc)
        : mThreadDesc(InThreadDesc)
    {
       
    }

    Worker::~Worker()
    {

    }

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
            mTaskQueue.push(InTask);
            mConditionVariable.notify_one();
        }

        ++mTaskCount;
    }

    void Worker::Destroy()
    {
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

            if (CurrentTask->Cancelled())
                continue;

            CurrentTask->Execute();
            //TODO: Add event notice.
            CurrentTask->DispatchSubsequents();
        }
    }

    void Worker::ExecuteAllTask()
    {
        std::lock_guard<std::mutex> LockGuard(mMtx);
        while (!mTaskQueue.empty())
        {
            GraphTaskPtr CurrentTask = mTaskQueue.front();
            mTaskQueue.pop();

            if (CurrentTask->Cancelled())
                continue;

            CurrentTask->Execute();
            CurrentTask->DispatchSubsequents();
        }
    }

    GraphTaskPtr Worker::RequestTask()
    {
        std::unique_lock<std::mutex> UniqueLock(mMtx);
        mConditionVariable.wait(UniqueLock, [&]() { return !mTaskQueue.empty(); });
        GraphTaskPtr PopTask = mTaskQueue.front();
        mTaskQueue.pop();
        UniqueLock.unlock();
        --mTaskCount;
        return PopTask;
    }
}