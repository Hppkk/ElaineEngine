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
		void Notify();
	private:
		std::mutex mMtx;
		std::condition_variable mConditionVariable;
	};
}