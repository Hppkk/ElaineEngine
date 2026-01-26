#pragma once
#include "ElaineEditorPrerequirements.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineEngine.h"
#include "ElainePlatformWindow.h"

namespace Editor
{
	class ElaineEditor
	{
	public:
		ElaineEditor(Elaine::ElaineEngine* InEngine);
		virtual ~ElaineEditor();
		void Initialize();
		void Destroy();
		void Tick();
		void Run();
		void SetWindow(Elaine::PlatformWindow* InWindow);
	private:
		Elaine::ElaineEngine* mEngineImpl = nullptr;
		Elaine::PlatformWindow* mMainWindow = nullptr;
	};
}