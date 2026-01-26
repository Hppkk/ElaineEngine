#include "ElainePrecompiledHeader.h"
#include "platform/ElainePlatformWindow.h"

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

namespace Elaine
{
    const wchar_t* WindowsWindow::s_windowClassName = L"ElaineWindowClass";
    int WindowsWindow::s_windowCount = 0;
    static HICON hIcon;

    //-------------------------------------------------------------------------
    // Constructor / Destructor
    //-------------------------------------------------------------------------
    WindowsWindow::WindowsWindow()
        : m_hinstance(GetModuleHandle(nullptr))
    {
    }

    WindowsWindow::~WindowsWindow()
    {
        destroyImpl();
    }

    //-------------------------------------------------------------------------
    // Window Creation / Destruction
    //-------------------------------------------------------------------------
    bool WindowsWindow::createImpl(const WindowDesc& desc)
    {
        if (!registerWindowClass())
        {
            return false;
        }

        DWORD style = getWindowStyle();
        DWORD exStyle = getWindowExStyle();

        // Calculate window size including borders
        RECT rect = { 0, 0, static_cast<LONG>(desc.width), static_cast<LONG>(desc.height) };
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;

        // Convert title to wide string
        int titleLen = MultiByteToWideChar(CP_UTF8, 0, desc.title.c_str(), -1, nullptr, 0);
        std::wstring wideTitle(titleLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, desc.title.c_str(), -1, &wideTitle[0], titleLen);

        m_hwnd = CreateWindowExW(
            exStyle,
            s_windowClassName,
            wideTitle.c_str(),
            style,
            desc.posX,
            desc.posY,
            windowWidth,
            windowHeight,
            static_cast<HWND>(desc.parentHandle),
            nullptr,
            m_hinstance,
            this  // Pass this pointer for WM_CREATE
        );

        if (!m_hwnd)
        {
            return false;
        }

        s_windowCount++;

        if (desc.visible)
        {
            showImpl();
        }

        if (desc.fullscreen)
        {
            updateFullscreenState(true);
        }

        if (hIcon != NULL)
        {
            SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }

        return true;
    }

    void WindowsWindow::destroyImpl()
    {
        if (m_hwnd)
        {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            s_windowCount--;

            if (s_windowCount == 0)
            {
                unregisterWindowClass();
            }
        }
    }

