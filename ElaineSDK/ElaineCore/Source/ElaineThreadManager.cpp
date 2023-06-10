#include "ElainePrecompiledHeader.h"

namespace Elaine
{
	ThreadManager::ThreadManager()
	{
		//int concurrency = std::thread::hardware_concurrency();
		//for (int i = 0; i < concurrency; ++i)
		//{
		//	m_threadPool.insert(new ElaineThread());
		//}

		m_threadPool.insert(new ElaineThread("RHIThread"));
		m_threadPool.insert(new ElaineThread("ResourceThread"));
		m_threadPool.insert(new ElaineThread("LogicThread"));
		//m_threadPool.insert(new ElaineThread("WorkThread"));
	}

	ThreadManager::~ThreadManager()
	{
		for (auto thr : m_threadPool)
		{
			SAFE_DELETE(thr);
		}
	}

	ElaineThread* ThreadManager::getThread(const std::string& type)
	{
		for (auto thr : m_threadPool)
		{
			if (thr->m_sThreadName == type)
			{
				return thr;
			}
		}
		return nullptr;
	}
}