#pragma once
#include <ElaineCorePrerequirements.h>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <Windows.h>

namespace Elaine
{
	class ElaineCoreExport Event
	{
	public:
		Event(bool InAutoReset = true, bool InInitSignaled = false);
		~Event();
		Event(const Event&) = delete;
		Event& operator=(const Event&) = delete;
		void Signal();
		void Reset();
		void Wait();
	private:
#if ELAINE_PLATFORM != ELAINE_PLATFORM_WINDOWS
		bool mAutoReset = true;
		std::mutex mMtx;
		std::condition_variable mCV;
		std::atomic_bool mSignaled = false;
#else
		HANDLE mHandle;
#endif
	};
}