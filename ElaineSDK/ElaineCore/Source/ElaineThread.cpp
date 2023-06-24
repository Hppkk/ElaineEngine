#include "ElainePrecompiledHeader.h"
#include "ElaineThread.h"

namespace Elaine
{
	ElaineThread::ElaineThread(const std::string& name /*= ""*/)
		: m_sThreadName(name)
		, m_bIsExit(false)
	{
		auto func = std::bind(&ElaineThread::ThreadMainFunc, this);
		m_thread = std::async(std::launch::async, func);
	}

	ElaineThread::~ElaineThread()
	{
		m_bIsExit = true;
	}

	void ElaineThread::pushThreadFunc(ThreadEventDesc even)
	{
		m_ThreadFuncQueue.push_back(even);
		m_conditionVariable.notify_one();
	}

	void ElaineThread::ThreadMainFunc()
	{
		while (!m_bIsExit)
		{
			std::unique_lock<std::mutex> uniqulock(m_mutex);
			uniqulock.lock();
			while (m_ThreadFuncQueue.empty())
			{
				//m_conditionVariable.wait(m_mutex);
			}

			while (!m_ThreadFuncQueue.empty())
			{
				ThreadEventDesc evn = m_ThreadFuncQueue.front();
				m_ThreadFuncQueue.pop_front();
				evn.m_func();
			}
			uniqulock.unlock();
			m_conditionVariable.notify_one();
		}
	}
}