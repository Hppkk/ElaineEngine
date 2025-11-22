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
		RHIThread_Gfx, //Render API Command Thread
		RHIThread_Compute,
		RHIThread_Transf,
		StreamingThread0,
		StreamingThread1,
		StreamingThread2,
		StreamingThread3,
		StreamingThread4,
		StreamingThread5,
		StreamingThread6,
		StreamingThread7,
		JobThread0,
		JobThread1,
		JobThread2,
		JobThread3,
		GeneralThread0,
		GeneralThread1,
		GeneralThread2,
		ThreadCount,
		ScheduleThread,
	};
	


	class ElaineCoreExport ThreadManager :public Singleton<ThreadManager>
	{
	public:
		ThreadManager();
		~ThreadManager();
		template <class Func, class... Args>
		ThreadWrap* GetOrCreateThread(NamedThread type, Func&& _Fx, Args&&... _Ax)
		{
			auto Iter = mThreads.find(type);
			if (Iter == mThreads.end())
			{
				auto task = std::bind(std::forward<Func>(_Fx), std::forward<Args>(_Ax)...);
				ThreadWrap* NewThread = new ThreadWrap(type, task);
				NewThread->Initialize();
				mThreads.emplace(type, NewThread);
				return NewThread;
			}
			return Iter->second;
		}

		void DestroyThread(ThreadWrap* InThread);

		const std::string&			GetStringName(NamedThread InName);
		const std::wstring&			GetWStringName(NamedThread InName);
	private:
		std::map<NamedThread, ThreadWrap*> mThreads;
		std::map<NamedThread, std::string>   mNamedStrs;
		std::map<NamedThread, std::wstring>  mNamedWStrs;
	};
}