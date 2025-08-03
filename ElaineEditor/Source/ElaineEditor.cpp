#include "ElaineCoreMacroDefinition.h"
#include "ElaineEditor.h"
#include "ElaineLogSystem.h"
#include "ElaineRoot.h"
#include "render/ElaineWindowSystem.h"
#include "ElaineInputSystem.h"

namespace ElaineEditor
{
	using namespace Elaine;

	ElaineEditor::ElaineEditor(Elaine::ElaineEngine* InEngine)
		: mEngineImpl(InEngine)
	{

	}

	ElaineEditor::~ElaineEditor()
	{
		Destroy();
	}

	void ElaineEditor::Initilize()
	{
		LOG_INFO("Elaine Engine Editor Initilize...");
		new EditorGlobalContext();
		EditorGlobalContext::instance()->Initilize();
		LOG_INFO("Elaine Engine Editor Initilize Complete!");
	}

	void ElaineEditor::Destroy()
	{
		delete EditorGlobalContext::instance();
		LOG_INFO("Elaine Engine Editor Destroy!");
	}

	void ElaineEditor::Tick()
	{
		InputSystem::instance()->PollEvent();
		mEngineImpl->RenderOneFrame();
	}
}