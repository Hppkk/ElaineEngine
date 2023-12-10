#include "ElaineEngine.h"
#include "ElaineLogSystem.h"
#include "ElaineRoot.h"

namespace Elaine
{
	ElaineEngine::~ElaineEngine()
	{
		close();
	}

	void ElaineEngine::initilize(const EngineInitDesc& desc)
	{
		new Root(em_Editor, tm_Thread0);
		Root::instance()->Init();
		LOG_INFO("Engine Init");
	}

	void ElaineEngine::tickOneFrame()
	{
		Root::instance()->tickOnceFrame();
	}

	void ElaineEngine::close()
	{
		LOG_INFO("Engine Unload");
	}
}