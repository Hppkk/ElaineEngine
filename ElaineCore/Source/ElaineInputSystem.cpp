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

	// ============================================================
	// 回调注册（带层级）
	// ============================================================
	uint32_t InputSystem::RegisterKeyCallback(KeyCallback callback, EInputLayer layer)
	{
		return InsertCallback(mKeyCallbacks, std::move(callback), layer);
	}

	uint32_t InputSystem::RegisterMouseButtonCallback(MouseButtonCallback callback, EInputLayer layer)
	{
		return InsertCallback(mMouseButtonCallbacks, std::move(callback), layer);
	}

	uint32_t InputSystem::RegisterMouseMoveCallback(MouseMoveCallback callback, EInputLayer layer)
	{
		return InsertCallback(mMouseMoveCallbacks, std::move(callback), layer);
	}

	uint32_t InputSystem::RegisterMouseScrollCallback(MouseScrollCallback callback, EInputLayer layer)
	{
		return InsertCallback(mMouseScrollCallbacks, std::move(callback), layer);
	}

	void InputSystem::UnregisterKeyCallback(uint32_t id)         { RemoveCallback(mKeyCallbacks, id); }
	void InputSystem::UnregisterMouseButtonCallback(uint32_t id) { RemoveCallback(mMouseButtonCallbacks, id); }
	void InputSystem::UnregisterMouseMoveCallback(uint32_t id)   { RemoveCallback(mMouseMoveCallbacks, id); }
	void InputSystem::UnregisterMouseScrollCallback(uint32_t id) { RemoveCallback(mMouseScrollCallbacks, id); }

	// ============================================================
	// Platform hooks — 按 layer 顺序分发，消费则停止
	// ============================================================
	void InputSystem::OnKeyPressed(EKeyCode key)
	{
		int k = static_cast<int>(key);
		if (k >= 0 && k < static_cast<int>(EKeyCode::Max))
		{
			EInputAction action = mKeyStates[k] ? EInputAction::Repeat : EInputAction::Press;
			mKeyStates[k] = true;

			for (auto& entry : mKeyCallbacks)
			{
				// 如果 UI 想要键盘输入，跳过非 Editor 层
				if (mUIWantsKeyboard && entry.layer > EInputLayer::Editor)
					break;

				if (entry.callback(key, action))
					break; // 事件被消费
			}
		}
	}

	void InputSystem::OnKeyReleased(EKeyCode key)
	{
		int k = static_cast<int>(key);
		if (k >= 0 && k < static_cast<int>(EKeyCode::Max))
		{
			mKeyStates[k] = false;

			for (auto& entry : mKeyCallbacks)
			{
				if (mUIWantsKeyboard && entry.layer > EInputLayer::Editor)
					break;

				if (entry.callback(key, EInputAction::Release))
					break;
			}
		}
	}

	void InputSystem::OnMouseButtonPressed(EMouseButton button)
	{
		int b = static_cast<int>(button);
		if (b >= 0 && b < static_cast<int>(EMouseButton::Max))
		{
			mMouseButtonStates[b] = true;

			for (auto& entry : mMouseButtonCallbacks)
			{
				if (mUIWantsMouse && entry.layer > EInputLayer::Editor)
					break;

				if (entry.callback(button, EInputAction::Press))
					break;
			}
		}
	}

	void InputSystem::OnMouseButtonReleased(EMouseButton button)
	{
		int b = static_cast<int>(button);
		if (b >= 0 && b < static_cast<int>(EMouseButton::Max))
		{
			mMouseButtonStates[b] = false;

			for (auto& entry : mMouseButtonCallbacks)
			{
				if (mUIWantsMouse && entry.layer > EInputLayer::Editor)
					break;

				if (entry.callback(button, EInputAction::Release))
					break;
			}
		}
	}

	void InputSystem::OnMouseMove(float x, float y)
	{
		mMouseX = x;
		mMouseY = y;

		for (auto& entry : mMouseMoveCallbacks)
		{
			if (mUIWantsMouse && entry.layer > EInputLayer::Editor)
				break;

			if (entry.callback(x, y))
				break;
		}
	}

	void InputSystem::OnMouseScroll(float xOffset, float yOffset)
	{
		for (auto& entry : mMouseScrollCallbacks)
		{
			if (mUIWantsMouse && entry.layer > EInputLayer::Editor)
				break;

			if (entry.callback(xOffset, yOffset))
				break;
		}
	}

	// ============================================================
	// Private helpers
	// ============================================================
	template<typename CallbackType>
	uint32_t InputSystem::InsertCallback(CallbackList<CallbackType>& list, CallbackType callback, EInputLayer layer)
	{
		uint32_t id = mNextCallbackId++;
		CallbackEntry<CallbackType> entry{ id, layer, std::move(callback) };

		// 保持按 layer 排序（稳定插入）
		auto it = std::lower_bound(list.begin(), list.end(), entry,
			[](const CallbackEntry<CallbackType>& a, const CallbackEntry<CallbackType>& b) {
				return static_cast<uint8_t>(a.layer) < static_cast<uint8_t>(b.layer);
			});
		list.insert(it, std::move(entry));
		return id;
	}

	template<typename CallbackType>
	void InputSystem::RemoveCallback(CallbackList<CallbackType>& list, uint32_t id)
	{
		list.erase(
			std::remove_if(list.begin(), list.end(),
				[id](const CallbackEntry<CallbackType>& e) { return e.id == id; }),
			list.end());
	}

	// 显式实例化模板
	template uint32_t InputSystem::InsertCallback<KeyCallback>(CallbackList<KeyCallback>&, KeyCallback, EInputLayer);
	template uint32_t InputSystem::InsertCallback<MouseButtonCallback>(CallbackList<MouseButtonCallback>&, MouseButtonCallback, EInputLayer);
	template uint32_t InputSystem::InsertCallback<MouseMoveCallback>(CallbackList<MouseMoveCallback>&, MouseMoveCallback, EInputLayer);
	template uint32_t InputSystem::InsertCallback<MouseScrollCallback>(CallbackList<MouseScrollCallback>&, MouseScrollCallback, EInputLayer);
	template void InputSystem::RemoveCallback<KeyCallback>(CallbackList<KeyCallback>&, uint32_t);
	template void InputSystem::RemoveCallback<MouseButtonCallback>(CallbackList<MouseButtonCallback>&, uint32_t);
	template void InputSystem::RemoveCallback<MouseMoveCallback>(CallbackList<MouseMoveCallback>&, uint32_t);
	template void InputSystem::RemoveCallback<MouseScrollCallback>(CallbackList<MouseScrollCallback>&, uint32_t);
}