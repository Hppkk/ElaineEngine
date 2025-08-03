#pragma once
#include <atomic>

namespace Elaine
{
	template<class Ty>
	struct RHIDefaultDeleter
	{
		RHIDefaultDeleter() noexcept = default;

		void operator()(Ty* InPtr) const noexcept
		{
			delete InPtr;
		}
	};

	template<class Ty>
	class SmartPtr;

	template<class Ty>
	class RHIResourceRef
	{
	public:
		RHIResourceRef() = default;
	private:
		void IncreaseRef() noexcept { ++mRefCount; }
		void DecreaseRef() noexcept { --mRefCount; }
		std::atomic_uint32_t UseCount() const { return mRefCount; }
	private:
		std::atomic_uint32_t mRefCount = 0;
		friend class SmartPtr<Ty>;
	};
}