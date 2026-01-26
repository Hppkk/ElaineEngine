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
		mNamedStrs.emplace(NamedThread::StreamingThread0, STR_NAME(StreamingThread0));
		mNamedStrs.emplace(NamedThread::StreamingThread1, STR_NAME(StreamingThread1));
		mNamedStrs.emplace(NamedThread::StreamingThread2, STR_NAME(StreamingThread2));
		mNamedStrs.emplace(NamedThread::StreamingThread3, STR_NAME(StreamingThread3));
		mNamedStrs.emplace(NamedThread::StreamingThread4, STR_NAME(StreamingThread4));
		mNamedStrs.emplace(NamedThread::StreamingThread5, STR_NAME(StreamingThread5));
		mNamedStrs.emplace(NamedThread::StreamingThread6, STR_NAME(StreamingThread6));
		mNamedStrs.emplace(NamedThread::StreamingThread7, STR_NAME(StreamingThread7));
		mNamedStrs.emplace(NamedThread::RHIGraphicThread, STR_NAME(RHIGraphicThread));
		mNamedStrs.emplace(NamedThread::RHIComputeThread, STR_NAME(RHIComputeThread));
		mNamedStrs.emplace(NamedThread::RHITransferThread, STR_NAME(RHITransferThread));
		mNamedStrs.emplace(NamedThread::ScheduleThread, STR_NAME(ScheduleThread));


		mNamedWStrs.emplace(NamedThread::GameThread, WSTR_NAME(GameThread));
		mNamedWStrs.emplace(NamedThread::GeneralThread0, WSTR_NAME(GeneralThread0));
		mNamedWStrs.emplace(NamedThread::GeneralThread1, WSTR_NAME(GeneralThread1));
		mNamedWStrs.emplace(NamedThread::GeneralThread2, WSTR_NAME(GeneralThread2));
		mNamedWStrs.emplace(NamedThread::RenderThread, WSTR_NAME(RenderThread));
		mNamedWStrs.emplace(NamedThread::StreamingThread0, WSTR_NAME(StreamingThread0));
		mNamedWStrs.emplace(NamedThread::StreamingThread1, WSTR_NAME(StreamingThread1));
		mNamedWStrs.emplace(NamedThread::StreamingThread2, WSTR_NAME(StreamingThread2));
		mNamedWStrs.emplace(NamedThread::StreamingThread3, WSTR_NAME(StreamingThread3));
		mNamedWStrs.emplace(NamedThread::StreamingThread4, WSTR_NAME(StreamingThread4));
		mNamedWStrs.emplace(NamedThread::StreamingThread5, WSTR_NAME(StreamingThread5));
		mNamedWStrs.emplace(NamedThread::StreamingThread6, WSTR_NAME(StreamingThread6));
		mNamedWStrs.emplace(NamedThread::StreamingThread7, WSTR_NAME(StreamingThread7));
		mNamedWStrs.emplace(NamedThread::RHIGraphicThread, WSTR_NAME(RHIGraphicThread));
		mNamedWStrs.emplace(NamedThread::RHIComputeThread, WSTR_NAME(RHIComputeThread));
		mNamedWStrs.emplace(NamedThread::RHITransferThread, WSTR_NAME(RHITransferThread));
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