#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineSingleton.h"
#include "ElaineTimer.h"

namespace Elaine
{
	enum EngineMode
	{
		em_Editor,
		em_Runtime,
	};

	enum ThreadMode
	{
		tm_Thread0, //逻辑渲染同一个线程
		tm_Thread1, //逻辑渲染单独线程
	};

	class ElaineCoreExport Root :public Singleton<Root>
	{
	public:
		Root(EngineMode runtimeMode, ThreadMode threadMode);
		~Root();
		void					Init();
		EngineMode				GetEngineMode()const { return m_RuntimeMode; }
		std::string&			getExePath() const { return m_sAppPath; }
		std::string&			getResourcePath()const { return m_sResourcePath; }
		float					calculateDeltaTime();
		void					calculateFPS(float dt);
		int						getFPS() { return m_fps; }
		Timer*					getTimer() { return m_timer; }
		void					beginFrame(float dt);
		void					fixedUpdate(float dt);
		void					endFrame(float dt);
		void					tickOnceFrame();
		RenderSystem*			getRenderSystem() { return m_pRenderSystem; }
	private:
		// shutdown
		void					terminate();
	private:
		EngineMode								m_RuntimeMode = em_Editor;
		ThreadMode								m_ThreadMode = tm_Thread1;
		mutable std::string						m_sAppPath;
		mutable std::string						m_sResourcePath;
		std::chrono::steady_clock::time_point	m_last_tick_time_point = std::chrono::steady_clock::now();
		const float								s_fps_alpha = 1.f / 100;
		int										m_fps = 0;
		float									m_average_duration = 0.f;
		int										m_frame_count = 0;
		Timer*									m_timer = nullptr;
		RenderSystem*							m_pRenderSystem = nullptr;
	};

}