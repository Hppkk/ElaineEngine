#pragma once
#include "ElaineEditorBase.h"
#include <d3d11.h>

namespace Editor
{
	// ============================================================
	// ViewportPanel — shows the 3D engine viewport (placeholder)
	// ============================================================
	class ViewportPanel : public EditorPanel
	{
	public:
		ViewportPanel()
			: EditorPanel("Viewport") {}

		void OnDraw() override;

		// Set the texture to display (from engine RTT readback)
		void SetViewportTexture(ID3D11ShaderResourceView* srv, int w, int h)
		{
			mViewportSRV = srv;
			mTexWidth = w;
			mTexHeight = h;
		}

	private:
		ID3D11ShaderResourceView* mViewportSRV = nullptr;
		int mTexWidth = 0;
		int mTexHeight = 0;

		// Gizmo state (Unity-style). Values match ImGuizmo::OPERATION / ImGuizmo::MODE; cast in .cpp.
		int mCurrentGizmoOperation = 7;  // default: TRANSLATE (1|2|4)
		int mCurrentGizmoMode = 1;        // default: WORLD (1); 0 = LOCAL
	};
}
