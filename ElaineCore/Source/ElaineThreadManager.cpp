#include "ElainePrecompiledHeader.h"
#include "ElaineThreadManager.h"

namespace Elaine
{
	ThreadManager::ThreadManager()
	{
		//int concurrency = std::thread::hardware_concurrency();
		//for (int i = 0; i < concurrency; ++i)
		//{
		//	m_threadPool.insert(new ThreadWrap());
		//}

		mNamedStrs.emplace(NamedThread::GameThread, STR_NAME(GameThread));
		mNamedStrs.emplace(NamedThread::GeneralThread0, STR_NAME(GeneralThread0));
		mNamedStrs.emplace(NamedThread::GeneralThread1, STR_NAME(GeneralThread1));
		mNamedStrs.emplace(NamedThread::GeneralThread2, STR_NAME(GeneralThread2));
		mNamedStrs.emplace(NamedThread::JobThread0, STR_NAME(JobThread0));
		mNamedStrs.emplace(NamedThread::JobThread1, STR_NAME(JobThread1));
		mNamedStrs.emplace(NamedThread::JobThread2, STR_NAME(JobThread2));
		mNamedStrs.emplace(NamedThread::RenderThread, STR_NAME(RenderThread));
		mNamedStrs.emplace(NamedThread::StreamingThread0, STR_NAME(StreamingThread0));
		mNamedStrs.emplace(NamedThread::StreamingThread1, STR_NAME(StreamingThread1));
		mNamedStrs.emplace(NamedThread::StreamingThread2, STR_NAME(StreamingThread2));
		mNamedStrs.emplace(NamedThread::StreamingThread3, STR_NAME(StreamingThread3));
		mNamedStrs.emplace(NamedThread::StreamingThread4, STR_NAME(StreamingThread4));
		mNamedStrs.emplace(NamedThread::StreamingThread5, STR_NAME(StreamingThread5));
		mNamedStrs.emplace(NamedThread::StreamingThread6, STR_NAME(StreamingThread6));
		mNamedStrs.emplace(NamedThread::StreamingThread7, STR_NAME(StreamingThread7));
		mNamedStrs.emplace(NamedThread::RHIThread_Gfx, STR_NAME(RHIThread_Gfx));
		mNamedStrs.emplace(NamedThread::RHIThread_Compute, STR_NAME(RHIThread_Compute));
		mNamedStrs.emplace(NamedThread::RHIThread_Transf, STR_NAME(RHIThread_Transf));
		mNamedStrs.emplace(NamedThread::ScheduleThread, STR_NAME(ScheduleThread));


		mNamedWStrs.emplace(NamedThread::GameThread, WSTR_NAME(GameThread));
		mNamedWStrs.emplace(NamedThread::GeneralThread0, WSTR_NAME(GeneralThread0));
		mNamedWStrs.emplace(NamedThread::GeneralThread1, WSTR_NAME(GeneralThread1));
		mNamedWStrs.emplace(NamedThread::GeneralThread2, WSTR_NAME(GeneralThread2));
		mNamedWStrs.emplace(NamedThread::JobThread0, WSTR_NAME(JobThread0));
		mNamedWStrs.emplace(NamedThread::JobThread1, WSTR_NAME(JobThread1));
		mNamedWStrs.emplace(NamedThread::JobThread2, WSTR_NAME(JobThread2));
		mNamedWStrs.emplace(NamedThread::RenderThread, WSTR_NAME(RenderThread));
		mNamedWStrs.emplace(NamedThread::StreamingThread0, WSTR_NAME(StreamingThread0));
		mNamedWStrs.emplace(NamedThread::StreamingThread1, WSTR_NAME(StreamingThread1));
		mNamedWStrs.emplace(NamedThread::StreamingThread2, WSTR_NAME(StreamingThread2));
		mNamedWStrs.emplace(NamedThread::StreamingThread3, WSTR_NAME(StreamingThread3));
		mNamedWStrs.emplace(NamedThread::StreamingThread4, WSTR_NAME(StreamingThread4));
		mNamedWStrs.emplace(NamedThread::StreamingThread5, WSTR_NAME(StreamingThread5));
		mNamedWStrs.emplace(NamedThread::StreamingThread6, WSTR_NAME(StreamingThread6));
		mNamedWStrs.emplace(NamedThread::StreamingThread7, WSTR_NAME(StreamingThread7));
		mNamedWStrs.emplace(NamedThread::RHIThread_Gfx, WSTR_NAME(RHIThread_Gfx));
		mNamedWStrs.emplace(NamedThread::RHIThread_Compute, WSTR_NAME(RHIThread_Compute));
		mNamedWStrs.emplace(NamedThread::RHIThread_Transf, WSTR_NAME(RHIThread_Transf));
		mNamedWStrs.emplace(NamedThread::ScheduleThread, WSTR_NAME(ScheduleThread));



		//mThreads.emplace(NamedThread::RenderThread, new ThreadWrap(NamedThread::RenderThread));
		//mThreads.emplace(NamedThread::StreamingThread0, new ThreadWrap(NamedThread::StreamingThread0));

	}

	ThreadManager::~ThreadManager()
	{
		for (auto&& thr : mThreads)
		{
			SAFE_DELETE(thr.second);
		}
	}

	void ThreadManager::DestroyThread(ThreadWrap* InThread)
	{
		if (InThread == nullptr)
			return;

		mThreads.erase(InThread->m_eNamedThread);
		SAFE_DELETE(InThread);
	}

	const std::string& ThreadManager::GetStringName(NamedThread InName)
	{
		return mNamedStrs[InName];
	}
	const std::wstring& ThreadManager::GetWStringName(NamedThread InName)
	{
		return mNamedWStrs[InName];

	}
}