    //-------------------------------------------------------------------------
    // Window State
    //-------------------------------------------------------------------------
    void WindowsWindow::showImpl()
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
    }

    void WindowsWindow::hideImpl()
    {
        ShowWindow(m_hwnd, SW_HIDE);
    }

    void WindowsWindow::minimizeImpl()
    {
        ShowWindow(m_hwnd, SW_MINIMIZE);
    }

    void WindowsWindow::maximizeImpl()
    {
        ShowWindow(m_hwnd, SW_MAXIMIZE);
    }

    void WindowsWindow::restoreImpl()
    {
        ShowWindow(m_hwnd, SW_RESTORE);
    }

    void WindowsWindow::focusImpl()
    {
        SetForegroundWindow(m_hwnd);
        SetFocus(m_hwnd);
    }

    bool WindowsWindow::shouldCloseImpl() const
    {
        return m_shouldClose;
    }

    bool WindowsWindow::isVisibleImpl() const
    {
        return IsWindowVisible(m_hwnd) != FALSE;
    }

    bool WindowsWindow::isMinimizedImpl() const
    {
        return m_isMinimized;
    }

    bool WindowsWindow::isMaximizedImpl() const
    {
        return m_isMaximized;
    }

    bool WindowsWindow::isFocusedImpl() const
    {
        return m_isFocused;
    }

    //-------------------------------------------------------------------------
    // Window Properties
    //-------------------------------------------------------------------------
    void WindowsWindow::setTitleImpl(const String& title)
    {
        int titleLen = MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, nullptr, 0);
        std::wstring wideTitle(titleLen, 0);
        MultiByteToWideChar(CP_UTF8, 0, title.c_str(), -1, &wideTitle[0], titleLen);
        SetWindowTextW(m_hwnd, wideTitle.c_str());
    }

    void WindowsWindow::setSizeImpl(uint32 width, uint32 height)
    {
        RECT rect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRectEx(&rect, getWindowStyle(), FALSE, getWindowExStyle());
        SetWindowPos(m_hwnd, nullptr, 0, 0, 
                     rect.right - rect.left, 
                     rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    }

    void WindowsWindow::setPositionImpl(int32 x, int32 y)
    {
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    void WindowsWindow::setFullscreenImpl(bool fullscreen)
    {
        updateFullscreenState(fullscreen);
    }

    void WindowsWindow::setVSyncImpl(bool vsync)
    {
        // VSync is typically handled by the swap chain, not the window
        // This is a placeholder for platform-specific implementation
    }

    //-------------------------------------------------------------------------
    // Native Handle
    //-------------------------------------------------------------------------
    void* WindowsWindow::getNativeHandleImpl() const
    {
        return static_cast<void*>(m_hwnd);
    }

    void* WindowsWindow::getNativeDisplayImpl() const
    {
        return nullptr; // Not used on Windows
    }

    //-------------------------------------------------------------------------
    // Event Processing
    //-------------------------------------------------------------------------
    void WindowsWindow::pollEventsImpl()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void WindowsWindow::waitEventsImpl()
    {
        WaitMessage();
        pollEventsImpl();
    }

    //-------------------------------------------------------------------------
    // Win32 Window Procedure
    //-------------------------------------------------------------------------
    LRESULT CALLBACK WindowsWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        WindowsWindow* window = nullptr;

        if (msg == WM_NCCREATE)
        {
            CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
            window = static_cast<WindowsWindow*>(cs->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
            window->m_hwnd = hwnd;
        }
        else
        {
            window = reinterpret_cast<WindowsWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (window)
        {
            return window->handleMessage(msg, wParam, lParam);
        }

        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    LRESULT WindowsWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CLOSE:
            m_shouldClose = true;
            onClose();
            return 0;

        case WM_DESTROY:
            return 0;

        case WM_SIZE:
            {
                uint32 width = LOWORD(lParam);
                uint32 height = HIWORD(lParam);
                m_isMinimized = (wParam == SIZE_MINIMIZED);
                m_isMaximized = (wParam == SIZE_MAXIMIZED);
                if (!m_isMinimized)
                {
                    onResize(width, height);
                }
            }
            return 0;

        case WM_MOVE:
            {
                int32 x = static_cast<int32>(LOWORD(lParam));
                int32 y = static_cast<int32>(HIWORD(lParam));
                onMove(x, y);
            }
            return 0;

        case WM_SETFOCUS:
            m_isFocused = true;
            onFocus(true);
            return 0;

        case WM_KILLFOCUS:
            m_isFocused = false;
            onFocus(false);
            return 0;

        case WM_GETMINMAXINFO:
            {
                // Allow window to be any size
                MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
                mmi->ptMinTrackSize.x = 1;
                mmi->ptMinTrackSize.y = 1;
            }
            return 0;
        }

        return DefWindowProc(m_hwnd, msg, wParam, lParam);
    }

    //-------------------------------------------------------------------------
    // Helper Methods
    //-------------------------------------------------------------------------

    bool WindowsWindow::registerWindowClass()
    {
        wchar_t ExePath[MAX_PATH];
        GetModuleFileNameW(NULL, ExePath, MAX_PATH);
        PathRemoveFileSpecW(ExePath);
        std::wstring IconPath = std::wstring(ExePath) + L"\\..\\..\\icons\\ElaineEngine.ico";

         hIcon = (HICON)LoadImage(
            NULL,
            IconPath.c_str(),
            IMAGE_ICON,
            32,
            32,
            LR_LOADFROMFILE | LR_DEFAULTSIZE
        );
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_hinstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszClassName = s_windowClassName;
        wc.hIcon = hIcon;
        wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassExW(&wc))
        {
            // Class may already be registered
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            {
                return false;
            }
        }
        return true;
    }

    void WindowsWindow::unregisterWindowClass()
    {
        UnregisterClassW(s_windowClassName, m_hinstance);
    }

    DWORD WindowsWindow::getWindowStyle() const
    {
        DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

        if (m_desc.decorated)
        {
            style |= WS_OVERLAPPEDWINDOW;
        }
        else
        {
            style |= WS_POPUP;
        }

        if (!m_desc.resizable && m_desc.decorated)
        {
            style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
        }

        return style;
    }

    DWORD WindowsWindow::getWindowExStyle() const
    {
        DWORD exStyle = WS_EX_APPWINDOW;
        return exStyle;
    }

    void WindowsWindow::updateFullscreenState(bool fullscreen)
    {
        if (fullscreen)
        {
            // Save current window placement
            GetWindowPlacement(m_hwnd, &m_prevPlacement);

            // Set borderless fullscreen
            SetWindowLong(m_hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

            HMONITOR monitor = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            GetMonitorInfo(monitor, &mi);

            SetWindowPos(m_hwnd, HWND_TOP,
                         mi.rcMonitor.left,
                         mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_FRAMECHANGED);
        }
        else
        {
            // Restore window style and position
            SetWindowLong(m_hwnd, GWL_STYLE, getWindowStyle());
            SetWindowPlacement(m_hwnd, &m_prevPlacement);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

} // namespace Elaine

#endif // ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
