#pragma once
#include <chrono>

namespace Elaine
{
	class ElaineCoreExport Timer
	{
	public:
		Timer();
		~Timer();

		long long	getMilliSeconds();
		long long	getMicroSeconds();
		long long	getNanoSeconds();
		long long	getNow();
		void		reset();

	private:
		std::chrono::time_point<std::chrono::system_clock>  m_time {};
	};
}