#pragma once
#include <atomic>

namespace Elaine
{
	template<typename T>
	class DefaultDeleter
	{
	public:
		DefaultDeleter() = default;

		void operator()(T* _Ptr) const noexcept
		{ 
			SAFE_DELETE(_Ptr);
		}
	};

	class _UseCountBase
	{
	public:
		virtual void Destroy() = 0;
		virtual ~_UseCountBase() = default;

		void _Incref() noexcept
		{
			_Used.fetch_add(1, std::memory_order_relaxed);
		}

		void _Decref() noexcept
		{
			if (_Used.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				Destroy();
				delete this;
			}
		}

		unsigned int getUseCount()
		{
			return _Used.load(std::memory_order_acquire);
		}

	public:
		std::atomic_uint _Used{0};
	};

	template<typename T, typename Deleter>
	struct UseCount : _UseCountBase
	{
		T* Ptr;
		Deleter Del;

		UseCount(T* p) : Ptr(p) {}

		void Destroy() override
		{
			Del(Ptr);
		}
	};
}
