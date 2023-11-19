#include "ElaineEditor.h"
#include "ElaineLogSystem.h"
#include "ElaineRoot.h"
#include "render/ElaineWindowSystem.h"
#include "ElaineInputSystem.h"

namespace Elaine
{
	ElaineEditor::ElaineEditor(ElaineEngine* engine)
		: m_pEngine(engine)
	{

	}

	ElaineEditor::~ElaineEditor()
	{
		close();
	}

	void ElaineEditor::initialize()
	{
		LOG_INFO("Editor Init");
	}

	void ElaineEditor::close()
	{
		LOG_INFO("Editor Close");
	}

	void ElaineEditor::tick()
	{
		while (!WindowSystem::instance()->shouldClose(WindowSystem::instance()->getMainWindow()))
		{
			InputSystem::instance()->PollEvent();
			m_pEngine->tickOneFrame();
		}
	}
}