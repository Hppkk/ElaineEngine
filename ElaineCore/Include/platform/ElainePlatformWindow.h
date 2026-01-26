#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineVector2.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineRenderTarget.h"
#include "ElaineSwapchainRenderTarget.h"

namespace Elaine
{
    // Forward declarations
    struct WindowDesc;
    class RHISwapchain;

    //-------------------------------------------------------------------------
    // Window Description
    //-------------------------------------------------------------------------
    struct WindowDesc
    {
        String      title       = "Elaine Window";
        uint32      width       = 1280;
        uint32      height      = 720;
        int32       posX        = 100;
        int32       posY        = 100;
        bool        resizable   = true;
        bool        fullscreen  = false;
        bool        vsync       = true;
        bool        decorated   = true;     // Has title bar and borders
        bool        visible     = true;
        void*       parentHandle = nullptr; // For child windows
    };

    //-------------------------------------------------------------------------
    // Window Event Types
    //-------------------------------------------------------------------------
    enum class WindowEventType
    {
        None,
        Resize,
        Close,
        Move,
        Focus,
        LostFocus,
        Minimize,
        Maximize,
        Restore
    };

    struct WindowEvent
    {
        WindowEventType type = WindowEventType::None;
        int32           data1 = 0;
        int32           data2 = 0;
    };

    //-------------------------------------------------------------------------
    // Platform Window Base - CRTP Pattern
    //-------------------------------------------------------------------------
    template<typename PlatformImpl>
    class Window : public RenderTarget
    {
    public:
        using ResizeCallback    = std::function<void(uint32, uint32)>;
        using CloseCallback     = std::function<void()>;
        using FocusCallback     = std::function<void(bool)>;
        using MoveCallback      = std::function<void(int32, int32)>;

    public:
        Window() = default;
        virtual ~Window() = default;

        // Non-copyable, movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = default;
        Window& operator=(Window&&) = default;

        //---------------------------------------------------------------------
        // Window Lifecycle
        //---------------------------------------------------------------------
        bool create(const WindowDesc& desc)
        {
            m_desc = desc;
            if (!impl().createImpl(desc))
            {
                return false;
            }

            mbInitilize = true;

            return true;
        }

        void createSwapchain()
        {
            //ENQUEUE_RENDER_COMMAND(CreateSwapchain)(
            //    [this](RenderContext& Context)
            //    {
            //        // --- 渲染线程执行 ---

            //        // RHI 工厂模式创建
            //        this->mSwapchainRHI = Context.mCommandCtxRHI->RHICreateSwapchain(m_desc.width, m_desc.height, false, PF_R8G8B8A8);
            //        Context.mCommandCtxRHI->RHISetSwapchain(mSwapchainRHI);
            //    });

            mSwapchainRHI = RenderSystem::instance()->GetRHICommandContext()->RHICreateSwapchain(m_desc.width, m_desc.height, false, PF_R8G8B8A8);
            RenderSystem::instance()->GetRHICommandContext()->RHISetSwapchain(mSwapchainRHI);

            mSwapchainRenderTarget.SetSwapchain(mSwapchainRHI);
        }

        void destroy()
        {
            impl().destroyImpl();
        }

        //---------------------------------------------------------------------
        // Window State
        //---------------------------------------------------------------------
        void show()                         { impl().showImpl(); }
        void hide()                         { impl().hideImpl(); }
        void minimize()                     { impl().minimizeImpl(); }
        void maximize()                     { impl().maximizeImpl(); }
        void restore()                      { impl().restoreImpl(); }
        void focus()                        { impl().focusImpl(); }

        bool shouldClose() const            { return impl().shouldCloseImpl(); }
        bool isVisible() const              { return impl().isVisibleImpl(); }
        bool isMinimized() const            { return impl().isMinimizedImpl(); }
        bool isMaximized() const            { return impl().isMaximizedImpl(); }
        bool isFocused() const              { return impl().isFocusedImpl(); }

        //---------------------------------------------------------------------
        // Window Properties
        //---------------------------------------------------------------------
        void setTitle(const String& title)
        {
            m_desc.title = title;
            impl().setTitleImpl(title);
        }

        void setSize(uint32 width, uint32 height)
        {
            m_desc.width = width;
            m_desc.height = height;
            impl().setSizeImpl(width, height);
        }

        void setPosition(int32 x, int32 y)
        {
            m_desc.posX = x;
            m_desc.posY = y;
            impl().setPositionImpl(x, y);
        }

        void setFullscreen(bool fullscreen)
        {
            m_desc.fullscreen = fullscreen;
            impl().setFullscreenImpl(fullscreen);
        }

        void setVSync(bool vsync)
        {
            m_desc.vsync = vsync;
            impl().setVSyncImpl(vsync);
        }

        //---------------------------------------------------------------------
        // Getters
        //---------------------------------------------------------------------
        const String&   getTitle() const        { return m_desc.title; }
        uint32          getWidth() const        { return m_desc.width; }
        uint32          getHeight() const       { return m_desc.height; }
        Vector2         getSize() const         { return Vector2(static_cast<float>(m_desc.width), static_cast<float>(m_desc.height)); }
        int32           getPosX() const         { return m_desc.posX; }
        int32           getPosY() const         { return m_desc.posY; }
        bool            isFullscreen() const    { return m_desc.fullscreen; }
        bool            isVSync() const         { return m_desc.vsync; }
        const WindowDesc& getDesc() const       { return m_desc; }

