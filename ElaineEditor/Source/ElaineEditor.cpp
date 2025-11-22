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

	void ElaineEditor::Initialize()
	{
		LOG_INFO("Elaine Engine Editor Initialize...");
		new EditorGlobalContext();
		EditorGlobalContext::instance()->Initialize();
		LOG_INFO("Elaine Engine Editor Initialize Complete!");
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