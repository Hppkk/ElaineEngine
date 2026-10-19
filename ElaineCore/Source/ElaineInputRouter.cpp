#include "ElainePrecompiledHeader.h"
#include "ElaineInputRouter.h"
#include <algorithm>
#include <map>
#include <utility>

namespace Elaine
{
    namespace
    {
        template<class T, class K> bool Read(const T& values, K key)
        {
            const auto index = static_cast<size_t>(key);
            return index < values.size() && values[index];
        }
        bool IsPointer(const InputEvent& e)
        {
            return e.Type == InputEventType::Button || e.Type == InputEventType::Move || e.Type == InputEventType::Scroll;
        }
        struct Layer
        {
            uint64_t Id = 0;
            uint64_t FirstEvent = 0;
            bool Alive = true, Cancelling = false;
            InputLayerDesc Desc;
            InputRouter::Handler Handler;
            InputState State;
            std::array<InputEvent, static_cast<size_t>(EKeyCode::Max)> KeyOwners{};
            std::array<InputEvent, static_cast<size_t>(EMouseButton::Max)> ButtonOwners{};
        };
        struct Viewport
        {
            InputDomain Domain = InputDomain::Any;
            InputRect Rect;
        };
        struct RawState
        {
            std::array<bool, static_cast<size_t>(EKeyCode::Max)> Keys{};
            std::array<bool, static_cast<size_t>(EMouseButton::Max)> Buttons{};
            float X = 0, Y = 0;
            bool HasPosition = false;
        };
    }

    struct InputRouterData
    {
        uint64_t NextId = 1;
        uint64_t EventSerial = 0;
        bool Dispatching = false, Refreshing = false;
        std::vector<std::shared_ptr<Layer>> Layers;
        std::vector<InputEvent> Queue;
        std::vector<std::function<void()>> Pending;
        std::map<std::pair<InputWindowId, InputViewportId>, Viewport> Viewports;
        InputWindowId FocusWindow = 0;
        InputViewportId FocusViewport = 0;
        std::map<InputWindowId, RawState> Raw;
        uint64_t CaptureId = 0;
        InputWindowId CaptureWindow = 0;
        std::function<void(InputWindowId, bool)> NativeCapture;

