#pragma once
#include "ElaineEditorBase.h"
#include "ElaineEditorCameraController.h"
#include "ElaineViewportFrame.h"
#include "ElaineInputSystem.h"
#include <d3d11.h>

struct ImDrawList;

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
        ~ViewportPanel() override;

		void OnDraw() override;
        void InitializeInput();
        void BeginInputFrame();
        void UpdateInputLayers();
        void ProcessInput();
        bool ContainsInput(const Elaine::InputEvent& event) const;
        bool WantsKeyboard() const { return mImageVisible && mWindowFocused; }
        bool IsDragging() const;
        const Elaine::InputState& GetSceneInputState() const { return Elaine::InputSystem::instance()->GetState(mSceneInput); }
        const Elaine::InputRect& GetImageRect() const { return mImageRect; }
        Elaine::InputWindowId GetInputWindow() const { return mInputWindow; }

		// Set the texture to display (from engine RTT readback)
		void SetViewportTexture(ID3D11ShaderResourceView* srv, int w, int h)
		{
			mViewportSRV = srv;
			mTexWidth = w;
			mTexHeight = h;
		}

	private:
        Elaine::InputLayerHandle mSceneInput, mGizmoInput;
        Elaine::InputLayerDesc mSceneDesc, mGizmoDesc;
        Elaine::InputRect mImageRect;
        Elaine::InputWindowId mInputWindow = 0, mRegisteredWindow = 0;
        ImDrawList* mDrawList = nullptr;
        unsigned int mImageWindowId = 0;
        bool mImageVisible = false, mWindowFocused = false, mGizmoHit = false;
        Elaine::Actor* mGizmoActor = nullptr; // Identity only; never dereferenced across frames.
        bool mGizmoBlocked = false;
        bool mSelectionActive = false, mAltDown = false, mRightDown = false;
		ID3D11ShaderResourceView* mViewportSRV = nullptr;
		int mTexWidth = 0;
		int mTexHeight = 0;

		// Structured camera controller
		ViewportFrameData mDisplayedFrame;
		EditorCameraController mCameraController;

		// Box selection state
		bool mIsBoxSelecting = false;
		float mBoxSelectStartX = 0.0f;
		float mBoxSelectStartY = 0.0f;
	};
}
