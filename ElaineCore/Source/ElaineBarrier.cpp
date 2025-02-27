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
		mConditionVariable.wait(UniqueLocker);
	}
	void EBarrier::Notify()
	{
		std::unique_lock<std::mutex> UniqueLocker(mMtx);
		mConditionVariable.notify_one();
	}
}