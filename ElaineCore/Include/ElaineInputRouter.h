#pragma once
#include "Common/ElaineInputTypes.h"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#ifdef ELAINE_INPUT_STANDALONE
#define ELAINE_INPUT_API
#else
#include "Common/ElaineCorePrerequirements.h"
#define ELAINE_INPUT_API ElaineCoreExport
#endif

namespace Elaine
{
    using InputWindowId = uintptr_t;
    using InputViewportId = uint64_t;
    enum class InputDomain { Any, Scene, Game };
    enum class InputCategory : uint8_t { Keyboard, Text, Button, Move, Scroll, Count };
    enum class InputPropagation { PassThrough, StopIfHandled, Block };
    enum class InputReply { Unhandled, Handled };
    enum class InputEventType { Key, Text, Button, Move, Scroll, FocusLost, CaptureLost, WindowDestroyed, Cancel };

    struct InputRect
    {
        float Left = 0, Top = 0, Right = 0, Bottom = 0;
        bool Contains(float x, float y) const { return x >= Left && x < Right && y >= Top && y < Bottom; }
    };

    struct ELAINE_INPUT_API InputEvent
    {
        InputEventType Type = InputEventType::Move;
        InputWindowId Window = 0;
        InputViewportId Viewport = 0;
        EInputAction Action = EInputAction::Press;
        EKeyCode Key = EKeyCode::Unknown;
        EMouseButton Button = EMouseButton::Left;
        char32_t Character = 0;
        // Desktop coordinates, including on floating windows. Scroll is in wheel steps.
        float X = 0, Y = 0, DeltaX = 0, DeltaY = 0;
        // On Cancel, Key != Unknown restricts keyboard cancellation to that key.
        bool CancelKeyboard = true, CancelPointer = true;
        InputCategory Category() const;
    };

    struct InputLayerDesc
    {
        std::string Name;
        int Priority = 0; // Higher values run first; ties preserve registration order.
        InputWindowId Window = 0; // Zero matches any window.
        InputViewportId Viewport = 0; // Zero is a window-wide overlay.
        InputDomain Domain = InputDomain::Any;
        bool Enabled = true;
        bool KeyboardFocused = true;
        bool PointerEnabled = true;
        bool Modal = false; // May preempt pointer capture; use Block policies for a modal barrier.
        std::array<bool, static_cast<size_t>(InputCategory::Count)> Categories = {true, true, true, true, true};
        std::array<InputPropagation, static_cast<size_t>(InputCategory::Count)> Propagation = {
            InputPropagation::StopIfHandled, InputPropagation::StopIfHandled,
            InputPropagation::StopIfHandled, InputPropagation::StopIfHandled,
            InputPropagation::StopIfHandled };
        // Side-effect-free pointer predicate. Empty matches the whole viewport/window.
        std::function<bool(const InputEvent&)> HitTest;
    };

    struct ELAINE_INPUT_API InputState
    {
        std::array<bool, static_cast<size_t>(EKeyCode::Max)> Keys{}, PressedKeys{}, ReleasedKeys{};
        std::array<bool, static_cast<size_t>(EMouseButton::Max)> Buttons{}, PressedButtons{}, ReleasedButtons{};
        float MouseX = 0, MouseY = 0, MouseDeltaX = 0, MouseDeltaY = 0, ScrollX = 0, ScrollY = 0;
        bool Cancelled = false;
        // Preserves multiple transitions in a single frame (including down+up).
        std::vector<InputEvent> Events;
        bool IsKeyDown(EKeyCode key) const;
        bool IsKeyPressed(EKeyCode key) const;
        bool IsKeyReleased(EKeyCode key) const;
        bool IsMouseDown(EMouseButton button) const;
        bool IsMousePressed(EMouseButton button) const;
        bool IsMouseReleased(EMouseButton button) const;
    };

    struct InputRouterData;
    // Owns both the layer and its subscription. Move-only; safe after router destruction.
    class ELAINE_INPUT_API InputLayerHandle
    {
    public:
        InputLayerHandle() = default;
        ~InputLayerHandle();
        InputLayerHandle(InputLayerHandle&& other) noexcept;
        InputLayerHandle& operator=(InputLayerHandle&& other) noexcept;
        InputLayerHandle(const InputLayerHandle&) = delete;
        InputLayerHandle& operator=(const InputLayerHandle&) = delete;
        void Reset();
        explicit operator bool() const;
    private:
        friend class InputRouter;
        std::weak_ptr<InputRouterData> mOwner;
        uint64_t mId = 0;
    };

    // Main-thread service. Has no renderer, OS or ImGui dependency.
    class ELAINE_INPUT_API InputRouter
    {
    public:
        using Handler = std::function<InputReply(const InputEvent&)>;
        InputRouter();
        virtual ~InputRouter();
        InputRouter(const InputRouter&) = delete;
        InputRouter& operator=(const InputRouter&) = delete;
        InputLayerHandle RegisterLayer(InputLayerDesc desc, Handler handler = {});
        void UpdateLayer(const InputLayerHandle& layer, InputLayerDesc desc);
        const InputState& GetState(const InputLayerHandle& layer) const;
        void SetViewport(InputWindowId window, InputViewportId viewport, InputDomain domain, InputRect rect);
        void RemoveViewport(InputWindowId window, InputViewportId viewport);
        void SetKeyboardFocus(InputWindowId window, InputViewportId viewport);
        bool CapturePointer(const InputLayerHandle& layer, InputWindowId window);
        void ReleasePointer(const InputLayerHandle& layer);
        bool HasPointerCapture(const InputLayerHandle& layer) const;
        void SetPointerCaptureCallback(std::function<void(InputWindowId, bool)> callback);
        void QueueEvent(InputEvent event);
        // Per frame: BeginFrame -> collect/update UI descriptors -> DispatchEvents -> read states.
        void BeginFrame();
        void DispatchEvents();
        // Re-evaluates held inputs after UI/focus/domain changes, even without new OS events.
        void RefreshRouting();
        bool IsRawKeyDown(InputWindowId window, EKeyCode key) const;
        bool IsRawMouseDown(InputWindowId window, EMouseButton button) const;
    private:
        std::shared_ptr<InputRouterData> mData;
    };
}
