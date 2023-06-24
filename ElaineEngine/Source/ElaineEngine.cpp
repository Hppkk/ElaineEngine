#include "ElaineEngine.h"
#include "ElaineLogSystem.h"

namespace Elaine
{
	ElaineEngine::~ElaineEngine()
	{
		close();
	}

	void ElaineEngine::initilize(const EngineInitDesc& desc)
	{
		LOG_INFO("Engine Init");
	}

	void ElaineEngine::tickOneFrame(float dt)
	{

	}

	void ElaineEngine::close()
	{
		LOG_INFO("Engine Unload");
	}
}