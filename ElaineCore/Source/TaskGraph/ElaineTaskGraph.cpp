#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTaskGraph.h"
#include "TaskGraph/ElaineTaskScheduler.h"
#include "TaskGraph/ElaineWorker.h"

namespace TaskGraph
{
    void GraphTaskDesc::DependencyTask(GraphTaskDesc& InTask)
    {
        InTask.mSubsequentTasks.emplace_back(*this);
    }

    void GraphTaskDesc::SubsequentTask(GraphTaskDesc& InTask)
    {
        mSubsequentTasks.emplace_back(InTask);
    }

    GraphTaskDesc GraphTaskDesc::DependencyTask(TaskFunction&& InTask, bool InAsync)
    {
        GraphTaskDesc Desc;
        if(!InAsync)
            Desc.mExecutedThread = mExecutedThread;
        Desc.mTaskFunction = InTask;
        Desc.DependencyTask(*this);
        return Desc;
    }

    void GraphTaskDesc::SubsequentTask(TaskFunction&& InTask, bool InAsync)
    {
        GraphTaskDesc Desc;
        if (!InAsync)
            Desc.mExecutedThread = mExecutedThread;
        Desc.mTaskFunction = InTask;
        SubsequentTask(Desc);
    }

    TaskGraph::TaskGraph()
    {

    }

    TaskGraph::~TaskGraph()
    {
        std::shared_ptr<TaskScheduler> Scheduler;
        { std::lock_guard<std::mutex> Lock(mDispatchMutex); Scheduler = std::move(mTaskScheduler); mTaskScheduler = nullptr; }
        if (Scheduler) Scheduler->Stop();
        Scheduler.reset();
        for (int Index = 1; Index < (int)Elaine::NamedThread::ThreadCount; ++Index)
        {
            if (mWorkers[Index] && mWorkers[Index]->IsInitialized())
            {
                mWorkers[Index]->Destroy();
            }
            SAFE_DELETE(mWorkers[Index]);
        }
        mTaskScheduler.reset();
    }

    void TaskGraph::Initialize()
    {
        mTaskScheduler = std::make_shared<TaskScheduler>();
        for (int Index = 1; Index < (int)Elaine::NamedThread::ThreadCount; ++Index)
        {
            mWorkers[Index] = new Worker((Elaine::NamedThread)Index);
            if (Index >= (int)Elaine::NamedThread::TaskThread0 && Index <= (int)Elaine::NamedThread::TaskThread7)
            {
                mWorkers[Index]->Initialize();
            }
        }
    }

    GraphTaskPtr TaskGraph::CreateAndDispatchWhenReady(const GraphTaskCreateDesc& InTask)
    {
        std::vector<GraphTaskPtr> CreatedTasks;
        for (auto&& TaskCreateDesc : InTask.mDirectTasks)
        {
            GraphTaskPtr GraphTaskIns = CreateTaskRecursively(TaskCreateDesc);
            CreatedTasks.push_back(GraphTaskIns);
            //mTasks.emplace(GraphTaskIns);
        }
        for (auto&& CanExeTask : CreatedTasks)
        {
            Dispatch(CanExeTask);
        }
        //TODO: notify to client.
        return nullptr;
    }

    GraphTaskPtr TaskGraph::CreateAndDispatchWhenReady(const TaskFunction& InTask, Elaine::NamedThread InExecutedThread)
    {
        GraphTaskPtr GraphTaskIns = std::make_shared<GraphTask>(InTask, InExecutedThread);
        //mTasks.emplace(GraphTaskIns);
        Dispatch(GraphTaskIns);
        //TODO: notify to client.
        return GraphTaskIns;
    }

    GraphTaskPtr TaskGraph::CreateTask(const TaskFunction& InTask, Elaine::NamedThread InExecutedThread)
    {
        GraphTaskPtr GraphTaskIns = std::make_shared<GraphTask>(InTask, InExecutedThread);
        return GraphTaskIns;
    }

    void TaskGraph::Dispatch(GraphTaskPtr InTask)
    {
        if (InTask == nullptr)
            return;

        std::shared_ptr<TaskScheduler> Scheduler;
        { std::lock_guard<std::mutex> Lock(mDispatchMutex); Scheduler = mTaskScheduler; }
        if (Scheduler) Scheduler->QueueTask(InTask); else InTask->Cancel();
    }

    Worker* TaskGraph::GetWorker(Elaine::NamedThread InNamedThread) const
    {
        const auto Index = static_cast<size_t>(InNamedThread);
        return Index < static_cast<size_t>(Elaine::NamedThread::ThreadCount) ? mWorkers[Index] : nullptr;
    }

    Worker* TaskGraph::FindIdleWorker()
    {
        uint32_t MinTaskCount = UINT32_MAX;
        Worker* IdleWorker = nullptr;
        for (int Index = (int)Elaine::NamedThread::TaskThread0; Index <= (int)Elaine::NamedThread::TaskThread7; ++Index)
        {
            if (mWorkers[Index] == nullptr || !mWorkers[Index]->IsInitialized())
                continue;

            if (MinTaskCount > mWorkers[Index]->GetTaskCount())
            {
                IdleWorker = mWorkers[Index];
                MinTaskCount = mWorkers[Index]->GetTaskCount();
            }
        }

        return IdleWorker;
    }

    void TaskGraph::ExecuteInThread(Elaine::NamedThread InNamedThread)
    {
        if (auto* Target = GetWorker(InNamedThread)) Target->ExecuteAllTask();
    }

    void TaskGraph::CancelTasks(TaskCategory Category)
    {
        std::shared_ptr<TaskScheduler> Scheduler;
        { std::lock_guard<std::mutex> Lock(mDispatchMutex); Scheduler = mTaskScheduler; }
        if (Scheduler) Scheduler->CancelCategory(Category);
    }
    void TaskGraph::ExecuteResourceTasks()
    { GetWorker(Elaine::NamedThread::GameThread)->ExecuteAllTask(true); }
    void TaskGraph::ExecuteResourceCallbacks()
    { GetWorker(Elaine::NamedThread::GameThread)->ExecuteAllTask(true, TaskCategory::ResourceCallback); }
    void TaskGraph::WaitForResourceTasks(const TaskCompletionEventPtr& Event)
    { GetWorker(Elaine::NamedThread::GameThread)->WaitForResourceTask(Event); }
    void TaskGraph::SetThreadWakeCallback(Elaine::NamedThread Thread, std::function<void()> Callback)
    { GetWorker(Thread)->SetWakeCallback(std::move(Callback)); }
    void TaskGraph::BeginGameThreadFrame()
    {
        if (!Elaine::ThreadManager::instance()->CheckThread(Elaine::NamedThread::GameThread))
            throw std::logic_error("Frame entry requires GameThread");
        if (mInGameThreadFrame) return;
        mInGameThreadFrame = true;
        ExecuteInThread(Elaine::NamedThread::GameThread);
    }

    GraphTaskPtr TaskGraph::CreateTaskRecursively(const GraphTaskDesc& InDesc)
    {
        GraphTaskPtr GraphTaskIns = std::make_shared<GraphTask>(InDesc.mTaskFunction, InDesc.mExecutedThread);
        //mTasks.emplace(GraphTaskIns);
        for (auto&& SubTask : InDesc.mSubsequentTasks)
        {
            GraphTaskPtr SubTaskPtr = CreateTaskRecursively(SubTask);
            SubTaskPtr->Dependency(GraphTaskIns);
            Dispatch(SubTaskPtr);
        }
        return GraphTaskIns;
    }

}