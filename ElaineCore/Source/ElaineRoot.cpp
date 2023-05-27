#include "ElainePrecompiledHeader.h"

namespace Elaine
{
	Root::Root(EngineMode mode)
	{
		m_currentMode = mode;
	}

	Root::~Root()
	{
		terminate();
	}

	void Root::Init()
	{
		new LogSystem();
	}

	void Root::terminate()
	{
		delete LogSystem::instance();
	}
}