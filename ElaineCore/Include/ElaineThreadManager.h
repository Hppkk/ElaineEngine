#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ThreadWrap;
	enum class NamedThread : uint8_t
	{
		AnyThread, //Any thread, indicating that tasks in the TaskGraph can be executed on any thread.
		GameThread, //Game Logic Thread. Usually is main threrd.
		RenderThread, //Render Logic Thread
		RHIGraphicThread, //Render API Command Thread
		RHIComputeThread,
		RHITransferThread,
		StreamingThread0,
		StreamingThread1,
		StreamingThread2,
		StreamingThread3,
		StreamingThread4,
		StreamingThread5,
		StreamingThread6,
		StreamingThread7,
		GeneralThread0,
		GeneralThread1,
		GeneralThread2,
		ThreadCount,
		ScheduleThread, //Use for TaskGraph schedule tacks.
	};



	class ElaineCoreExport ThreadManager :public Singleton<ThreadManager>
	{
	public:
		ThreadManager();
		~ThreadManager();
		template <class Func, class... Args>
		ThreadWrap* GetOrCreateThread(NamedThread InType, Func&& _Fx, Args&&... _Ax)
		{
			auto Iter = mThreads.find(InType);
			if (Iter == mThreads.end())
			{
				auto task = std::bind(std::forward<Func>(_Fx), std::forward<Args>(_Ax)...);
				ThreadWrap* NewThread = new ThreadWrap(InType, [task, InType]()
				{
					ThreadManager::instance()->InitilizeThread(InType);
					task();
				});
				NewThread->Initialize();
				mThreads.emplace(InType, NewThread);
				return NewThread;
			}
			return Iter->second;
		}

		void DestroyThread(ThreadWrap* InThread);

		bool CheckThread(NamedThread InName);
		void InitilizeThread(NamedThread InType);

		const std::string&			GetStringName(NamedThread InName);
		const std::wstring&			GetWStringName(NamedThread InName);
	private:
		std::map<NamedThread, ThreadWrap*> mThreads;
		std::map<NamedThread, std::string>   mNamedStrs;
		std::map<NamedThread, std::wstring>  mNamedWStrs;
	};
}