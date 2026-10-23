#pragma once
#include "TaskGraph/ElaineTask.h"
namespace TaskGraph
{
    class ElaineCoreExport TaskScheduler : public std::enable_shared_from_this<TaskScheduler>
    {
    public:
        TaskScheduler();
        ~TaskScheduler();
        void QueueTask(GraphTaskPtr Task);
        void EnqueueReady(GraphTaskPtr Task);
        void Stop();
        void CancelCategory(TaskCategory Category);
    private:
        static void ThreadMain(void* Self);
        void ScheduleTask();
        void DispatchTaskInThread(GraphTaskPtr Task);
        std::queue<GraphTaskPtr> mReadyForDispatch;
        // Ownership only; never scanned to advance dependencies.
        std::map<TaskUUID, GraphTaskPtr> mActive;
        Elaine::ThreadWrap* mThreadHandle = nullptr;
        bool mStopping = false;
        std::mutex mMtx;
        std::condition_variable mConditionVariable;
    };
}
