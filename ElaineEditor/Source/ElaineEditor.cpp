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
		LOG_INFO("Elaine Engine Editor Initilize...");
		LOG_INFO("Elaine Engine Editor Initilize Complete!");
	}

	void ElaineEditor::close()
	{
		LOG_INFO("Elaine Engine Editor Destroy!");
	}

	void ElaineEditor::tick()
	{

		{
			InputSystem::instance()->PollEvent();
			m_pEngine->RenderOneFrame();
		}
	}
}