#pragma once
#include "ElaineEditorBase.h"

namespace Elaine { class World; class Actor; }

namespace Editor
{
	class EditorUI;

	// ============================================================
	// SceneHierarchyPanel — shows the Actor tree
	// ============================================================
	class SceneHierarchyPanel : public EditorPanel
	{
	public:
		SceneHierarchyPanel(EditorUI* ui)
			: EditorPanel("Scene Hierarchy"), mUI(ui) {}

		void OnDraw() override;
	private:
		void DrawActorNode(Elaine::Actor* obj);
		EditorUI* mUI = nullptr;
	};
}
