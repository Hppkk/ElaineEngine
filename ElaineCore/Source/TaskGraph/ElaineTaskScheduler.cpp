#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTaskScheduler.h"
#include "TaskGraph/ElaineTask.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineWorker.h"

namespace TaskGraph
{
    TaskScheduler::TaskScheduler()
    {
        mThreadHandle = Elaine::ThreadManager::instance()->GetOrCreateThread(Elaine::NamedThread::ScheduleThread,
            TaskScheduler::ThreadMain, this);
    }

    TaskScheduler::~TaskScheduler()
    {
        mStopping = true; mConditionVariable.notify_all();
        Elaine::ThreadManager::instance()->DestroyThread(mThreadHandle);
        for (auto& Task : mWaitForDispatch) Task->Cancel();
    }

    void TaskScheduler::QueueTask(GraphTaskPtr InTask)
    {
        std::lock_guard<std::mutex> LockGuard(mMtx);
        if (mStopping) { InTask->Cancel(); return; }
        auto Expected = TaskState::None;
        if (!InTask->mTaskState.compare_exchange_strong(Expected, TaskState::Pending)) return;
        mWaitForDispatch.push_back(InTask);
        mConditionVariable.notify_one();
    }

    void TaskScheduler::ScheduleTask()
    {
        std::unique_lock<std::mutex> UniqueLock(mMtx);
        mConditionVariable.wait_for(UniqueLock, std::chrono::milliseconds(2), [&]() { return mStopping || !mWaitForDispatch.empty(); });
        if (mStopping) return;
        for (auto&& Iter = mWaitForDispatch.begin(); Iter!= mWaitForDispatch.end();)
        {
            if ((*Iter)->Cancelled()) { Iter = mWaitForDispatch.erase(Iter); continue; }
            if ((*Iter)->CanExecute())
            {
                auto Expected = TaskState::Pending;
                if ((*Iter)->mTaskState.compare_exchange_strong(Expected, TaskState::Ready)) mReadyForDispatch.push((*Iter));
                Iter = mWaitForDispatch.erase(Iter);
            }
            else
            {
                ++Iter;
            }
        }

        UniqueLock.unlock();
        while (!mReadyForDispatch.empty())
        {
            GraphTaskPtr CurrentTask = mReadyForDispatch.front();
            mReadyForDispatch.pop();
            DispatchTaskInThread(CurrentTask);
        }
    }

    void TaskScheduler::ThreadMain(void* InThis)
    {
        TaskScheduler* This = static_cast<TaskScheduler*>(InThis);
        while (!This->mStopping)
        {
            {
                This->ScheduleTask();
            }
        }
    }

    void TaskScheduler::DispatchTaskInThread(GraphTaskPtr InTask)
    {
        Worker* SuitableWorker = nullptr;
        if (InTask->mExecutedThread == Elaine::NamedThread::AnyThread)
        {
            SuitableWorker = TaskGraph::instance()->FindIdleWorker();
        }
        else
        {
            SuitableWorker = TaskGraph::instance()->GetWorker(InTask->mExecutedThread);
        }

        if (SuitableWorker == nullptr)
        {
            LOG_ERROR("TaskGraph find no worker.");
            InTask->Cancel(); return;
        }

        SuitableWorker->EnqueueTask(InTask);
    }

    GraphTaskPtr TaskScheduler::PrepareNextTask()
    {
        GraphTaskPtr ResultTask = nullptr;
        while (!mReadyForDispatch.empty())
        {
            ResultTask = mReadyForDispatch.front();
            mReadyForDispatch.pop();
        }

        return ResultTask;
    }
}