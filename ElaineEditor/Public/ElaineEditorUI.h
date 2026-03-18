#pragma once
#include "ElaineEditorBase.h"
#include <vector>
#include <memory>

namespace Elaine
{
	class ElaineEngine;
}

namespace Editor
{
	class ImGuiLayer;

	// ============================================================
	// EditorUI — manages all panels, DockSpace, and MenuBar
	// ============================================================
	class EditorUI
	{
	public:
		EditorUI() = default;
		~EditorUI() = default;

		void Initialize(Elaine::ElaineEngine* engine);

		// Register a panel to be managed
		void AddPanel(EditorPanel* panel);

		// Main draw call (called between BeginFrame / EndFrame)
		void Draw();



	private:
		void DrawDockSpace();
		void DrawMenuBar();

		std::vector<EditorPanel*> mPanels;
		Elaine::ElaineEngine*     mEngine = nullptr;
		bool                      mFirstFrame = true;
	};
}
