#pragma once

namespace Elaine
{
	class ElaineCoreExport ThreadManager :public Singleton<ThreadManager>
	{
	public:
		ThreadManager();
		~ThreadManager();
		ElaineThread*				getThread(const std::string& type);
	private:
		std::set<ElaineThread*>		m_threadPool;
	};
}