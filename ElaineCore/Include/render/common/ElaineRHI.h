#pragma once
#include "render/common/ElaineRHIProtocol.h"

namespace Elaine
{
    enum RHITYPE
    {
        Vulkan,
        Dx11,
        Dx12,
        Metal,
        OpenGl,
    };

    struct RHI_PARAM_DESC
    {
#ifdef WIN32
        void* WindowHandle = 0;
#endif
        uint32 Width = 1920;
        uint32 Height = 1080;
        RHITYPE RHIType = Vulkan;
        bool UseRHIThread = false;
    };

    enum EM_RHICommand
    {
        CreateBuffer,
        CreateTexture,
        CopyBuffer,
        CreateRenderPipeline,
        BeginRenderPass,
        EndRenderPass,
        BindGfxPipeline,
        BindDrawData,
        ResizeWindow,
        DrawPrimitive,
        DrawIndex,
        DrawInstance,
        DrawIndirect,
        DrawIndexInstance,
        Dispatch,
    };

    bool ElaineCoreExport InitEngineRHI(const RHI_PARAM_DESC& InDesc);
    bool ElaineCoreExport DestroyEngineRHI();
    bool ElaineCoreExport RecordRHICommand(EM_RHICommand InCmd, void* InRHIResourceHandle);
    void ElaineCoreExport WaitForRHIThread_Gfx();
    void ElaineCoreExport NotifyForRHIThread_Gfx();
    void ElaineCoreExport WaitForRenderThread_Gfx();
    void ElaineCoreExport NotifyForRenderThread_Gfx();

    struct RHIRenderContext
    {

    };

    class ElaineCoreExport IRHIRenderListener
    {
    public:
        IRHIRenderListener() = default;
        virtual ~IRHIRenderListener() { }
        virtual void FrameBegin(const RHIRenderContext& InCtx) = 0;
        virtual void FrameEnd(const RHIRenderContext& InCtx) = 0;
    };

    using RHIRenderListenerList = std::vector<IRHIRenderListener*>;


    class RHICommandContext;

    class ElaineCoreExport DynamicRHI
    {
    public:
        virtual ~DynamicRHI();
        virtual void Initilize(const RHI_PARAM_DESC& InDesc) = 0;
        virtual void Destroy() = 0;
        RHICommandContext* GetDefaultCommandContext() const { return mDefaultCommandContext; }
        RHICommandContext* GetDefaultComputeContext() const { return mDefaultComputeContext; }
        RHICommandContext* GetDefaultTransferContext() const { return mDefaultTransferContext; }
        virtual RHICommandContext* CreateCommandContex() = 0;
        virtual void DestroyCommandContext(RHICommandContext* InCtx) = 0;
    protected:
        RHICommandContext* mDefaultCommandContext = nullptr;
        RHICommandContext* mDefaultComputeContext = nullptr;
        RHICommandContext* mDefaultTransferContext = nullptr;
        std::vector<RHICommandContext*> mCommandContexts;
        void* mWindowHandle = nullptr;
    };

    ElaineCoreExport DynamicRHI*  GetDynamicRHI();
}