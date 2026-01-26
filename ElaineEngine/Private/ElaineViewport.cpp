#include "ElainePrecompiledHeader.h"
#include "ElaineViewport.h"
#include "ElaineCameraComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineWorld.h"
#include "ElainePlatformWindow.h"

namespace Elaine
{
    Viewport::Viewport(const ViewportDesc& InDesc)
    {
        mName = InDesc.ViewName;
        mConfig.X = InDesc.X;
        mConfig.Y = InDesc.Y;
        mConfig.W = InDesc.Width;
        mConfig.H = InDesc.Height;

        mRenderView = new RenderView();
        mRenderView->mRegion.MinX = InDesc.X;
        mRenderView->mRegion.MinY = InDesc.Y;
        mRenderView->mRegion.MaxX = InDesc.Width;
        mRenderView->mRegion.MaxY = InDesc.Height;

    }

    void Viewport::SetTarget(RenderTarget* InTarget)
    {
        if (InTarget == nullptr)
            return;

        mTarget = InTarget;

        ENQUEUE_RENDER_COMMAND(SetTarget)([=](RenderContext& InContext)
        {    
            mRenderView->mRenderTarget = static_cast<PlatformWindow*>(mTarget)->GetSwapchainRenderTarget();
        });
    }

    void Viewport::SetCamera(CameraComponent* InCamera)
    {
        if (InCamera == nullptr)
            return;

        mCamera = InCamera;

        ENQUEUE_RENDER_COMMAND(UpdateCamera)([=](RenderContext& InContext)
        {
            mRenderView->mCamera = mCamera->GetRenderThreadCamera();
        });
    }

    void Viewport::SetWorld(World* InWorld)
    {
        mWorld = InWorld;
        ENQUEUE_RENDER_COMMAND(SetWorld)([=](RenderContext& InContext)
        {
            mRenderView->mSceneManager = mWorld->GetSceneManager();
        });
    }

    void Viewport::OnTargetResized(int InTargetW, int InTargetH)
    {
        float ActualW = InTargetW * mConfig.W;
        float ActualH = InTargetH * mConfig.H;

        if (mCamera)
        {
            mCamera->SetAspect(ActualW / ActualH);
        }
    }

    void Viewport::Update()
    {

    }
}