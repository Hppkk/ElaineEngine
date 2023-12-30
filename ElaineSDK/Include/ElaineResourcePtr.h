#pragma once
#include "ElaineUseCount.h"

namespace Elaine
{
	template<typename _Ty, typename Deleter = DefaultDeleter<_Ty>>
	class ResourcePtr
	{
	public:
		ResourcePtr() = default;
		ResourcePtr(_Ty* ptr_)
			: _Ptr(ptr_)
		{
			_Used._Incref();
		}

		ResourcePtr(const ResourcePtr<_Ty, Deleter>& rhs)
		{
			rhs._Used._Incref();
			_Used = rhs._Used;
			_Ptr = rhs._Ptr;
		}

		ResourcePtr(ResourcePtr<_Ty, Deleter>&& rhs)
		{
			_Used = rhs._Used;
			_Ptr = rhs._Ptr;
			//rhs->_Used = nullptr;
			//rhs->_Ptr = nullptr;
		}

		~ResourcePtr()
		{
			_Used._Decref();
			if (_Used._Used == 0u)
			{
				_Ptr._delete();
				_Ptr = nullptr;
			}
		}

		unsigned int getUsed() const
		{
			return _Used->_uUsed;
		}

		_Ty* get()
		{
			return _Ptr._ptr;
		}

		ResourcePtr& operator=(ResourcePtr<_Ty, Deleter>& rhs)
		{
			rhs._Used._Incref();
			_Used = rhs._Used;
			_Ptr = rhs._Ptr;
			return *this;
		}

		ResourcePtr& operator=(ResourcePtr<_Ty, Deleter>&& rhs)
		{
			_Used = rhs._Used;
			_Ptr = rhs._Ptr;
			//rhs._Used = nullptr;
			//rhs._Ptr = nullptr;
			return *this;
		}

		bool operator==(const ResourcePtr<_Ty, Deleter>& rhs)
		{
			return get() == rhs.get();
		}

		bool isNull()
		{
			return _Used.getUseCount() != 0;
		}

		_Ty& operator*()
		{
			return *_Ptr._ptr;
		}
		_Ty* operator->()
		{
			return _Ptr._ptr;
		}

	protected:
		Deleter				  _Ptr;
		mutable _UseCountBase _Used;
	};
}