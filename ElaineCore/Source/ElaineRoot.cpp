#include "ElainePrecompiledHeader.h"
#include "ElaineTimer.h"
#include "render/ElaineWindowSystem.h"
#include "ElaineInputSystem.h"
#include "resource/ElaineDataStreamMgr.h"
#include "ElaineThreadManager.h"
#include "ElaineFileManager.h"

namespace Elaine
{
	Root::Root()
	{
		new LogSystem();
	}

	Root::~Root()
	{
		terminate();
	}

	void Root::initilize(const RHI_PARAM_DESC& InDesc)
	{
		new FileManager();
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
		m_sResourcePath = m_sAppPath + "/../../../Contents/";
#endif 

		
		new DataStreamMgr();
		new GameObjectInfoMgr();
		readConfig(m_sResourcePath + "config/EngineConfig.cfg");
		new ThreadManager();
		//new WindowSystem();
		m_pRenderSystem = new RenderSystem();
		m_pRenderSystem->Initilize(InDesc);
		new InputSystem();
		m_MainSceneMgr = new SceneManager("Main SceneManager");
		m_SceneMgrs.emplace("Main SceneManager", m_MainSceneMgr);
		
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

	void Root::beginFrame(float dt)
	{
		
	}
	void Root::fixedUpdate(float dt)
	{

	}
	void Root::endFrame(float dt)
	{

	}

	void Root::tickOnceFrame()
	{
		float dt = calculateDeltaTime();
		calculateFPS(dt);
		beginFrame(dt);
		fixedUpdate(dt);
		endFrame(dt);
		
	}

	SceneManager* Root::getSceneManager(const String& name)
	{
		auto iter = m_SceneMgrs.find(name);
		if (iter != m_SceneMgrs.end())
			return iter->second;

		return nullptr;
	}

	SceneManager* Root::getMainSceneManager()
	{
		return m_MainSceneMgr;
	}

	SceneManager* Root::createSceneManager(const String& name)
	{
		auto mgr = new SceneManager(name);
		m_SceneMgrs.emplace(name, mgr);
		return mgr;
	}

	void Root::readConfig(const std::string& file)
	{
		 ResourceBasePtr res = DataStreamMgr::instance()->getDataStreamFromFile(file);
		 DataStream* ds = static_cast<DataStream*>(res.get());
		 auto stream = ds->getDataStream();
		 cJSON* pNode = cJSON_Parse(stream);
		 cJSON* pWindows = cJSON_GetObjectItem(pNode, "Windows");
		 cJSON* pRHI = cJSON_GetObjectItem(pWindows, "RenderRHI");
		 m_RHIType = (RHITYPE)pRHI->valueint;
	}

	void Root::terminate()
	{
		delete RenderSystem::instance();
		delete ThreadManager::instance();
		delete LogSystem::instance();
		//delete WindowSystem::instance();
		SAFE_DELETE(m_timer);
		delete InputSystem::instance();
		delete GameObjectInfoMgr::instance();
		delete DataStreamMgr::instance();
		delete FileManager::instance();
		for (auto& iter : m_SceneMgrs)
		{
			SAFE_DELETE(iter.second);
		}
		
	}
}