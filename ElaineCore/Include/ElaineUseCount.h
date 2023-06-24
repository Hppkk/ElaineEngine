#pragma once

namespace Elaine
{
	template<typename T>
	class _UseCountBase
	{
	public:
		_UseCountBase()
		{
			m_uUseMemory = 0;
			m_ptr = static_cast<T*>(this);
		}
		virtual ~_UseCountBase()
		{
			if (m_uUseMemory.load() == 0)
			{
				m_ptr = 0;
			}
			else
			{
				m_uUseMemory--;
			}
		}

		template<class T>
		_UseCountBase(const _UseCountBase<T>& rhs)
		{
			rhs.m_uUseMemory++;
			m_uUseMemory.store(rhs.m_uUseMemory.load());
			m_ptr = rhs.m_ptr;
		}
		//_UseCountBase& operator=(const _UseCountBase& rhs);
		//_UseCountBase& operator=(const _UseCountBase&& rhs);


		size_t					getUseCount()
		{
			return m_uUseMemory.load();
		}

	protected:
		std::atomic_size_t		m_uUseMemory;
		T*						m_ptr = nullptr;
	};
}