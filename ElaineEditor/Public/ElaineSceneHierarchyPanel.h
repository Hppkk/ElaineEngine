#pragma once
#include "ElaineEditorBase.h"

namespace Elaine { class World; class GameObject; }

namespace Editor
{
	class EditorUI;

	// ============================================================
	// SceneHierarchyPanel — shows the GameObject tree
	// ============================================================
	class SceneHierarchyPanel : public EditorPanel
	{
	public:
		SceneHierarchyPanel(EditorUI* ui)
			: EditorPanel("Scene Hierarchy"), mUI(ui) {}

		void OnDraw() override;
	private:
		void DrawGameObjectNode(Elaine::GameObject* obj);
		EditorUI* mUI = nullptr;
	};
}
