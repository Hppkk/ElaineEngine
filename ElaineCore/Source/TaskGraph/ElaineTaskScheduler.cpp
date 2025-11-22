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

    }

    void TaskScheduler::QueueTask(GraphTaskPtr InTask)
    {
        std::lock_guard<std::mutex> LockGuard(mMtx);
        mWaitForDispatch.push_back(InTask);
        mConditionVariable.notify_one();
        InTask->mTaskState = TaskState::Pending;
    }

    void TaskScheduler::ScheduleTask()
    {
        std::unique_lock<std::mutex> UniqueLock(mMtx);
        mConditionVariable.wait(UniqueLock, [&]() { return !mWaitForDispatch.empty() || !mReadyForDispatch.empty(); });
        for (auto&& Iter = mWaitForDispatch.begin(); Iter!= mWaitForDispatch.end();)
        {
            if ((*Iter)->CanExecute())
            {
                mReadyForDispatch.push((*Iter));
                (*Iter)->mTaskState = TaskState::Ready;
                Iter = mWaitForDispatch.erase(Iter);
            }
            else
            {
                ++Iter;
            }
        }

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
        while (true)
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