#ifndef ELAINE_INPUT_STANDALONE
#include "ElainePrecompiledHeader.h"
#endif
#include "platform/Windows/ElaineWindowsInput.h"
#include "ElaineInputSystem.h"
#include <windowsx.h>
#include <map>

namespace Elaine
{
    EKeyCode WindowsInput::TranslateKey(WPARAM key, LPARAM flags)
    {
        if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z')) return static_cast<EKeyCode>(key);
        if (key >= VK_F1 && key <= VK_F12) return static_cast<EKeyCode>(static_cast<int>(EKeyCode::F1) + static_cast<int>(key - VK_F1));
        switch (key)
        {
        case VK_SHIFT: return MapVirtualKeyW((static_cast<UINT>(flags) >> 16) & 0xff, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT ? EKeyCode::RightShift : EKeyCode::LeftShift;
        case VK_CONTROL: return (flags & (1LL << 24)) ? EKeyCode::RightControl : EKeyCode::LeftControl;
        case VK_MENU: return (flags & (1LL << 24)) ? EKeyCode::RightAlt : EKeyCode::LeftAlt;
        case VK_LSHIFT: return EKeyCode::LeftShift;
        case VK_RSHIFT: return EKeyCode::RightShift;
        case VK_LCONTROL: return EKeyCode::LeftControl;
        case VK_RCONTROL: return EKeyCode::RightControl;
        case VK_LMENU: return EKeyCode::LeftAlt;
        case VK_RMENU: return EKeyCode::RightAlt;
        case VK_LWIN: return EKeyCode::LeftSuper;
        case VK_RWIN: return EKeyCode::RightSuper;
        case VK_APPS: return EKeyCode::Menu;
        case VK_SPACE: return EKeyCode::Space;
        case VK_ESCAPE: return EKeyCode::Escape;
        case VK_RETURN: return EKeyCode::Enter;
        case VK_TAB: return EKeyCode::Tab;
        case VK_BACK: return EKeyCode::Backspace;
        case VK_INSERT: return EKeyCode::Insert;
        case VK_DELETE: return EKeyCode::Delete;
        case VK_LEFT: return EKeyCode::Left;
        case VK_RIGHT: return EKeyCode::Right;
        case VK_UP: return EKeyCode::Up;
        case VK_DOWN: return EKeyCode::Down;
        case VK_PRIOR: return EKeyCode::PageUp;
        case VK_NEXT: return EKeyCode::PageDown;
        case VK_HOME: return EKeyCode::Home;
        case VK_END: return EKeyCode::End;
        case VK_CAPITAL: return EKeyCode::CapsLock;
        case VK_SCROLL: return EKeyCode::ScrollLock;
        case VK_NUMLOCK: return EKeyCode::NumLock;
        case VK_SNAPSHOT: return EKeyCode::PrintScreen;
        case VK_PAUSE: return EKeyCode::Pause;
        case VK_OEM_1: return EKeyCode::Semicolon;
        case VK_OEM_2: return EKeyCode::Slash;
        case VK_OEM_3: return EKeyCode::GraveAccent;
        case VK_OEM_4: return EKeyCode::LeftBracket;
        case VK_OEM_5: return EKeyCode::Backslash;
        case VK_OEM_6: return EKeyCode::RightBracket;
        case VK_OEM_7: return EKeyCode::Apostrophe;
        case VK_OEM_COMMA: return EKeyCode::Comma;
        case VK_OEM_PERIOD: return EKeyCode::Period;
        case VK_OEM_MINUS: return EKeyCode::Minus;
        case VK_OEM_PLUS: return EKeyCode::Equal;
        default: return EKeyCode::Unknown;
        }
    }

    void WindowsInput::QueueMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* input = InputSystem::TryGet();
        if (!input) return; // Window creation/destruction can precede/follow the engine.
        static std::map<HWND, char32_t> highSurrogates;
        InputEvent e;
        e.Window = reinterpret_cast<InputWindowId>(window);
        POINT position{};
        GetCursorPos(&position);
        e.X = static_cast<float>(position.x); e.Y = static_cast<float>(position.y);
        switch (message)
        {
        case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP:
            e.Type = InputEventType::Key;
            e.Key = TranslateKey(wParam, lParam);
            e.Action = (message == WM_KEYUP || message == WM_SYSKEYUP) ? EInputAction::Release :
                ((lParam & (1LL << 30)) ? EInputAction::Repeat : EInputAction::Press);
            break;
        case WM_CHAR:
        {
            const auto unit = static_cast<char32_t>(wParam);
            if (unit >= 0xD800 && unit <= 0xDBFF) { highSurrogates[window] = unit; return; }
            e.Type = InputEventType::Text;
            auto& high = highSurrogates[window];
            if (unit >= 0xDC00 && unit <= 0xDFFF)
            {
                if (!high) return;
                e.Character = 0x10000 + ((high - 0xD800) << 10) + unit - 0xDC00;
            }
            else e.Character = unit;
            high = 0;
            break;
        }
        case WM_UNICHAR:
            if (wParam == UNICODE_NOCHAR) return;
            e.Type = InputEventType::Text; e.Character = static_cast<char32_t>(wParam); break;
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP:
        case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: case WM_XBUTTONUP:
        case WM_MOUSEMOVE:
            position = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(window, &position);
            e.X = static_cast<float>(position.x); e.Y = static_cast<float>(position.y);
            if (message == WM_MOUSEMOVE) { e.Type = InputEventType::Move; break; }
            e.Type = InputEventType::Button;
            e.Action = (message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP || message == WM_XBUTTONUP) ? EInputAction::Release : EInputAction::Press;
            if (message == WM_RBUTTONDOWN || message == WM_RBUTTONDBLCLK || message == WM_RBUTTONUP) e.Button = EMouseButton::Right;
            else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONDBLCLK || message == WM_MBUTTONUP) e.Button = EMouseButton::Middle;
            else if (message == WM_XBUTTONDOWN || message == WM_XBUTTONDBLCLK || message == WM_XBUTTONUP) e.Button = GET_XBUTTON_WPARAM(wParam) == XBUTTON1 ? EMouseButton::X1 : EMouseButton::X2;
            break;
        case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            e.Type = InputEventType::Scroll;
            e.X = static_cast<float>(GET_X_LPARAM(lParam)); e.Y = static_cast<float>(GET_Y_LPARAM(lParam));
            if (message == WM_MOUSEWHEEL) e.DeltaY = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            else e.DeltaX = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
            break;
        case WM_KILLFOCUS:
            highSurrogates.erase(window);
            e.Type = InputEventType::FocusLost; break;
        case WM_CANCELMODE:
            e.Type = InputEventType::CaptureLost; break;
        case WM_CAPTURECHANGED:
            // The ImGui backend releases native capture during ordinary button-up handling.
            // That is completion, not cancellation; the queued up event must still be delivered.
            if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000) && !(GetAsyncKeyState(VK_RBUTTON) & 0x8000) &&
                !(GetAsyncKeyState(VK_MBUTTON) & 0x8000) && !(GetAsyncKeyState(VK_XBUTTON1) & 0x8000) && !(GetAsyncKeyState(VK_XBUTTON2) & 0x8000)) return;
            e.Type = InputEventType::CaptureLost; break;
        case WM_NCDESTROY:
            highSurrogates.erase(window);
            e.Type = InputEventType::WindowDestroyed; break;
        default: return;
        }
        input->QueueEvent(e);
    }
}
