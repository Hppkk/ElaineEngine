#pragma once
#include "ElaineEditorPrerequirements.h"
#include "ElainePlatformWindow.h"

namespace Elaine
{
	class ElaineEngine;
	class World;
}

namespace Editor
{
	class ImGuiLayer;
	class EditorUI;
	class SceneHierarchyPanel;
	class InspectorPanel;
	class ConsolePanel;
	class ViewportPanel;
	class EditorGlobalContext;
	class ContentBrowserPanel;

	// ============================================================
	// ElaineEditor — main editor application
	// ============================================================
	class ElaineEditor
	{
	public:
		ElaineEditor(Elaine::ElaineEngine* InEngine);
		virtual ~ElaineEditor();

		// Initialize: creates DX11 ImGui on the PlatformWindow's native handle
		bool Initialize(Elaine::PlatformWindow* window);
		void Destroy();
		void Run();               // Main loop — polls window events + ticks
		void Tick();              // Single frame: ImGui BeginFrame → Draw → EndFrame

		// Access panels
		ConsolePanel* GetConsole() const { return mConsolePanel; }

	private:
		Elaine::ElaineEngine*   mEngineImpl = nullptr;
		Elaine::PlatformWindow* mMainWindow = nullptr;
		ImGuiLayer*             mImGuiLayer = nullptr;
		EditorUI*               mEditorUI = nullptr;

		// Panels (owned)
		SceneHierarchyPanel*    mHierarchyPanel = nullptr;
		InspectorPanel*         mInspectorPanel = nullptr;
		ConsolePanel*           mConsolePanel = nullptr;
		ViewportPanel*          mViewportPanel = nullptr;
		ContentBrowserPanel*    mContentBrowserPanel = nullptr;
	};
}