#pragma once

namespace Elaine
{
	using FuncCallHandler = unsigned long long;
	using ThreadFunc = void(*)(void* p, FuncCallHandler handler);
	using NoParamFunc = void(*)();

	struct ThreadEventDesc
	{
		friend class ElaineThread;

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

	class ElaineCoreExport ElaineThread
	{
		friend class ThreadManager;
	public:
		ElaineThread(const std::string& name = "");
		~ElaineThread();
		void			ThreadMainFunc();
		void			pushThreadFunc(ThreadEventDesc even);
	private:
		std::future<void>				m_thread;
		std::string						m_sThreadName;
		bool							m_bIsExit;
		std::deque<ThreadEventDesc>		m_ThreadFuncQueue;
		std::condition_variable			m_conditionVariable;
		std::mutex						m_mutex;
	};
}