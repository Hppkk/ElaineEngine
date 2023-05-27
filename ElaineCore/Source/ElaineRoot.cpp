#include "ElainePrecompiledHeader.h"

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
	}

	void Root::terminate()
	{
		delete LogSystem::instance();
	}
}