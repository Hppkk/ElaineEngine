#include "ElaineEditorCameraController.h"
#include "GamePlay/ElaineCameraComponent.h"
#include "GamePlay/ElaineActor.h"

#include "math/ElaineQuaternion.h"
#include "math/ElaineVector3.h"
#include "math/ElaineMath.h"

namespace Editor
{
	void EditorCameraController::Tick(
		float deltaTime,
		bool inputActive,
        const Elaine::InputState& input,
		Elaine::CameraComponent* camera,
		Elaine::Actor* selectedObj)
	{
		if (!camera || !inputActive)
		{
			// Reset drag state when not active
			mCurrentMode = ECameraMode::None;
			mMouseDragStarted = false;
			return;
		}

		using namespace Elaine;

		// ============================================================
		// Determine current mode from input
		// ============================================================
		bool altDown   = (input.IsKeyDown(EKeyCode::LeftAlt) || input.IsKeyDown(EKeyCode::RightAlt));
		bool rightDown = input.IsMouseDown(EMouseButton::Right);
		bool midDown   = input.IsMouseDown(EMouseButton::Middle);
		bool leftDown  = input.IsMouseDown(EMouseButton::Left);

		ECameraMode desiredMode = ECameraMode::None;

		if (rightDown)
			desiredMode = ECameraMode::Fly;
		else if (midDown)
			desiredMode = ECameraMode::Pan;
		else if (altDown && leftDown)
			desiredMode = ECameraMode::Orbit;

		// Mode transition
		if (desiredMode != mCurrentMode)
		{
			mCurrentMode = desiredMode;
			mMouseDragStarted = false;

			// Set orbit pivot on entering orbit mode
			if (mCurrentMode == ECameraMode::Orbit)
			{
				if (selectedObj)
					mOrbitPivot = selectedObj->GetPosition();
				else
					mOrbitPivot = camera->GetPosition() + camera->GetForward() * mSettings.focusDistance;
			}
		}

		// ============================================================
		// Compute mouse delta
		// ============================================================
		float dx = 0.0f, dy = 0.0f;

		if (mMouseDragStarted)
		{
			dx = input.MouseDeltaX;
			dy = input.MouseDeltaY;
		}
		mMouseDragStarted = true;

		// ============================================================
		// Dispatch to mode handler
		// ============================================================
		switch (mCurrentMode)
		{
		case ECameraMode::Fly:
			HandleFlyMode(deltaTime, input, camera);
			// Also rotate camera with mouse delta
			if (dx != 0.0f || dy != 0.0f)
			{
				float yawDeg   = -dx * mSettings.mouseSensitivity;
				float pitchDeg = -dy * mSettings.mouseSensitivity;

				Elaine::Quaternion yawRot(Elaine::Radian(Elaine::Degree(yawDeg)), Elaine::Vector3::UNIT_Y);
				Elaine::Quaternion pitchRot(Elaine::Radian(Elaine::Degree(pitchDeg)), camera->GetRight());

				Elaine::Quaternion newRot = yawRot * camera->GetRotation() * pitchRot;
				newRot.normalise();
				camera->SetRotation(newRot);
			}
			break;

		case ECameraMode::Pan:
			HandlePanMode(dx, dy, camera);
			break;

		case ECameraMode::Orbit:
			HandleOrbitMode(dx, dy, camera);
			break;

		case ECameraMode::None:
			mMouseDragStarted = false;
			break;
		}

		// ============================================================
		// Scroll zoom (always available when hovered, no mode required)
		// ============================================================
		if (input.ScrollY != 0.0f)
		{
			HandleZoom(input.ScrollY, camera);
		}

		// ============================================================
		// F = Focus on selection
		// ============================================================
		if (input.IsKeyPressed(EKeyCode::F))
		{
			HandleFocus(camera, selectedObj);
		}
	}

