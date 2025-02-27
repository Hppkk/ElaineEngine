#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineThreadManager.h"

namespace Elaine
{
	using FuncCallHandler = unsigned long long;
	using ThreadFunc = void(*)(void* p, FuncCallHandler handler);
	using NoParamFunc = void(*)();

	struct ThreadEventDesc
	{
		friend class ThreadWrap;

		template<class Func, class... Args>
		void init(Func func, Args&&... args)
		{
			m_func = std::bind(func, std::forward<Args>(args)...);
		}

		void requestCompleted()
		{

		}

		static void	ExeEventFunc(void* p, FuncCallHandler handler)
		{

		}
	private:
		std::function<void()>		m_func;
	};

	class ElaineCoreExport ThreadWrap
	{
		friend class ThreadManager;
	public:
		template <class Func, class... Args>
		ThreadWrap(NamedThread InName, Func&& _Fx, Args&&... _Ax) 
			: m_eNamedThread(InName)
		{
			m_sThreadName = ThreadManager::instance()->GetStringName(InName);
			m_thread = std::thread(std::forward<Func>(_Fx), std::forward<Args>(_Ax)...);

		}

		void Initilize();

		~ThreadWrap();
		const std::thread& GetThread();
	private:
		std::thread						m_thread;
		std::string						m_sThreadName;
		NamedThread						m_eNamedThread;
	};
}