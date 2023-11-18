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

	class ElaineCoreExport Root :public Singleton<Root>
	{
	public:
		Root() = default;
		Root(EngineMode mode);
		~Root();
		void					Init();
		EngineMode				GetEngineMode()const { return m_currentMode; }
		std::string&			getExePath() const { return m_sAppPath; }
		std::string&			getResourcePath()const { return m_sResourcePath; }
		float					calculateDeltaTime();
		void					calculateFPS(float dt);
		int						getFPS() { return m_fps; }
		Timer*					getTimer() { return m_timer; }
		void					beginFrame();
		void					fixedUpdate();
		void					endFrame();
	private:
		// shutdown
		void					terminate();
	private:
		EngineMode								m_currentMode = em_Editor;
		mutable std::string						m_sAppPath;
		mutable std::string						m_sResourcePath;
		std::chrono::steady_clock::time_point	m_last_tick_time_point = std::chrono::steady_clock::now();
		const float								s_fps_alpha = 1.f / 100;
		int										m_fps = 0;
		float									m_average_duration = 0.f;
		int										m_frame_count = 0;
		Timer*									m_timer = nullptr;
	};

}