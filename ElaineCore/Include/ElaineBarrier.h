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
        bool WaitFor(uint32_t Milliseconds);
		void Signal();
        void SetWakeCallback(std::function<void()> Callback);
	private:
		std::mutex mMtx;
		std::condition_variable mConditionVariable;
		bool mSingaled = false;
        std::function<void()> mWakeCallback;
	};
}