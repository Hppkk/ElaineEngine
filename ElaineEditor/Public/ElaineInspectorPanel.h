#pragma once
#include "ElaineEditorBase.h"

namespace Editor
{
	class EditorUI;

	// ============================================================
	// InspectorPanel — shows selected GameObject's components
	// ============================================================
	class InspectorPanel : public EditorPanel
	{
	public:
		InspectorPanel(EditorUI* ui)
			: EditorPanel("Inspector"), mUI(ui) {}

		void OnDraw() override;
	private:
		EditorUI* mUI = nullptr;
	};
}
