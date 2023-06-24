#pragma once

namespace Elaine
{
	class RequestListener
	{
	public:
		virtual void	requestArrived() { }

	};

	class ElaineCoreExport WorkBackStageThread
	{
	public:
		WorkBackStageThread();
		~WorkBackStageThread();
		void			requestAsyncResource();
		void			threadMainFunc();
		//static void		threadMainFunc(void*);
	private:
		bool				m_bIsExit = false;
		std::future<void>	m_future;
	};
}