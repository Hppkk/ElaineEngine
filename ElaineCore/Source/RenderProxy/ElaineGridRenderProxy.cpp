#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineGridRenderProxy.h"
#include "ElaineRenderQueue.h"

namespace Elaine
{
    GridRenderProxy::GridRenderProxy()
    {
        mType = EProxyType::Grid;
        // Grid is global; give it a huge AABB so it's always "visible"
        mWorldAABB = AxisAlignedBox(Vector3(-10000), Vector3(10000));
    }

    GridRenderProxy::~GridRenderProxy()
    {
    }

    void GridRenderProxy::InitializeResourceBinding()
    {
        // The vertex shader generates a three-vertex fullscreen triangle from gl_VertexIndex.
        mResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] = nullptr;
        mResourceBinding.mVertexCount = 3;
        mResourceBinding.mInstanceCount = 1;
    }

    void GridRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsVisible())
            return;

        if (!IsBindingsInitialized())
            return;

        // 使用 RenderMaterialProxy 的 IsReady 检查
        if (!mMaterialProxy.IsReady())
            return;

        ShaderPass* GridPass = mMaterialProxy.GetPass(Name("Grid"));
        if (GridPass == nullptr)
            return;

        // Submit to Transparent queue so it renders after opaque objects
        // Use a high priority to render before other transparent objects
        RenderQueue* CurrentRenderQueue = InRenderQueue->GetRenderQueue(RenderQueue_Transparent);
        CurrentRenderQueue->UpdateRenderQueue(GridPass, this, -900);
    }

    void GridRenderProxy::PrepareResourceBinding()
    {
    }
}
