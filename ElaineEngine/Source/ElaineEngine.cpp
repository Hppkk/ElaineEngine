#include "ElaineEngine.h"
#include "ElaineLogSystem.h"

namespace Elaine
{
	ElaineEngine::~ElaineEngine()
	{
		DestroyEngine();
	}

	void ElaineEngine::Initilize(const RHI_PARAM_DESC& InRendererDesc)
	{
		new Root();
		LOG_INFO("Elaine Engine Initilize...");
		Root::instance()->initilize(InRendererDesc);
		LOG_INFO("Elaine Engine Initilize Complete!");
	}

	void ElaineEngine::RenderOneFrame()
	{
		Root::instance()->tickOnceFrame();
	}

	void ElaineEngine::RenderOneFrame(float InDeltaTime)
	{

	}

	void ElaineEngine::DestroyEngine()
	{
		LOG_INFO("Elaine Engine Unload");
		delete Root::instance();
	}
}