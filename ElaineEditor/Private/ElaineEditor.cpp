#include "ElaineCoreMacroDefinition.h"
#include "ElaineEditor.h"
#include "ElaineLogSystem.h"
#include "ElaineRoot.h"
#include "ElaineInputSystem.h"
#include "ElaineViewport.h"

namespace Editor
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
		RenderToLogic_Barrier->Wait();
		InputSystem::instance()->PollEvent();
		mEngineImpl->RenderOneFrame();
		LogicToRender_Barrier->Signal();
	}

	void ElaineEditor::Run()
	{



		while (!mMainWindow->shouldClose())
		{
			mMainWindow->pollEvents();
			Tick();
			mMainWindow->setTitle(std::format("ElaineEditor [Logic FPS]: {} [Render FPS]: {}", mEngineImpl->GetFPS(), Root::instance()->GetFPS()));
		}


	}

	void ElaineEditor::SetWindow(Elaine::PlatformWindow* InWindow)
	{
		mMainWindow = InWindow;
	}
}