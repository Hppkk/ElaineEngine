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

	void ElaineThread::ThreadMainFunc()
	{
		while (!m_bIsExit)
		{
			
		}
	}
}