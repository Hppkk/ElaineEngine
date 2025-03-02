#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport EBarrier
	{
	public:
		EBarrier();
		~EBarrier();
		void Wait();
		void Signal();
	private:
		std::mutex mMtx;
		std::condition_variable mConditionVariable;
	};
}