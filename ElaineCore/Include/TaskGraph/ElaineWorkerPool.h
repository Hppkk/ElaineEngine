#pragma once
#include "ElaineCorePrerequirements.h"

namespace TaskGraph
{
	class TaskQueue;
	class Worker;

	class ElaineCoreExport WorkerPool
	{
	public:
		WorkerPool(size_t InWorkerSize);
		~WorkerPool();

	private:
		TaskQueue* mTaskQueue = nullptr;
		size_t mWorkerSize = 0u;
		std::vector<Worker*> mWorkers;
	};
}