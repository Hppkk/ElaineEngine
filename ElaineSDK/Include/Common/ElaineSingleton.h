#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineStdRequirements.h"

namespace Elaine
{
	template<class T>
	class Singleton
	{
	public:

		Singleton()
		{
			assert(m_instance == 0);
			m_instance = static_cast<T*>(this);
		}
		~Singleton()
		{ 
			m_instance = 0;
		}

		inline static T* instance()
		{
			assert(m_instance != nullptr);
			return m_instance;
		}
	protected:
		inline static T*		m_instance = 0;
	};

}