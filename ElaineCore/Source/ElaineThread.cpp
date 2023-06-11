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
	}

	void ElaineThread::ThreadMainFunc()
	{
		while (!m_bIsExit)
		{
			if (!m_ThreadFuncQueue.empty())
			{

			}
			else
			{

			}

			while (!m_ThreadFuncQueue.empty())
			{
				ThreadEventDesc evn = m_ThreadFuncQueue.front();
				evn.m_func();
			}
		}
	}
}