        //---------------------------------------------------------------------
        // Native Handle Access
        //---------------------------------------------------------------------
        void* getNativeHandle() const           { return impl().getNativeHandleImpl(); }
        void* getNativeDisplay() const          { return impl().getNativeDisplayImpl(); } // For X11/Wayland

        //---------------------------------------------------------------------
        // Event Processing
        //---------------------------------------------------------------------
        void pollEvents()                       { impl().pollEventsImpl(); }
        void waitEvents()                       { impl().waitEventsImpl(); }

        //---------------------------------------------------------------------
        // Callbacks
        //---------------------------------------------------------------------
        void setResizeCallback(ResizeCallback cb)   { m_resizeCallback = std::move(cb); }
        void setCloseCallback(CloseCallback cb)     { m_closeCallback = std::move(cb); }
        void setFocusCallback(FocusCallback cb)     { m_focusCallback = std::move(cb); }
        void setMoveCallback(MoveCallback cb)       { m_moveCallback = std::move(cb); }

    protected:
        // Called by implementations to trigger callbacks
        void onResize(uint32 width, uint32 height)
        {
            if (!mbInitilize)
                return;

            m_desc.width = width;
            m_desc.height = height;
            ENQUEUE_RENDER_COMMAND(ResizeSwapchain)(
                [SwapchainRHI = mSwapchainRHI, width, height](RenderContext& Context)
                {
                    // --- 渲染线程 ---
                    // 1. 等待 GPU 空闲 (必须！不能销毁正在用的 BackBuffer)
                    Context.mCommandCtxRHI->RHIWaitIdle();

                    // 2. 调用具体的 Resize 实现 (销毁旧的 VkSwapchain, 创建新的)
                    Context.mCommandCtxRHI->RHIWindowResize(SwapchainRHI, width, height);
                });
            if (m_resizeCallback) m_resizeCallback(width, height);
        }

        void onClose()
        {
            if (m_closeCallback) m_closeCallback();
        }

        void onFocus(bool focused)
        {
            if (m_focusCallback) m_focusCallback(focused);
        }

        void onMove(int32 x, int32 y)
        {
            m_desc.posX = x;
            m_desc.posY = y;
            if (m_moveCallback) m_moveCallback(x, y);
        }
    public:
        RHITexture* GetTargetImpl() override
        {
            return mSwapchainRenderTarget.GetTargetImpl();
        }

        void GetSize(uint32& OutWidth, uint32& OutHeight) override
        {
            OutWidth = m_desc.width;
            OutHeight = m_desc.height;
        }

        SwapchainRenderTarget* GetSwapchainRenderTarget()
        {
            return &mSwapchainRenderTarget;
        }

        RenderTarget* GetRenderTarget()
        {
            return &mSwapchainRenderTarget;
        }

        // 实现 RenderTarget 的交换链接口
        bool IsSwapchainTarget() const override { return true; }
        RHISwapchain* GetSwapchain() const override { return mSwapchainRHI; }
        void SetCurrentImageIndex(uint32 Index) override { mSwapchainRenderTarget.SetCurrentImageIndex(Index); }
        uint32 GetCurrentImageIndex() const override { return mSwapchainRenderTarget.GetCurrentImageIndex(); }
        RHITexture* GetColorTarget(uint32 Index = 0) override { return mSwapchainRenderTarget.GetColorTarget(Index); }
        RHITexture* GetDepthStencilTarget() override { return mSwapchainRenderTarget.GetDepthStencilTarget(); }
    protected:
        WindowDesc      m_desc;

    private:
        // CRTP helper
        PlatformImpl& impl()                { return static_cast<PlatformImpl&>(*this); }
        const PlatformImpl& impl() const    { return static_cast<const PlatformImpl&>(*this); }

        // Callbacks
        ResizeCallback  m_resizeCallback;
        CloseCallback   m_closeCallback;
        FocusCallback   m_focusCallback;
        MoveCallback    m_moveCallback;
        RHISwapchain*   mSwapchainRHI = nullptr;
        SwapchainRenderTarget   mSwapchainRenderTarget;
        bool            mbInitilize = false;
    };

} // namespace Elaine

//-----------------------------------------------------------------------------
// Include Platform-Specific Implementation
//-----------------------------------------------------------------------------
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
    #include "platform/Windows/ElaineWindowsWindow.h"
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_LINUX
    #include "platform/Linux/ElaineLinuxWindow.h"
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_APPLE
    #include "platform/Apple/ElaineAppleWindow.h"
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID
    #include "platform/Android/ElaineAndroidWindow.h"
#endif

namespace Elaine
{
    //-------------------------------------------------------------------------
    // Type Alias for Current Platform
    //-------------------------------------------------------------------------
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
    using PlatformWindow = WindowsWindow;
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_LINUX
    using Window = LinuxWindow;
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_APPLE
    using Window = AppleWindow;
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID
    using Window = AndroidWindow;
#endif

} // namespace Elaine
