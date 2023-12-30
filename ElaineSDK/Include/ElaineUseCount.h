#pragma once

namespace Elaine
{
	template<typename T>
	class DefaultDeleter
	{
	public:
		DefaultDeleter() = default;
		DefaultDeleter(T* rhs)
		{
			_ptr = rhs;
		}

		~DefaultDeleter()
		{
			//_delete();
		}

		void _delete()
		{
			SAFE_DELETE(_ptr);
		}

		T* _ptr = nullptr;
	};

	class _UseCountBase
	{
	public:
		_UseCountBase()
		{
		}

		virtual ~_UseCountBase()
		{	
		}

		void _Incref() noexcept
		{
			_InterlockedIncrement(reinterpret_cast<volatile long*>(&_Used));
		}

		void _Decref() noexcept
		{
			_InterlockedDecrement(reinterpret_cast<volatile long*>(&_Used));
		}

		_UseCountBase(const _UseCountBase& rhs)
		{
			_Used = rhs._Used;
		}

		_UseCountBase& operator=(const _UseCountBase& rhs)
		{
			_Used = rhs._Used;
			return *this;
		}

		//_UseCountBase& operator=(const _UseCountBase& rhs);
		//_UseCountBase& operator=(const _UseCountBase&& rhs);


		unsigned long getUseCount()
		{
			return _Used;
		}

	public:
		unsigned long	_Used = 0;
	};
}