        std::shared_ptr<Layer> Find(uint64_t id) const
        {
            for (const auto& layer : Layers) if (layer->Id == id && layer->Alive) return layer;
            return {};
        }
        std::vector<std::shared_ptr<Layer>> Ordered() const
        {
            auto result = Layers;
            result.erase(std::remove_if(result.begin(), result.end(), [this](const auto& l) {
                return !l->Alive || l->FirstEvent > EventSerial;
            }), result.end());
            std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
                return a->Desc.Priority > b->Desc.Priority;
            });
            return result;
        }
        void DropCapture()
        {
            if (!CaptureId) return;
            const auto window = CaptureWindow;
            CaptureId = 0;
            CaptureWindow = 0;
            if (NativeCapture) NativeCapture(window, false);
        }
        void Cancel(const std::shared_ptr<Layer>& layer, bool keyboard = true, bool pointer = true, EKeyCode key = EKeyCode::Unknown)
        {
            if (layer->Cancelling) return;
            auto& s = layer->State;
            const bool held = (keyboard && (key == EKeyCode::Unknown ? std::any_of(s.Keys.begin(), s.Keys.end(), [](bool v) { return v; }) : Read(s.Keys, key))) ||
                (pointer && (std::any_of(s.Buttons.begin(), s.Buttons.end(), [](bool v) { return v; }) || CaptureId == layer->Id));
            // Also invalidate this frame's edges, so a newly opened modal cannot leak a click.
            const auto affected = [keyboard, pointer, key](const InputEvent& e) {
                return (keyboard && ((e.Type == InputEventType::Key && (key == EKeyCode::Unknown || key == e.Key)) ||
                    (key == EKeyCode::Unknown && e.Type == InputEventType::Text))) || (pointer && IsPointer(e));
            };
            const bool received = std::any_of(s.Events.begin(), s.Events.end(), affected);
            if (!held && !received) return;
            layer->Cancelling = true;
            InputWindowId sourceWindow = layer->Desc.Window;
            InputViewportId sourceViewport = layer->Desc.Viewport;
            for (size_t k = 0; keyboard && k < s.Keys.size(); ++k)
                if (s.Keys[k] && (key == EKeyCode::Unknown || static_cast<size_t>(key) == k))
                { sourceWindow = layer->KeyOwners[k].Window; sourceViewport = layer->KeyOwners[k].Viewport; break; }
            for (size_t b = 0; pointer && b < s.Buttons.size(); ++b)
                if (s.Buttons[b]) { sourceWindow = layer->ButtonOwners[b].Window; sourceViewport = layer->ButtonOwners[b].Viewport; break; }
            if (keyboard)
            {
                if (key == EKeyCode::Unknown) { s.Keys.fill(false); s.PressedKeys.fill(false); s.ReleasedKeys.fill(false); }
                else { const auto k = static_cast<size_t>(key); s.Keys[k] = s.PressedKeys[k] = s.ReleasedKeys[k] = false; }
            }
            if (pointer)
            {
                s.Buttons.fill(false); s.PressedButtons.fill(false); s.ReleasedButtons.fill(false);
                s.MouseDeltaX = s.MouseDeltaY = s.ScrollX = s.ScrollY = 0;
            }
            s.Events.erase(std::remove_if(s.Events.begin(), s.Events.end(), affected), s.Events.end());
            s.Cancelled = true;
            InputEvent e;
            e.Type = InputEventType::Cancel;
            e.Window = sourceWindow;
            e.Viewport = sourceViewport;
            e.CancelKeyboard = keyboard;
            e.CancelPointer = pointer;
            e.Key = key;
            s.Events.push_back(e);
            if (pointer && CaptureId == layer->Id) DropCapture();
            auto handler = layer->Handler;
            if (handler) handler(e);
            layer->Cancelling = false;
        }
        void Remove(uint64_t id)
        {
            auto layer = Find(id);
            if (!layer) return;
            layer->Alive = false; // Immediately skip it in any in-flight snapshot.
            Cancel(layer);
        }
        void Flush()
        {
            auto pending = std::move(Pending);
            Pending.clear();
            for (auto& operation : pending) operation();
            Layers.erase(std::remove_if(Layers.begin(), Layers.end(), [](const auto& l) { return !l->Alive; }), Layers.end());
        }
        void Resolve(InputEvent& e) const
        {
            if (IsPointer(e))
            {
                if (CaptureId)
                {
                    if (auto capture = Find(CaptureId)) e.Viewport = capture->Desc.Viewport;
                    return;
                }
                if (!e.Viewport)
                    for (const auto& [key, viewport] : Viewports)
                        if (key.first == e.Window && viewport.Rect.Contains(e.X, e.Y)) { e.Viewport = key.second; break; }
            }
            else if (!e.Viewport && e.Window == FocusWindow) e.Viewport = FocusViewport;
        }
        bool Eligible(const Layer& layer, const InputEvent& e, bool owning = false) const
        {
            const auto& d = layer.Desc;
            if (!layer.Alive || !d.Enabled || (d.Window && d.Window != e.Window)) return false;
            if (!d.Categories[static_cast<size_t>(e.Category())]) return false;
            if (d.Viewport && d.Viewport != e.Viewport) return false;
            if (d.Viewport && Viewports.find({e.Window, e.Viewport}) == Viewports.end()) return false;
            if (d.Domain != InputDomain::Any)
            {
                auto it = Viewports.find({e.Window, e.Viewport});
                if (it == Viewports.end() || it->second.Domain != d.Domain) return false;
            }
            if (!IsPointer(e)) return d.KeyboardFocused && (!d.Viewport || (FocusWindow == e.Window && FocusViewport == d.Viewport));
            if (!d.PointerEnabled) return false;
            if (owning || CaptureId == layer.Id) return true;
            return !d.HitTest || d.HitTest(e);
        }
        bool Occluded(const std::vector<std::shared_ptr<Layer>>& order, const Layer& target, InputEvent e) const
        {
            Resolve(e);
            // Ownership survives leaving its hit rectangle, but not focus/domain/enable changes.
            if (!Eligible(target, e, true)) return true;
            for (const auto& upper : order)
            {
                if (upper->Id == target.Id) break;
                if (IsPointer(e) && CaptureId == target.Id && !upper->Desc.Modal) continue;
                if (Eligible(*upper, e) && upper->Desc.Propagation[static_cast<size_t>(e.Category())] == InputPropagation::Block) return true;
            }
            return false;
        }
        void Refresh()
        {
            if (Refreshing) return;
            Refreshing = true;
            const auto order = Ordered();
            for (const auto& layer : order)
            {
                if (!layer->Alive) continue;
                bool keyboard = !layer->Desc.Enabled, pointer = !layer->Desc.Enabled;
                for (size_t k = 0; !keyboard && k < layer->State.Keys.size(); ++k)
                    if (layer->State.Keys[k]) keyboard = Occluded(order, *layer, layer->KeyOwners[k]);
                for (size_t b = 0; !pointer && b < layer->State.Buttons.size(); ++b)
                    if (layer->State.Buttons[b]) pointer = Occluded(order, *layer, layer->ButtonOwners[b]);
                if (keyboard || pointer) Cancel(layer, keyboard, pointer);
            }
            Refreshing = false;
        }
        InputReply Deliver(const std::shared_ptr<Layer>& layer, const InputEvent& e)
        {
            auto& s = layer->State;
            if (e.Type == InputEventType::Key)
            {
                const auto k = static_cast<size_t>(e.Key);
                if (e.Action == EInputAction::Press) { s.Keys[k] = true; s.PressedKeys[k] = true; layer->KeyOwners[k] = e; }
                if (e.Action == EInputAction::Release) { s.Keys[k] = false; s.ReleasedKeys[k] = true; }
            }
            if (e.Type == InputEventType::Button)
            {
                const auto b = static_cast<size_t>(e.Button);
                if (e.Action == EInputAction::Press) { s.Buttons[b] = true; s.PressedButtons[b] = true; layer->ButtonOwners[b] = e; }
                if (e.Action == EInputAction::Release) { s.Buttons[b] = false; s.ReleasedButtons[b] = true; }
            }
            if (IsPointer(e)) { s.MouseX = e.X; s.MouseY = e.Y; }
            if (e.Type == InputEventType::Move) { s.MouseDeltaX += e.DeltaX; s.MouseDeltaY += e.DeltaY; }
            if (e.Type == InputEventType::Scroll) { s.ScrollX += e.DeltaX; s.ScrollY += e.DeltaY; }
            s.Events.push_back(e);
            auto handler = layer->Handler; // Safe when a callback unregisters itself.
            return handler ? handler(e) : InputReply::Unhandled;
        }
        void Route(InputEvent e)
        {
            if (e.Type == InputEventType::Cancel) return; // Cancellation is generated by the router.
            auto& raw = Raw[e.Window];
            if (e.Type == InputEventType::FocusLost || e.Type == InputEventType::WindowDestroyed || e.Type == InputEventType::CaptureLost)
            {
                const auto order = Ordered();
                for (const auto& layer : order)
                {
                    bool affected = CaptureId == layer->Id && CaptureWindow == e.Window;
                    if (e.Type != InputEventType::CaptureLost)
                    {
                        for (size_t k = 0; k < layer->State.Keys.size(); ++k)
                            affected |= layer->State.Keys[k] && layer->KeyOwners[k].Window == e.Window;
                        for (size_t b = 0; b < layer->State.Buttons.size(); ++b)
                            affected |= layer->State.Buttons[b] && layer->ButtonOwners[b].Window == e.Window;
                    }
                    if (affected) Cancel(layer);
                }
                if (e.Type != InputEventType::CaptureLost)
                {
                    raw = RawState{};
                    if (FocusWindow == e.Window) { FocusWindow = 0; FocusViewport = 0; }
                }
                if (e.Type == InputEventType::WindowDestroyed)
                {
                    for (auto it = Viewports.begin(); it != Viewports.end(); )
                        if (it->first.first == e.Window) it = Viewports.erase(it); else ++it;
                    for (const auto& layer : Layers)
                        if (layer->Desc.Window == e.Window && e.Window) layer->Desc.Enabled = false;
                    Raw.erase(e.Window);
                }
                return;
            }
            if (e.Type == InputEventType::Key)
            {
                const auto k = static_cast<size_t>(e.Key);
                if (e.Key == EKeyCode::Unknown || k >= raw.Keys.size()) return;
                if (e.Action != EInputAction::Release) e.Action = (e.Action == EInputAction::Repeat || raw.Keys[k]) ? EInputAction::Repeat : EInputAction::Press;
                raw.Keys[k] = e.Action != EInputAction::Release;
            }
            if (e.Type == InputEventType::Button)
            {
                const auto b = static_cast<size_t>(e.Button);
                if (b >= raw.Buttons.size()) return;
                if (e.Action != EInputAction::Release) e.Action = raw.Buttons[b] ? EInputAction::Repeat : EInputAction::Press;
                raw.Buttons[b] = e.Action != EInputAction::Release;
            }
            if (e.Type == InputEventType::Text && (e.Character > 0x10FFFF || (e.Character >= 0xD800 && e.Character <= 0xDFFF))) return;
            if (e.Type == InputEventType::Move)
            {
                e.DeltaX = raw.HasPosition ? e.X - raw.X : 0;
                e.DeltaY = raw.HasPosition ? e.Y - raw.Y : 0;
                raw.HasPosition = true;
            }
            if (IsPointer(e)) { raw.X = e.X; raw.Y = e.Y; raw.HasPosition = true; }
            Resolve(e);
            if (e.Type == InputEventType::Button && e.Action == EInputAction::Press)
            {
                FocusWindow = e.Window;
                FocusViewport = e.Viewport;
                Refresh();
            }
            const auto order = Ordered();
            // Releases always finish every surviving recipient of the corresponding press.
            // Consumption must never strand a lower layer in a held state.
            if ((e.Type == InputEventType::Key || e.Type == InputEventType::Button) && e.Action == EInputAction::Release)
            {
                for (const auto& layer : order)
                {
                    if (!layer->Alive) continue;
                    const bool owned = e.Type == InputEventType::Key ?
                        layer->State.Keys[static_cast<size_t>(e.Key)] && layer->KeyOwners[static_cast<size_t>(e.Key)].Window == e.Window :
                        layer->State.Buttons[static_cast<size_t>(e.Button)] && layer->ButtonOwners[static_cast<size_t>(e.Button)].Window == e.Window;
                    if (owned) Deliver(layer, e);
                }
            }
            else
            {
                bool blocked = false;
                for (const auto& layer : order)
                {
                    if (!layer->Alive) continue;
                    if (blocked)
                    {
                        // Blocking text must not cancel unrelated held gameplay keys.
                        const bool held = e.Type == InputEventType::Key ? Read(layer->State.Keys, e.Key) :
                            IsPointer(e) && std::any_of(layer->State.Buttons.begin(), layer->State.Buttons.end(), [](bool v) { return v; });
                        if (held && Eligible(*layer, e, true)) Cancel(layer, e.Type == InputEventType::Key, IsPointer(e), e.Key);
                        continue;
                    }
                    if (IsPointer(e) && CaptureId && CaptureId != layer->Id && !layer->Desc.Modal) continue;
                    if (!Eligible(*layer, e)) continue;
                    bool repeatOwned = true;
                    if (e.Type == InputEventType::Key && e.Action == EInputAction::Repeat) repeatOwned = Read(layer->State.Keys, e.Key);
                    if (e.Type == InputEventType::Button && e.Action == EInputAction::Repeat) repeatOwned = Read(layer->State.Buttons, e.Button);
                    const auto reply = repeatOwned ? Deliver(layer, e) : InputReply::Unhandled;
                    const auto propagation = layer->Desc.Propagation[static_cast<size_t>(e.Category())];
                    blocked = propagation == InputPropagation::Block ||
                        (propagation == InputPropagation::StopIfHandled && reply == InputReply::Handled);
                }
            }
            if (CaptureId)
                if (auto capture = Find(CaptureId); !capture ||
                    std::none_of(capture->State.Buttons.begin(), capture->State.Buttons.end(), [](bool v) { return v; })) DropCapture();
        }
    };

    InputCategory InputEvent::Category() const
    {
        switch (Type)
        {
        case InputEventType::Key: return InputCategory::Keyboard;
        case InputEventType::Text: return InputCategory::Text;
        case InputEventType::Button: return InputCategory::Button;
        case InputEventType::Scroll: return InputCategory::Scroll;
        default: return InputCategory::Move;
        }
    }
    bool InputState::IsKeyDown(EKeyCode k) const { return Read(Keys, k); }
    bool InputState::IsKeyPressed(EKeyCode k) const { return Read(PressedKeys, k); }
    bool InputState::IsKeyReleased(EKeyCode k) const { return Read(ReleasedKeys, k); }
    bool InputState::IsMouseDown(EMouseButton b) const { return Read(Buttons, b); }
    bool InputState::IsMousePressed(EMouseButton b) const { return Read(PressedButtons, b); }
    bool InputState::IsMouseReleased(EMouseButton b) const { return Read(ReleasedButtons, b); }

    InputLayerHandle::~InputLayerHandle() { Reset(); }
    InputLayerHandle::InputLayerHandle(InputLayerHandle&& other) noexcept : mOwner(std::move(other.mOwner)), mId(std::exchange(other.mId, 0)) {}
    InputLayerHandle& InputLayerHandle::operator=(InputLayerHandle&& other) noexcept
    {
        if (this != &other) { Reset(); mOwner = std::move(other.mOwner); mId = std::exchange(other.mId, 0); }
        return *this;
    }
    void InputLayerHandle::Reset() { if (auto owner = mOwner.lock()) owner->Remove(mId); mOwner.reset(); mId = 0; }
    InputLayerHandle::operator bool() const { auto owner = mOwner.lock(); return owner && owner->Find(mId) != nullptr; }
    InputRouter::InputRouter() : mData(std::make_shared<InputRouterData>()) {}
    InputRouter::~InputRouter() { mData->DropCapture(); }
    InputLayerHandle InputRouter::RegisterLayer(InputLayerDesc desc, Handler handler)
    {
        auto layer = std::make_shared<Layer>();
        layer->Id = mData->NextId++;
        layer->FirstEvent = mData->EventSerial + (mData->Dispatching ? 1 : 0);
        layer->Desc = std::move(desc);
        layer->Handler = std::move(handler);
        mData->Layers.push_back(layer);
        InputLayerHandle result;
        result.mOwner = mData;
        result.mId = layer->Id;
        return result;
    }
    void InputRouter::UpdateLayer(const InputLayerHandle& handle, InputLayerDesc desc)
    {
        if (handle.mOwner.lock() != mData) return;
        if (auto layer = mData->Find(handle.mId))
        {
            auto update = [layer, desc = std::move(desc)]() mutable { layer->Desc = std::move(desc); };
            if (mData->Dispatching) mData->Pending.push_back(std::move(update)); else update();
        }
    }
    const InputState& InputRouter::GetState(const InputLayerHandle& handle) const
    {
        static const InputState empty;
        if (handle.mOwner.lock() == mData) if (auto layer = mData->Find(handle.mId)) return layer->State;
        return empty;
    }
    void InputRouter::SetViewport(InputWindowId window, InputViewportId viewport, InputDomain domain, InputRect rect)
    {
        if (!viewport) return;
        auto update = [data = mData.get(), window, viewport, domain, rect] { data->Viewports[{window, viewport}] = {domain, rect}; };
        if (mData->Dispatching) mData->Pending.push_back(std::move(update)); else update();
    }
    void InputRouter::RemoveViewport(InputWindowId window, InputViewportId viewport)
    {
        auto update = [data = mData.get(), window, viewport] {
            data->Viewports.erase({window, viewport});
            if (data->FocusWindow == window && data->FocusViewport == viewport) data->FocusViewport = 0;
        };
        if (mData->Dispatching) mData->Pending.push_back(std::move(update)); else update();
    }
    void InputRouter::SetKeyboardFocus(InputWindowId window, InputViewportId viewport)
    {
        auto update = [data = mData.get(), window, viewport] { data->FocusWindow = window; data->FocusViewport = viewport; };
        if (mData->Dispatching) mData->Pending.push_back(std::move(update)); else update();
    }
    bool InputRouter::CapturePointer(const InputLayerHandle& handle, InputWindowId window)
    {
        if (handle.mOwner.lock() != mData) return false;
        auto layer = mData->Find(handle.mId);
        if (!layer || !layer->Desc.Enabled || (mData->CaptureId && mData->CaptureId != layer->Id) ||
            (layer->Desc.Window && layer->Desc.Window != window)) return false;
        bool ownsButton = false;
        for (size_t b = 0; b < layer->State.Buttons.size(); ++b)
            ownsButton |= layer->State.Buttons[b] && layer->ButtonOwners[b].Window == window;
        if (!ownsButton || (mData->CaptureId && mData->CaptureWindow != window)) return false;
        if (mData->CaptureId == layer->Id) return true;
        mData->CaptureId = layer->Id;
        mData->CaptureWindow = window;
        if (mData->NativeCapture) mData->NativeCapture(window, true);
        return true;
    }
    void InputRouter::ReleasePointer(const InputLayerHandle& handle)
    {
        if (handle.mOwner.lock() == mData && mData->CaptureId == handle.mId) mData->DropCapture();
    }
    bool InputRouter::HasPointerCapture(const InputLayerHandle& handle) const
    {
        return handle.mOwner.lock() == mData && handle.mId != 0 && mData->CaptureId == handle.mId;
    }
    void InputRouter::SetPointerCaptureCallback(std::function<void(InputWindowId, bool)> callback) { mData->NativeCapture = std::move(callback); }
    void InputRouter::QueueEvent(InputEvent e) { mData->Queue.push_back(e); }
    void InputRouter::BeginFrame()
    {
        mData->Flush();
        for (const auto& layer : mData->Layers)
        {
            auto& s = layer->State;
            s.PressedKeys.fill(false); s.ReleasedKeys.fill(false);
            s.PressedButtons.fill(false); s.ReleasedButtons.fill(false);
            s.MouseDeltaX = s.MouseDeltaY = s.ScrollX = s.ScrollY = 0;
            s.Cancelled = false;
            s.Events.clear();
        }
    }
    void InputRouter::RefreshRouting() { mData->Refresh(); }
    void InputRouter::DispatchEvents()
    {
        if (mData->Dispatching) return;
        mData->Dispatching = true;
        auto queue = std::move(mData->Queue);
        mData->Queue.clear();
        for (const auto& e : queue) { ++mData->EventSerial; mData->Flush(); mData->Refresh(); mData->Route(e); }
        mData->Flush();
        mData->Refresh();
        mData->Dispatching = false;
    }
    bool InputRouter::IsRawKeyDown(InputWindowId w, EKeyCode k) const
    {
        auto it = mData->Raw.find(w); return it != mData->Raw.end() && Read(it->second.Keys, k);
    }
    bool InputRouter::IsRawMouseDown(InputWindowId w, EMouseButton b) const
    {
        auto it = mData->Raw.find(w); return it != mData->Raw.end() && Read(it->second.Buttons, b);
    }
}
