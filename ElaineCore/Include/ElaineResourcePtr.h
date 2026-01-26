#pragma once
#include "ElaineUseCount.h"

namespace Elaine
{
	template<typename _Ty>
	class ResourcePtr
	{
	public:
		using element_type = _Ty;
		ResourcePtr() = default;
		ResourcePtr(_Ty* InPtr)
		{
			if (InPtr == nullptr)
				return;

			_Ptr = InPtr;
			_Used = new UseCount<element_type, DefaultDeleter<_Ty>>(InPtr);
			_Used->_Incref();
		}

		ResourcePtr(const ResourcePtr<_Ty>& InOther)
		{
			InOther._Used->_Incref();
			_Used = InOther._Used;
			_Ptr = InOther._Ptr;
		}

		ResourcePtr(ResourcePtr<_Ty>&& InOther)
		{
			_Used = InOther._Used;
			_Ptr = InOther._Ptr;
			InOther._Used = nullptr;
			InOther._Ptr = nullptr;
		}

		template<typename Ty2>
		ResourcePtr(ResourcePtr<Ty2>&& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			InOther._Used = nullptr;
			InOther._Ptr = nullptr;
		}

		template<typename Ty2>
		ResourcePtr(ResourcePtr<Ty2>& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			if (_Used)
				_Used->_Incref();
		}

		template<typename Ty2>
		ResourcePtr(ResourcePtr<_Ty>&& InOther, Ty2* InPtr)
		{
			_Ptr = InPtr;
			_Used = InOther._Used;
			InOther._Used = nullptr;
			InOther._Ptr = nullptr;
		}

		template<typename Ty2>
		ResourcePtr(ResourcePtr<_Ty>& InOther, Ty2* InPtr)
		{
			_Ptr = InPtr;
			_Used = InOther._Used;
			if (_Used)
				_Used->_Incref();
		}

		template<typename Ty2>
		ResourcePtr(const ResourcePtr<Ty2>& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			if (_Used)
				_Used->_Incref();
		}

		~ResourcePtr()
		{
			if(_Used)
				_Used->_Decref();
		}

		unsigned int getUsed() const
		{
			return _Used->getUseCount();
		}

		_Ty* get() const
		{
			return _Ptr;
		}

		template<typename Ty2>
		ResourcePtr& operator=(ResourcePtr<Ty2>&& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			InOther._Used = nullptr;
			InOther._Ptr = nullptr;
			return *this;
		}

		template<typename Ty2>
		ResourcePtr& operator=(ResourcePtr<Ty2>& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			if (_Used)
				_Used->_Incref();

			return *this;
		}

		template<typename Ty2>
		ResourcePtr& operator=(const ResourcePtr<Ty2>& InOther)
		{
			_Ptr = static_cast<_Ty*>(InOther._Ptr);
			_Used = InOther._Used;
			if (_Used)
				_Used->_Incref();
			return *this;
		}

		ResourcePtr& operator=(const ResourcePtr<_Ty>& InOther)
		{
			InOther._Used->_Incref();
			_Used = InOther._Used;
			_Ptr = InOther._Ptr;
			return *this;
		}

		ResourcePtr& operator=(ResourcePtr<_Ty>& InOther)
		{
			InOther._Used->_Incref();
			_Used = InOther._Used;
			_Ptr = InOther._Ptr;
			return *this;
		}

		ResourcePtr& operator=(ResourcePtr<_Ty>&& InOther) noexcept
		{
			_Used = InOther._Used;
			_Ptr = InOther._Ptr;
			InOther._Used = nullptr;
			InOther._Ptr = nullptr;
			return *this;
		}

		bool operator==(const ResourcePtr<_Ty>& InOther) const
		{
			return get() == InOther.get();
		}

		bool isNull() const
		{
			return !_Used || _Used->getUseCount() == 0;
		}

		_Ty& operator*() const
		{
			return *_Ptr;
		}
		_Ty* operator->() const
		{
			return _Ptr;
		}

	protected:
		_UseCountBase* _Used = nullptr;
		element_type* _Ptr = nullptr;

		template<typename>
		friend class ResourcePtr;
	};

	template <class Ty1, class Ty2>
	ResourcePtr<Ty1> dynamic_pointer_cast(ResourcePtr<Ty2>&& _Other) noexcept
	{
		const auto _Ptr = dynamic_cast<typename ResourcePtr<Ty1>::element_type*>(_Other.get());

		if (_Ptr)
		{
			return ResourcePtr<Ty1>(std::move(_Other), _Ptr);
		}

		return {};
	}

	template <class Ty1, class Ty2>
	ResourcePtr<Ty1> dynamic_pointer_cast(ResourcePtr<Ty2>& _Other) noexcept
	{
		const auto _Ptr = dynamic_cast<typename ResourcePtr<Ty1>::element_type*>(_Other.get());

		if (_Ptr)
		{
			return ResourcePtr<Ty1>(_Other, _Ptr);
		}

		return {};
	}

	template <class Ty1, class Ty2>
	ResourcePtr<Ty1> static_pointer_cast(ResourcePtr<Ty2>&& _Other) noexcept
	{
		const auto _Ptr = static_cast<typename ResourcePtr<Ty1>::element_type*>(_Other.get());

		if (_Ptr)
		{
			return ResourcePtr<Ty1>(std::move(_Other), _Ptr);
		}

		return {};
	}

	template <class Ty1, class Ty2>
	ResourcePtr<Ty1> static_pointer_cast(ResourcePtr<Ty2>& _Other) noexcept
	{
		const auto _Ptr = static_cast<typename ResourcePtr<Ty1>::element_type*>(_Other.get());

		if (_Ptr)
		{
			return ResourcePtr<Ty1>(_Other, _Ptr);
		}

		return {};
	}
}