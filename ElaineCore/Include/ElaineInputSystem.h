#pragma once
#include "ElaineCorePrerequirements.h"
#include "Common/ElaineInputTypes.h"
#include <functional>
#include <vector>
#include <map>
#include <algorithm>

namespace Elaine
{
	// 返回 true 表示事件被消费，不再向低优先级层传播
	using KeyCallback = std::function<bool(EKeyCode, EInputAction)>;
	using MouseButtonCallback = std::function<bool(EMouseButton, EInputAction)>;
	using MouseMoveCallback = std::function<bool(float, float)>;
	using MouseScrollCallback = std::function<bool(float, float)>;

	// 输入层级，数值越小优先级越高
	enum class EInputLayer : uint8_t
	{
		Editor = 0,   // 最高优先级：编辑器 UI（ImGui WantCapture*）
		Gizmo  = 1,   // Gizmo 操控
		Game   = 2,   // 游戏/场景逻辑
		Max
	};

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

		// ============================================================
		// 带层级的回调注册
		// layer 参数控制优先级，默认为 Game 层
		// ============================================================
		uint32_t RegisterKeyCallback(KeyCallback callback, EInputLayer layer = EInputLayer::Game);
		uint32_t RegisterMouseButtonCallback(MouseButtonCallback callback, EInputLayer layer = EInputLayer::Game);
		uint32_t RegisterMouseMoveCallback(MouseMoveCallback callback, EInputLayer layer = EInputLayer::Game);
		uint32_t RegisterMouseScrollCallback(MouseScrollCallback callback, EInputLayer layer = EInputLayer::Game);

		void UnregisterKeyCallback(uint32_t id);
		void UnregisterMouseButtonCallback(uint32_t id);
		void UnregisterMouseMoveCallback(uint32_t id);
		void UnregisterMouseScrollCallback(uint32_t id);

		// ============================================================
		// UI 输入阻断标志
		// 当 ImGui WantCapture* 为 true 时由编辑器设置，
		// InputSystem 会跳过 Game/Gizmo 层的分发
		// ============================================================
		void SetUIWantsKeyboard(bool wants) { mUIWantsKeyboard = wants; }
		void SetUIWantsMouse(bool wants) { mUIWantsMouse = wants; }
		bool GetUIWantsKeyboard() const { return mUIWantsKeyboard; }
		bool GetUIWantsMouse() const { return mUIWantsMouse; }

		// Platform hooks
		void OnKeyPressed(EKeyCode key);
		void OnKeyReleased(EKeyCode key);
		void OnMouseButtonPressed(EMouseButton button);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseMove(float x, float y);
		void OnMouseScroll(float xOffset, float yOffset);

	private:
		struct LayeredCallback
		{
			uint32_t    id;
			EInputLayer layer;
		};

		template<typename CallbackType>
		struct CallbackEntry
		{
			uint32_t     id;
			EInputLayer  layer;
			CallbackType callback;
		};

		template<typename CallbackType>
		using CallbackList = std::vector<CallbackEntry<CallbackType>>;

		// 按 layer 排序的回调列表
		CallbackList<KeyCallback>         mKeyCallbacks;
		CallbackList<MouseButtonCallback> mMouseButtonCallbacks;
		CallbackList<MouseMoveCallback>   mMouseMoveCallbacks;
		CallbackList<MouseScrollCallback> mMouseScrollCallbacks;

		bool mKeyStates[static_cast<int>(EKeyCode::Max)] = { false };
		bool mMouseButtonStates[static_cast<int>(EMouseButton::Max)] = { false };
		float mMouseX = 0.0f;
		float mMouseY = 0.0f;

		uint32_t mNextCallbackId = 1;

		// UI 阻断标志
		bool mUIWantsKeyboard = false;
		bool mUIWantsMouse = false;

		// 辅助：插入并保持按 layer 排序
		template<typename CallbackType>
		uint32_t InsertCallback(CallbackList<CallbackType>& list, CallbackType callback, EInputLayer layer);

		template<typename CallbackType>
		void RemoveCallback(CallbackList<CallbackType>& list, uint32_t id);
	};
}