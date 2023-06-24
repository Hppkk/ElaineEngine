#include "ElainePrecompiledHeader.h"

namespace Elaine
{
	//template<typename T>
	//_UseCountBase<T>::_UseCountBase()
	//{
	//	m_uUseMemory = 0;
	//	m_ptr = static_cast<T*>(this);
	//}

	//template<typename T>
	//_UseCountBase<T>::~_UseCountBase()
	//{
	//	if (m_uUseMemory.load() == 0)
	//	{
	//		m_ptr = 0;
	//	}
	//	else
	//	{
	//		m_uUseMemory--;
	//	}
	//	
	//}

	//template<typename T>
	//_UseCountBase<T>::_UseCountBase(const _UseCountBase& rhs)
	//{
	//	rhs->m_uUseMemory++;
	//	m_uUseMemory = rhs->m_uUseMemory;
	//	m_ptr = rhs->m_ptr;

	//}

	////template<typename T>
	////_UseCountBase<T>& _UseCountBase<T>::operator=(const _UseCountBase& rhs)
	////{
	////	rhs->m_uUseMemory++;
	////	m_uUseMemory = rhs->m_uUseMemory;
	////	m_ptr = rhs->m_ptr;
	////}

	////template<typename T>
	////_UseCountBase<T>& _UseCountBase<T>::operator=(const _UseCountBase&& rhs)
	////{
	////	rhs->m_uUseMemory++;
	////	m_uUseMemory = rhs->m_uUseMemory;
	////	m_ptr = rhs->m_ptr;
	////}

	//template<typename T>
	//size_t _UseCountBase<T>::getUseCount()
	//{
	//	return m_uUseMemory.load();
	//}
}