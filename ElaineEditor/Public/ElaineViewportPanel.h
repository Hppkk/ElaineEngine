#pragma once
#include "ElaineEditorBase.h"
#include "ElaineEditorCameraController.h"
#include <d3d11.h>

namespace Editor
{
	// ============================================================
	// ViewportPanel — shows the 3D engine viewport
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

		// Structured camera controller
		EditorCameraController mCameraController;

		// Box selection state
		bool mIsBoxSelecting = false;
		float mBoxSelectStartX = 0.0f;
		float mBoxSelectStartY = 0.0f;
	};
}
