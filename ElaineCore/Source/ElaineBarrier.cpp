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
	void EBarrier::Signal()
	{
		std::unique_lock<std::mutex> UniqueLocker(mMtx);
		mSingaled = true;
		mConditionVariable.notify_one();
	}
}