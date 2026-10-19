#pragma once
#include "math/ElaineVector3.h"
#include "ElaineInputRouter.h"

namespace Elaine
{
	class CameraComponent;
	class Actor;
}

namespace Editor
{
	// ============================================================
	// EditorCameraController
	//
	// Unity-style editor Scene View camera controller.
	// Handles fly-mode, orbit, pan, zoom, and focus controls.
	//
	// Usage: Call Tick() after routing, including inactive frames to reset gestures.
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

		// Call once per frame with the viewport's routed input.
		// deltaTime: frame time in seconds
		// inputActive: whether the viewport is enabled and its gesture is valid
		// camera: the editor scene camera to manipulate
		// selectedObj: currently selected object (for focus/orbit)
		void Tick(float deltaTime,
				  bool inputActive,
				  const Elaine::InputState& input,
				  Elaine::CameraComponent* camera,
				  Elaine::Actor* selectedObj);

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
		bool  mMouseDragStarted = false;

		// Orbit pivot point
		Elaine::Vector3 mOrbitPivot = Elaine::Vector3::ZERO;

		// Helpers
		void HandleFlyMode(float deltaTime, const Elaine::InputState& input, Elaine::CameraComponent* camera);
		void HandlePanMode(float dx, float dy, Elaine::CameraComponent* camera);
		void HandleOrbitMode(float dx, float dy, Elaine::CameraComponent* camera);
		void HandleZoom(float scrollDelta, Elaine::CameraComponent* camera);
		void HandleFocus(Elaine::CameraComponent* camera, Elaine::Actor* selectedObj);
	};
}
