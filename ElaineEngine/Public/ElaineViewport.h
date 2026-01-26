#pragma once
#include "ElaineEnginePrerequirements.h"
#include "ElaineRenderTarget.h"

namespace Elaine
{
    class CameraComponent;
    class World;
    class RenderView;

    struct ViewportDesc
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Width = 1.0f;
        float Height = 1.0f;
        std::string ViewName;
    };

	class ElaineEngineExport Viewport
	{
	public:
        Viewport(const ViewportDesc& InDesc);
        void SetTarget(RenderTarget* InTarget);
        void SetCamera(CameraComponent* InCamera);
        void SetWorld(World* InWorld);
        void OnTargetResized(int InTargetW, int InTargetH);
        void Update();
        RenderView* GetRenderView() const { return mRenderView; }

    private:
        RenderTarget* mTarget = nullptr;
        CameraComponent* mCamera = nullptr;
        RenderView* mRenderView = nullptr;
        World* mWorld = nullptr;
        bool mbActive = false;
        std::string mName;
        struct { float X = 0, Y = 0, W = 1, H = 1; } mConfig;
	};
}