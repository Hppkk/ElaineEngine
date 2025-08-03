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
	void EBarrier::Signal()
	{
		std::unique_lock<std::mutex> UniqueLocker(mMtx);
		mSingaled = true;
		mConditionVariable.notify_one();
	}
}