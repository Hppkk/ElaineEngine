#pragma once
#include "math/ElaineVector3.h"

namespace Elaine
{
	class CameraComponent;
	class GameObject;
}

namespace Editor
{
	// ============================================================
	// EditorCameraController
	//
	// Unity-style editor Scene View camera controller.
	// Handles fly-mode, orbit, pan, zoom, and focus controls.
	//
	// Usage: Call Tick() once per frame from ViewportPanel when
	//        the viewport window is hovered/focused.
	//
	// Controls:
	//   Right-Click + WASD/QE  : Fly mode (FPS-style move + look)
	//   Middle-Click + Drag    : Pan (move on local XY plane)
	//   Alt + Left-Click + Drag: Orbit around focus point
	//   Scroll Wheel           : Zoom (dolly forward/backward)
	//   F                      : Focus on selected object
	//   Alt + Scroll           : Zoom speed adjustment
	// ============================================================
	class EditorCameraController
	{
	public:
		EditorCameraController() = default;

		// Call once per frame while the viewport is hovered/active.
		// deltaTime: frame time in seconds
		// viewportHovered: is the viewport window hovered
		// camera: the editor scene camera to manipulate
		// selectedObj: currently selected object (for focus/orbit)
		void Tick(float deltaTime,
				  bool viewportHovered,
				  Elaine::CameraComponent* camera,
				  Elaine::GameObject* selectedObj);

		// ============================================================
		// Tunable parameters
		// ============================================================
		struct Settings
		{
			float flySpeed       = 5.0f;   // Base fly speed (units/sec)
			float flySpeedFast   = 15.0f;  // Fly speed while holding Shift
			float flySpeedSlow   = 1.5f;   // Fly speed while holding Ctrl
			float mouseSensitivity = 0.15f; // Degrees per pixel for look
			float panSpeed       = 0.01f;  // Pan speed multiplier
			float orbitSpeed     = 0.25f;  // Orbit speed (degrees/pixel)
			float zoomSpeed      = 1.0f;   // Scroll zoom speed (units/click)
			float focusDistance   = 5.0f;   // Default distance when focusing
		};

		Settings& GetSettings() { return mSettings; }
		const Settings& GetSettings() const { return mSettings; }

	private:
		// Internal modes
		enum class ECameraMode
		{
			None,
			Fly,        // Right-click + WASD
			Pan,        // Middle-click drag
			Orbit       // Alt + Left-click drag
		};

		Settings    mSettings;
		ECameraMode mCurrentMode = ECameraMode::None;

		// Mouse state for delta computation
		float mLastMouseX = 0.0f;
		float mLastMouseY = 0.0f;
		bool  mMouseDragStarted = false;

		// Orbit pivot point
		Elaine::Vector3 mOrbitPivot = Elaine::Vector3::ZERO;

		// Helpers
		void HandleFlyMode(float deltaTime, Elaine::CameraComponent* camera);
		void HandlePanMode(float dx, float dy, Elaine::CameraComponent* camera);
		void HandleOrbitMode(float dx, float dy, Elaine::CameraComponent* camera);
		void HandleZoom(float scrollDelta, Elaine::CameraComponent* camera);
		void HandleFocus(Elaine::CameraComponent* camera, Elaine::GameObject* selectedObj);
	};
}
