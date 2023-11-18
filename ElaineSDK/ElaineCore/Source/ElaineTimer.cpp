#include "ElaineTimer.h"

namespace Elaine
{
	Timer::Timer()
	{
		
	}

	long long Timer::getMilliSeconds()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(m_time.time_since_epoch()).count();
	}

	long long Timer::getMicroSeconds()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(m_time.time_since_epoch()).count();
	}

	long long Timer::getNanoSeconds()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(m_time.time_since_epoch()).count();
	}

	Timer::~Timer()
	{
		
	}
}