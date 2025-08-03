#pragma once
#include "render/common/ElaineRHIResourceRef.h"

namespace Elaine
{
	template<class Ty, class Deleter = RHIDefaultDeleter<Ty>>
	class RHISmartPtr
	{
	public:
		using element_type = Ty;
		RHISmartPtr() = default;
		RHISmartPtr(std::nullptr_t)
			: mPtr(nullptr)
		{

		}
		RHISmartPtr& operator=(const RHISmartPtr&) = delete;
		~RHISmartPtr()
		{
			mPtr->DecreaseRef();
			if (mPtr->UseCount() == 0)
			{
				Deleter()(mPtr);
			}
		}

		RHISmartPtr(RHISmartPtr&& InPtr)
			: mPtr(InPtr->mPtr)
		{
			InPtr->mPtr = nullptr;
		}

		RHISmartPtr(RHISmartPtr& InPtr)
		{
			mPtr = InPtr->mPtr;
			mPtr->IncreaseRef();
		}
	private:
		element_type* mPtr = nullptr;
	};
}