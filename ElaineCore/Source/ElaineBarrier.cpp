#include "ElainePrecompiledHeader.h"
#include "ElaineBarrier.h"

namespace Elaine
{
	EBarrier::EBarrier()
	{

	}

	EBarrier::~EBarrier()
	{

	}

	void EBarrier::Wait()
	{
		std::unique_lock<std::mutex> UniqueLocker(mMtx);
		mConditionVariable.wait(UniqueLocker, [this] { return mSingaled; });
		mSingaled = false;
	}
    bool EBarrier::WaitFor(uint32_t Milliseconds)
    {
        std::unique_lock<std::mutex> Lock(mMtx);
        if (!mConditionVariable.wait_for(Lock, std::chrono::milliseconds(Milliseconds), [this] { return mSingaled; })) return false;
        mSingaled = false;
        return true;
    }
    void EBarrier::SetWakeCallback(std::function<void()> Callback)
    { std::lock_guard<std::mutex> Lock(mMtx); mWakeCallback = std::move(Callback); }
	void EBarrier::Signal()
	{
        std::function<void()> Callback;
        { std::lock_guard<std::mutex> Lock(mMtx); mSingaled = true; Callback = mWakeCallback; }
        mConditionVariable.notify_one();
        if (Callback) Callback();
	}
}