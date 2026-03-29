#include "ElaineEditorCameraController.h"
#include "GamePlay/ElaineCameraComponent.h"
#include "GamePlay/ElaineGameObject.h"
#include "imgui.h"
#include "math/ElaineQuaternion.h"
#include "math/ElaineVector3.h"
#include "math/ElaineMath.h"

namespace Editor
{
	void EditorCameraController::Tick(
		float deltaTime,
		bool viewportHovered,
		Elaine::CameraComponent* camera,
		Elaine::GameObject* selectedObj)
	{
		if (!camera || !viewportHovered)
		{
			// Reset drag state when not active
			mCurrentMode = ECameraMode::None;
			mMouseDragStarted = false;
			return;
		}

		ImGuiIO& io = ImGui::GetIO();

		// ============================================================
		// Determine current mode from input
		// ============================================================
		bool altDown   = io.KeyAlt;
		bool rightDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);
		bool midDown   = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
		bool leftDown  = ImGui::IsMouseDown(ImGuiMouseButton_Left);

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
		ImVec2 mousePos = ImGui::GetMousePos();
		float dx = 0.0f, dy = 0.0f;

		if (mMouseDragStarted)
		{
			dx = mousePos.x - mLastMouseX;
			dy = mousePos.y - mLastMouseY;
		}
		mLastMouseX = mousePos.x;
		mLastMouseY = mousePos.y;
		mMouseDragStarted = true;

		// ============================================================
		// Dispatch to mode handler
		// ============================================================
		switch (mCurrentMode)
		{
		case ECameraMode::Fly:
			HandleFlyMode(deltaTime, camera);
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
		if (io.MouseWheel != 0.0f)
		{
			HandleZoom(io.MouseWheel, camera);
		}

		// ============================================================
		// F = Focus on selection
		// ============================================================
		if (ImGui::IsKeyPressed(ImGuiKey_F) && !ImGui::IsAnyItemActive())
		{
			HandleFocus(camera, selectedObj);
		}
	}

	// ============================================================
	// Fly Mode — FPS-style WASD + QE
	// ============================================================
	void EditorCameraController::HandleFlyMode(float deltaTime, Elaine::CameraComponent* camera)
	{
		float speed = mSettings.flySpeed;
		if (ImGui::GetIO().KeyShift)
			speed = mSettings.flySpeedFast;
		else if (ImGui::GetIO().KeyCtrl)
			speed = mSettings.flySpeedSlow;

		Elaine::Vector3 move = Elaine::Vector3::ZERO;
		Elaine::Vector3 forward = camera->GetForward();
		Elaine::Vector3 right   = camera->GetRight();
		Elaine::Vector3 up      = Elaine::Vector3::UNIT_Y;

		if (ImGui::IsKeyDown(ImGuiKey_W))  move += forward;
		if (ImGui::IsKeyDown(ImGuiKey_S))  move -= forward;
		if (ImGui::IsKeyDown(ImGuiKey_D))  move += right;
		if (ImGui::IsKeyDown(ImGuiKey_A))  move -= right;
		if (ImGui::IsKeyDown(ImGuiKey_E))  move += up;
		if (ImGui::IsKeyDown(ImGuiKey_Q))  move -= up;

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
	void EditorCameraController::HandleFocus(Elaine::CameraComponent* camera, Elaine::GameObject* selectedObj)
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
