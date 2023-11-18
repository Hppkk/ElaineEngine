#include "ElainePrecompiledHeader.h"
#include "ElaineTimer.h"
#include "render/ElaineWindowSystem.h"

namespace Elaine
{
	Root::Root(EngineMode mode):
		m_currentMode(mode)
	{

	}

	Root::~Root()
	{
		terminate();
	}

	void Root::Init()
	{
		m_timer = new Timer();
#if  ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		char szFilePath[MAX_PATH + 1] = { 0 };
		GetModuleFileNameA(NULL, szFilePath, MAX_PATH);
		for (int i = 0; i < MAX_PATH + 1; ++i)
		{
			if (szFilePath[i] == '\\')
				szFilePath[i] = '/';
		}
		m_sAppPath = szFilePath;
		auto pos = m_sAppPath.find_last_of('/');
		m_sAppPath = m_sAppPath.substr(0, pos);
		m_sResourcePath = m_sAppPath + "/../../../../../Asset/";
#endif 


		new LogSystem();
		new ThreadManager();
		new WindowSystem();
		new RenderSystem();
		new VulkanRHI();
		RHIInitInfo info;
		if (m_currentMode == em_Editor)
			info.windowname = "ElaineEditor";
		else
			info.windowname = "Runtime";
		VulkanRHI::instance()->initialize(&info);
	}

	float Root::calculateDeltaTime()
	{
		float dt = .0f;
		using namespace std::chrono;
		steady_clock::time_point tick_time_point = steady_clock::now();
		duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
		dt = time_span.count();
		m_last_tick_time_point = tick_time_point;
		return dt;
	}

	void Root::calculateFPS(float dt)
	{
		m_frame_count++;
		if (m_frame_count == 1)
		{
			m_average_duration = dt;
		}
		else
		{
			m_average_duration = m_average_duration * (1 - s_fps_alpha) + dt * s_fps_alpha;
		}
		m_fps = static_cast<int>(1.f / m_average_duration);
	}

	void Root::terminate()
	{
		delete VulkanRHI::instance();
		delete RenderSystem::instance();
		delete ThreadManager::instance();
		delete LogSystem::instance();
		delete WindowSystem::instance();
		SAFE_DELETE(m_timer);
	}
}