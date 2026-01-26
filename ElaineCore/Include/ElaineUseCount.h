#pragma once

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
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&_Used));
		}

		void _Decref() noexcept
		{
			if (_InterlockedDecrement(reinterpret_cast<volatile long*>(&_Used)) == 0)
			{
				Destroy();
				delete this;
			}
		}

		unsigned int getUseCount()
		{
			return _Used;
		}

	public:
		unsigned int _Used = 0;
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
			delete this;
		}
	};
}