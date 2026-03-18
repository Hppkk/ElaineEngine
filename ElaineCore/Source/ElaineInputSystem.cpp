#include "ElainePrecompiledHeader.h"
#include "ElaineInputSystem.h"

namespace Elaine
{
	InputSystem::InputSystem()
	{
	}

	void InputSystem::PollEvent()
	{
		// Unused: Window events are processed immediately currently.
	}

	bool InputSystem::IsKeyDown(EKeyCode key) const
	{
		return mKeyStates[static_cast<int>(key)];
	}

	bool InputSystem::IsMouseButtonDown(EMouseButton button) const
	{
		return mMouseButtonStates[static_cast<int>(button)];
	}

	uint32_t InputSystem::RegisterKeyCallback(KeyCallback callback)
	{
		mKeyCallbacks[mNextCallbackId] = callback;
		return mNextCallbackId++;
	}

	uint32_t InputSystem::RegisterMouseButtonCallback(MouseButtonCallback callback)
	{
		mMouseButtonCallbacks[mNextCallbackId] = callback;
		return mNextCallbackId++;
	}

	uint32_t InputSystem::RegisterMouseMoveCallback(MouseMoveCallback callback)
	{
		mMouseMoveCallbacks[mNextCallbackId] = callback;
		return mNextCallbackId++;
	}

	uint32_t InputSystem::RegisterMouseScrollCallback(MouseScrollCallback callback)
	{
		mMouseScrollCallbacks[mNextCallbackId] = callback;
		return mNextCallbackId++;
	}

	void InputSystem::UnregisterKeyCallback(uint32_t id) { mKeyCallbacks.erase(id); }
	void InputSystem::UnregisterMouseButtonCallback(uint32_t id) { mMouseButtonCallbacks.erase(id); }
	void InputSystem::UnregisterMouseMoveCallback(uint32_t id) { mMouseMoveCallbacks.erase(id); }
	void InputSystem::UnregisterMouseScrollCallback(uint32_t id) { mMouseScrollCallbacks.erase(id); }

	void InputSystem::OnKeyPressed(EKeyCode key)
	{
		int k = static_cast<int>(key);
		if (k >= 0 && k < static_cast<int>(EKeyCode::Max))
		{
			EInputAction action = mKeyStates[k] ? EInputAction::Repeat : EInputAction::Press;
			mKeyStates[k] = true;
			for (auto& pair : mKeyCallbacks) pair.second(key, action);
		}
	}

	void InputSystem::OnKeyReleased(EKeyCode key)
	{
		int k = static_cast<int>(key);
		if (k >= 0 && k < static_cast<int>(EKeyCode::Max))
		{
			mKeyStates[k] = false;
			for (auto& pair : mKeyCallbacks) pair.second(key, EInputAction::Release);
		}
	}

	void InputSystem::OnMouseButtonPressed(EMouseButton button)
	{
		int b = static_cast<int>(button);
		if (b >= 0 && b < static_cast<int>(EMouseButton::Max))
		{
			mMouseButtonStates[b] = true;
			for (auto& pair : mMouseButtonCallbacks) pair.second(button, EInputAction::Press);
		}
	}

	void InputSystem::OnMouseButtonReleased(EMouseButton button)
	{
		int b = static_cast<int>(button);
		if (b >= 0 && b < static_cast<int>(EMouseButton::Max))
		{
			mMouseButtonStates[b] = false;
			for (auto& pair : mMouseButtonCallbacks) pair.second(button, EInputAction::Release);
		}
	}

	void InputSystem::OnMouseMove(float x, float y)
	{
		mMouseX = x;
		mMouseY = y;
		for (auto& pair : mMouseMoveCallbacks) pair.second(x, y);
	}

	void InputSystem::OnMouseScroll(float xOffset, float yOffset)
	{
		for (auto& pair : mMouseScrollCallbacks) pair.second(xOffset, yOffset);
	}
}