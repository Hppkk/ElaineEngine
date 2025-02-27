#include "ElainePrecompiledHeader.h"
#include "ElaineTimer.h"

namespace Elaine
{
	Timer::Timer()
	{
		reset();
	}

	long long Timer::getNow()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(m_time.time_since_epoch()).count();
	}

	void Timer::reset()
	{
		m_time = std::chrono::system_clock::now();
	}

	long long Timer::getMilliSeconds()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - m_time).count();
	}

	long long Timer::getMicroSeconds()
	{
		return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - m_time).count();
	}

	long long Timer::getNanoSeconds()
	{
		return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now() - m_time).count();
	}

	double Timer::getSeconds()
	{
		return getMilliSeconds() / 1000.0;
	}

	Timer::~Timer()
	{
		
	}
}