	// ============================================================
	// Fly Mode — FPS-style WASD + QE
	// ============================================================
	void EditorCameraController::HandleFlyMode(float deltaTime, const Elaine::InputState& input, Elaine::CameraComponent* camera)
	{
		float speed = mSettings.flySpeed;
		if ((input.IsKeyDown(Elaine::EKeyCode::LeftShift) || input.IsKeyDown(Elaine::EKeyCode::RightShift)))
			speed = mSettings.flySpeedFast;
		else if ((input.IsKeyDown(Elaine::EKeyCode::LeftControl) || input.IsKeyDown(Elaine::EKeyCode::RightControl)))
			speed = mSettings.flySpeedSlow;

		Elaine::Vector3 move = Elaine::Vector3::ZERO;
		Elaine::Vector3 forward = camera->GetForward();
		Elaine::Vector3 right   = camera->GetRight();
		Elaine::Vector3 up      = Elaine::Vector3::UNIT_Y;

		if (input.IsKeyDown(Elaine::EKeyCode::W))  move += forward;
		if (input.IsKeyDown(Elaine::EKeyCode::S))  move -= forward;
		if (input.IsKeyDown(Elaine::EKeyCode::D))  move += right;
		if (input.IsKeyDown(Elaine::EKeyCode::A))  move -= right;
		if (input.IsKeyDown(Elaine::EKeyCode::E))  move += up;
		if (input.IsKeyDown(Elaine::EKeyCode::Q))  move -= up;

		if (move != Elaine::Vector3::ZERO)
		{
			move.normalise();
			camera->SetPosition(camera->GetPosition() + move * speed * deltaTime);
		}
	}

	// ============================================================
	// Pan Mode — slide camera on its local XY plane
	// ============================================================
	void EditorCameraController::HandlePanMode(float dx, float dy, Elaine::CameraComponent* camera)
	{
		Elaine::Vector3 right = camera->GetRight();
		Elaine::Vector3 up    = camera->GetUp();

		Elaine::Vector3 offset = (-right * dx + up * dy) * mSettings.panSpeed;
		camera->SetPosition(camera->GetPosition() + offset);
	}

	// ============================================================
	// Orbit Mode — rotate around a pivot point
	// ============================================================
	void EditorCameraController::HandleOrbitMode(float dx, float dy, Elaine::CameraComponent* camera)
	{
		if (dx == 0.0f && dy == 0.0f) return;

		float yawDeg   = -dx * mSettings.orbitSpeed;
		float pitchDeg = -dy * mSettings.orbitSpeed;

		Elaine::Vector3 offset = camera->GetPosition() - mOrbitPivot;
		float distance = offset.length();
		if (distance < 0.001f) return;

		// Yaw around world up
		Elaine::Quaternion yawRot(Elaine::Radian(Elaine::Degree(yawDeg)), Elaine::Vector3::UNIT_Y);
		offset = yawRot * offset;

		// Pitch around camera right
		Elaine::Quaternion pitchRot(Elaine::Radian(Elaine::Degree(pitchDeg)), camera->GetRight());
		offset = pitchRot * offset;

		// Prevent flipping at poles
		Elaine::Vector3 newDir = offset.normalisedCopy();
		float dotUp = newDir.dotProduct(Elaine::Vector3::UNIT_Y);
		if (std::abs(dotUp) > 0.99f) return;

		camera->SetPosition(mOrbitPivot + offset);
		camera->LookAt(mOrbitPivot);
	}

	// ============================================================
	// Zoom — dolly forward/backward along view direction
	// ============================================================
	void EditorCameraController::HandleZoom(float scrollDelta, Elaine::CameraComponent* camera)
	{
		Elaine::Vector3 forward = camera->GetForward();
		camera->SetPosition(camera->GetPosition() + forward * scrollDelta * mSettings.zoomSpeed);
	}

	// ============================================================
	// Focus — move camera to look at the selected object
	// ============================================================
	void EditorCameraController::HandleFocus(Elaine::CameraComponent* camera, Elaine::Actor* selectedObj)
	{
		if (!selectedObj) return;

		Elaine::Vector3 targetPos = selectedObj->GetPosition();
		Elaine::Vector3 currentForward = camera->GetForward();

		// Place camera at a distance from the object, looking at it
		camera->SetPosition(targetPos - currentForward * mSettings.focusDistance);
		camera->LookAt(targetPos);

		mOrbitPivot = targetPos;
	}
}
