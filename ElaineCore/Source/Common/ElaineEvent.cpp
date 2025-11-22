#include <ElainePrecompiledHeader.h>
#include <ElaineEvent.h>

namespace Elaine
{
	Event::Event(bool InAutoReset, bool InInitSignaled)
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		: mHandle(nullptr)
#endif
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		mHandle = CreateEvent(nullptr, InAutoReset ? FALSE : TRUE, InInitSignaled ? TRUE : FALSE, nullptr);
#else
		mAutoReset = InAutoReset
		mSignaled.store(InInitSignaled);
#endif
	}

	Event::~Event()
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		if (mHandle)
			CloseHandle(mHandle);
#endif
	}

	void Event::Signal()
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		SetEvent(mHandle);
#else
		if (mAutoReset)
		{
			std::lock_guard<std::mutex> LockGuard(mMtx);
			mSignaled.store(false);
			mCV.notify_all();
		}
		else
		{
			std::lock_guard<std::mutex> LockGuard(mMtx);
			mSignaled.store(true);
			mCV.notify_all();
		}
#endif
	}

	void Event::Reset()
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		ResetEvent(mHandle);
#else
		mSignaled.store(false);
#endif
	}

	void Event::Wait()
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		WaitForSingleObject(mHandle, INFINITE);
#else
		std::unique_lock<std::mutex> lk(mMtx);
		mCV.wait(lk, [this] { return mSignaled.load(); });
		if (mAutoReset)
		{
			mSignaled.store(false);
		}
#endif
	}
}