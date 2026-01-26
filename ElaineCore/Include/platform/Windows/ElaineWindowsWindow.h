#pragma once
#include "Common/ElainePatform.h"

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace Elaine
{
    // Forward declaration
    template<typename T> class Window;
    struct WindowDesc;

    //-------------------------------------------------------------------------
    // Windows Platform Window Implementation
    //-------------------------------------------------------------------------
    class ElaineCoreExport WindowsWindow : public Window<WindowsWindow>
    {
        friend class Window<WindowsWindow>;

    public:
        WindowsWindow();
        ~WindowsWindow();

        //---------------------------------------------------------------------
        // Windows-Specific API
        //---------------------------------------------------------------------
        HWND        getHWND() const         { return m_hwnd; }
        HINSTANCE   getHInstance() const    { return m_hinstance; }

        // For Vulkan Surface creation
        HWND        getWin32Window() const  { return m_hwnd; }
        HINSTANCE   getWin32Instance() const { return m_hinstance; }

    protected:
        //---------------------------------------------------------------------
        // Implementation Methods (called by CRTP base)
        //---------------------------------------------------------------------
        bool createImpl(const WindowDesc& desc);
        void destroyImpl();

        void showImpl();
        void hideImpl();
        void minimizeImpl();
        void maximizeImpl();
        void restoreImpl();
        void focusImpl();

        bool shouldCloseImpl() const;
        bool isVisibleImpl() const;
        bool isMinimizedImpl() const;
        bool isMaximizedImpl() const;
        bool isFocusedImpl() const;

        void setTitleImpl(const String& title);
        void setSizeImpl(uint32 width, uint32 height);
        void setPositionImpl(int32 x, int32 y);
        void setFullscreenImpl(bool fullscreen);
        void setVSyncImpl(bool vsync);

        void* getNativeHandleImpl() const;
        void* getNativeDisplayImpl() const; // Returns nullptr on Windows

        void pollEventsImpl();
        void waitEventsImpl();

    private:
        //---------------------------------------------------------------------
        // Win32 Window Procedure
        //---------------------------------------------------------------------
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        //---------------------------------------------------------------------
        // Helper Methods
        //---------------------------------------------------------------------
        bool registerWindowClass();
        void unregisterWindowClass();
        DWORD getWindowStyle() const;
        DWORD getWindowExStyle() const;
        void updateFullscreenState(bool fullscreen);

    private:
        HWND        m_hwnd          = nullptr;
        HINSTANCE   m_hinstance     = nullptr;
        bool        m_shouldClose   = false;
        bool        m_isMinimized   = false;
        bool        m_isMaximized   = false;
        bool        m_isFocused     = false;

        // For fullscreen toggle
        WINDOWPLACEMENT m_prevPlacement = { sizeof(WINDOWPLACEMENT) };

        static const wchar_t* s_windowClassName;
        static int s_windowCount;
    };

} // namespace Elaine

#endif // ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
