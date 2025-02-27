#include "ElainePrecompiledHeader.h"
#include "ElaineThread.h"
#ifdef WIN32
#include <Windows.h>
#include <processthreadsapi.h>
#else
#include <pthread.h>
#endif

namespace Elaine
{

	const std::thread& ThreadWrap::GetThread()
	{
		return m_thread;
	}

	void ThreadWrap::Initilize()
	{
#ifdef WIN32
		SetThreadDescription(m_thread.native_handle(), ThreadManager::instance()->GetWStringName(m_eNamedThread).c_str());
#else
		pthread_setname_np(m_thread.native_handle(), m_sThreadName.c_str());
#endif
	}

	ThreadWrap::~ThreadWrap()
	{
		
	}

}