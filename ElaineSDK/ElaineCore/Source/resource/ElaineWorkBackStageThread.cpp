#include "ElainePrecompiledHeader.h"
#include "resource/ElaineWorkBackStageThread.h"

namespace Elaine
{

	WorkBackStageThread::WorkBackStageThread()
	{
		auto func = std::bind(&WorkBackStageThread::threadMainFunc, this);
		m_future = std::async(std::launch::async, func);
	}

	WorkBackStageThread::~WorkBackStageThread()
	{

	}

	void WorkBackStageThread::requestAsyncResource()
	{

	}

	void WorkBackStageThread::threadMainFunc()
	{
		while (!m_bIsExit)
		{

		}
	}

}