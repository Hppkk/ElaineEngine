#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ThreadWrap;
	enum class NamedThread
	{
		RHIThread_Gfx, //Render API Command Thread
		RHIThread_Compute,
		RHIThread_Transf,
		GameThread, //Game Logic Thread
		RenderThread, //Render Logic Thread
		ResourceThread0,
		ResourceThread1,
		ResourceThread2,
		JobThread0,
		JobThread1,
		JobThread2,
		JobThread3,
		GeneralThread0,
		GeneralThread1,
		GeneralThread2,
		ThreadCount,
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
				NewThread->Initilize();
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