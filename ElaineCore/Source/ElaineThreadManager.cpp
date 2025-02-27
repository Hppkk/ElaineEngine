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
		mNamedStrs.emplace(NamedThread::ResourceThread0, STR_NAME(ResourceThread0));
		mNamedStrs.emplace(NamedThread::ResourceThread1, STR_NAME(ResourceThread1));
		mNamedStrs.emplace(NamedThread::ResourceThread2, STR_NAME(ResourceThread2));
		mNamedStrs.emplace(NamedThread::RHIThread_Gfx, STR_NAME(RHIThread_Gfx));
		mNamedStrs.emplace(NamedThread::RHIThread_Compute, STR_NAME(RHIThread_Compute));
		mNamedStrs.emplace(NamedThread::RHIThread_Transf, STR_NAME(RHIThread_Transf));

		mNamedWStrs.emplace(NamedThread::GameThread, WSTR_NAME(GameThread));
		mNamedWStrs.emplace(NamedThread::GeneralThread0, WSTR_NAME(GeneralThread0));
		mNamedWStrs.emplace(NamedThread::GeneralThread1, WSTR_NAME(GeneralThread1));
		mNamedWStrs.emplace(NamedThread::GeneralThread2, WSTR_NAME(GeneralThread2));
		mNamedWStrs.emplace(NamedThread::JobThread0, WSTR_NAME(JobThread0));
		mNamedWStrs.emplace(NamedThread::JobThread1, WSTR_NAME(JobThread1));
		mNamedWStrs.emplace(NamedThread::JobThread2, WSTR_NAME(JobThread2));
		mNamedWStrs.emplace(NamedThread::RenderThread, WSTR_NAME(RenderThread));
		mNamedWStrs.emplace(NamedThread::ResourceThread0, WSTR_NAME(ResourceThread0));
		mNamedWStrs.emplace(NamedThread::ResourceThread1, WSTR_NAME(ResourceThread1));
		mNamedWStrs.emplace(NamedThread::ResourceThread2, WSTR_NAME(ResourceThread2));
		mNamedWStrs.emplace(NamedThread::RHIThread_Gfx, WSTR_NAME(RHIThread_Gfx));
		mNamedStrs.emplace(NamedThread::RHIThread_Compute, STR_NAME(RHIThread_Compute));
		mNamedStrs.emplace(NamedThread::RHIThread_Transf, STR_NAME(RHIThread_Transf));



		//mThreads.emplace(NamedThread::RenderThread, new ThreadWrap(NamedThread::RenderThread));
		//mThreads.emplace(NamedThread::ResourceThread0, new ThreadWrap(NamedThread::ResourceThread0));

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