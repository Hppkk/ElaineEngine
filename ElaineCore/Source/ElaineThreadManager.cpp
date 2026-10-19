#include "ElainePrecompiledHeader.h"
#include "ElaineThreadManager.h"

namespace Elaine
{

	thread_local NamedThread _CurrentThread;

	ThreadManager::ThreadManager()
	{
		mNamedStrs.emplace(NamedThread::GameThread, STR_NAME(GameThread));
		mNamedStrs.emplace(NamedThread::GeneralThread0, STR_NAME(GeneralThread0));
		mNamedStrs.emplace(NamedThread::GeneralThread1, STR_NAME(GeneralThread1));
		mNamedStrs.emplace(NamedThread::GeneralThread2, STR_NAME(GeneralThread2));
		mNamedStrs.emplace(NamedThread::RenderThread, STR_NAME(RenderThread));
		mNamedStrs.emplace(NamedThread::TaskThread0, STR_NAME(TaskThread0));
		mNamedStrs.emplace(NamedThread::TaskThread1, STR_NAME(TaskThread1));
		mNamedStrs.emplace(NamedThread::TaskThread2, STR_NAME(TaskThread2));
		mNamedStrs.emplace(NamedThread::TaskThread3, STR_NAME(TaskThread3));
		mNamedStrs.emplace(NamedThread::TaskThread4, STR_NAME(TaskThread4));
		mNamedStrs.emplace(NamedThread::TaskThread5, STR_NAME(TaskThread5));
		mNamedStrs.emplace(NamedThread::TaskThread6, STR_NAME(TaskThread6));
		mNamedStrs.emplace(NamedThread::TaskThread7, STR_NAME(TaskThread7));
		mNamedStrs.emplace(NamedThread::RHIThread, STR_NAME(RHIThread));
		mNamedStrs.emplace(NamedThread::ScheduleThread, STR_NAME(ScheduleThread));


		mNamedWStrs.emplace(NamedThread::GameThread, WSTR_NAME(GameThread));
		mNamedWStrs.emplace(NamedThread::GeneralThread0, WSTR_NAME(GeneralThread0));
		mNamedWStrs.emplace(NamedThread::GeneralThread1, WSTR_NAME(GeneralThread1));
		mNamedWStrs.emplace(NamedThread::GeneralThread2, WSTR_NAME(GeneralThread2));
		mNamedWStrs.emplace(NamedThread::RenderThread, WSTR_NAME(RenderThread));
		mNamedWStrs.emplace(NamedThread::TaskThread0, WSTR_NAME(TaskThread0));
		mNamedWStrs.emplace(NamedThread::TaskThread1, WSTR_NAME(TaskThread1));
		mNamedWStrs.emplace(NamedThread::TaskThread2, WSTR_NAME(TaskThread2));
		mNamedWStrs.emplace(NamedThread::TaskThread3, WSTR_NAME(TaskThread3));
		mNamedWStrs.emplace(NamedThread::TaskThread4, WSTR_NAME(TaskThread4));
		mNamedWStrs.emplace(NamedThread::TaskThread5, WSTR_NAME(TaskThread5));
		mNamedWStrs.emplace(NamedThread::TaskThread6, WSTR_NAME(TaskThread6));
		mNamedWStrs.emplace(NamedThread::TaskThread7, WSTR_NAME(TaskThread7));
		mNamedWStrs.emplace(NamedThread::RHIThread, WSTR_NAME(RHIThread));
		mNamedWStrs.emplace(NamedThread::ScheduleThread, WSTR_NAME(ScheduleThread));
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

	bool ThreadManager::CheckThread(NamedThread InName)
	{
		return _CurrentThread == InName;
	}

	void ThreadManager::InitilizeThread(NamedThread InType)
	{
		_CurrentThread = InType;
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