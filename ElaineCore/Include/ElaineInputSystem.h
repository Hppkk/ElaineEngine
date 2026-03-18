#pragma once
#include "ElaineCorePrerequirements.h"
#include "Common/ElaineInputTypes.h"
#include <functional>
#include <vector>
#include <map>

namespace Elaine
{
	using KeyCallback = std::function<void(EKeyCode, EInputAction)>;
	using MouseButtonCallback = std::function<void(EMouseButton, EInputAction)>;
	using MouseMoveCallback = std::function<void(float, float)>;
	using MouseScrollCallback = std::function<void(float, float)>;

	class ElaineCoreExport InputSystem : public Singleton<InputSystem>
	{
	public:
		InputSystem();

		void PollEvent(); // Called per frame

		// States
		bool IsKeyDown(EKeyCode key) const;
		bool IsMouseButtonDown(EMouseButton button) const;
		float GetMouseX() const { return mMouseX; }
		float GetMouseY() const { return mMouseY; }

		// Delegates
		uint32_t RegisterKeyCallback(KeyCallback callback);
		uint32_t RegisterMouseButtonCallback(MouseButtonCallback callback);
		uint32_t RegisterMouseMoveCallback(MouseMoveCallback callback);
		uint32_t RegisterMouseScrollCallback(MouseScrollCallback callback);

		void UnregisterKeyCallback(uint32_t id);
		void UnregisterMouseButtonCallback(uint32_t id);
		void UnregisterMouseMoveCallback(uint32_t id);
		void UnregisterMouseScrollCallback(uint32_t id);

		// Platform hooks
		void OnKeyPressed(EKeyCode key);
		void OnKeyReleased(EKeyCode key);
		void OnMouseButtonPressed(EMouseButton button);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseMove(float x, float y);
		void OnMouseScroll(float xOffset, float yOffset);

	private:
		bool mKeyStates[static_cast<int>(EKeyCode::Max)] = { false };
		bool mMouseButtonStates[static_cast<int>(EMouseButton::Max)] = { false };
		float mMouseX = 0.0f;
		float mMouseY = 0.0f;

		uint32_t mNextCallbackId = 1;
		std::map<uint32_t, KeyCallback> mKeyCallbacks;
		std::map<uint32_t, MouseButtonCallback> mMouseButtonCallbacks;
		std::map<uint32_t, MouseMoveCallback> mMouseMoveCallbacks;
		std::map<uint32_t, MouseScrollCallback> mMouseScrollCallbacks;
	};
}