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
			_Used->_Incref();
		}

		ResourcePtr(const ResourcePtr& rhs)
		{
			rhs._Used->_Incref();
			_Used = rhs->_Used;
			_Ptr = rhs->_Ptr;
		}

		ResourcePtr(ResourcePtr&& rhs)
		{
			_Used = rhs->_Used;
			_Ptr = rhs->_Ptr;
			rhs->_Used = nullptr;
			rhs->_Ptr = nullptr;
		}

		~ResourcePtr()
		{
			_Used->_Decref();
			if (_Used->_uUsed == 0u)
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

		ResourcePtr& operator=(const ResourcePtr& rhs) = delete;
		ResourcePtr& operator=(ResourcePtr&& rhs) = delete;

	private:
		Deleter				_Ptr;
		_UseCountBase		_Used;
	};
}