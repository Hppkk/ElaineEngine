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

    TaskGraph::TaskGraph()
    {

    }

    TaskGraph::~TaskGraph()
    {
        for (int Index = 1; Index < (int)Elaine::NamedThread::ThreadCount; ++Index)
        {
            if (Index >= 6)
            {
                mWorkers[Index]->Destroy();
            }
            SAFE_DELETE(mWorkers[Index]);
        }
        SAFE_DELETE(mTaskScheduler);
    }

    void TaskGraph::Initialize()
    {
        mTaskScheduler = new TaskScheduler();
        for (int Index = 1; Index < (int)Elaine::NamedThread::ThreadCount; ++Index)
        {
            mWorkers[Index] = new Worker((Elaine::NamedThread)Index);
            if (Index >= 6)
            {
                mWorkers[Index]->Initialize();
            }
        }
    }

    GraphTask* TaskGraph::CreateAndDispatchWhenReady(const GraphTaskCreateDesc& InTask)
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

    GraphTask* TaskGraph::CreateAndDispatchWhenReady(const TaskFunction& InTask, NamedThread InExecutedThread)
    {
        GraphTaskPtr GraphTaskIns = std::make_shared<GraphTask>(InTask, InExecutedThread);
        //mTasks.emplace(GraphTaskIns);
        Dispatch(GraphTaskIns);
        //TODO: notify to client.
        return nullptr;
    }

    GraphTask* TaskGraph::CreateTask(const TaskFunction& InTask, NamedThread InExecutedThread)
    {
        return nullptr;
    }

    void TaskGraph::Dispatch(GraphTaskPtr InTask)
    {
        if (InTask == nullptr)
            return;

        mTaskScheduler->QueueTask(InTask);
    }

    Worker* TaskGraph::GetWorker(Elaine::NamedThread InNamedThread) const
    {
        return mWorkers[(size_t)InNamedThread];
    }

    Worker* TaskGraph::FindIdleWorker()
    {
        uint32_t MinTaskCount = UINT32_MAX;
        Worker* IdleWorker = nullptr;
        for (int Index = 0; Index < (int)Elaine::NamedThread::ThreadCount; ++Index)
        {
            if (mWorkers[Index] == nullptr)
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
        if (mWorkers[(int)InNamedThread] != nullptr)
        {
            mWorkers[(int)InNamedThread]->ExecuteAllTask();
        }
    }

    GraphTaskPtr TaskGraph::CreateTaskRecursively(const GraphTaskDesc& InDesc)
    {
        GraphTaskPtr GraphTaskIns = std::make_shared<GraphTask>(InDesc.mTaskFunction, InDesc.mExecutedThread);
        //mTasks.emplace(GraphTaskIns);
        for (auto&& SubTask : InDesc.mSubsequentTasks)
        {
            GraphTaskPtr SubTaskPtr = CreateTaskRecursively(SubTask);
            GraphTaskIns->Subsequent(SubTaskPtr);
        }
        return GraphTaskIns;
    }

}