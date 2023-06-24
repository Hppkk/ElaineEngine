#pragma once
#include <chrono>

namespace Elaine
{
	class ElaineCoreExport Timer
	{
	public:
		Timer();
		~Timer();

		

	private:
		std::chrono::time_point<std::chrono::system_clock>  m_time {};
		
	};
}