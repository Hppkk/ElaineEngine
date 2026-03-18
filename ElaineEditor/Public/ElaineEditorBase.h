#pragma once
#include "ElaineEditorGlobalContext.h"
#include <string>

namespace Editor
{
	class EditorGlobalContext;

	// ============================================================
	// EditorPanel — base class for all editor panels
	// ============================================================
	class EditorPanel
	{
	public:
		EditorPanel(const char* title, bool visible = true);

		virtual ~EditorPanel() = default;

		// Draw the panel content (called every frame if visible)
		virtual void OnDraw() = 0;

		// Optional lifecycle hooks
		virtual void OnOpen() {}
		virtual void OnClose() {}

		const char* GetTitle() const { return mTitle; }
		bool IsVisible() const { return mVisible; }
		void SetVisible(bool v)
		{
			if (v && !mVisible) OnOpen();
			if (!v && mVisible) OnClose();
			mVisible = v;
		}
		void ToggleVisible() { SetVisible(!mVisible); }

		// Each panel gets a unique ID for ImGui
		bool* GetVisiblePtr() { return &mVisible; }

	protected:
		const char* mTitle = "Panel";
		bool        mVisible = true;
		EditorGlobalContext* mContext = nullptr;
	};
}