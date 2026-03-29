#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineGridRenderProxy.h"
#include "ElaineRenderQueue.h"
#include "ElaineMaterial.h"
#include "ElaineMaterialInstanceDynamic.h"

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
        // Fullscreen triangle: 6 vertices, no vertex buffer needed.
        // The vertex shader generates positions from gl_VertexIndex.
        // We still need a dummy VBO for the pipeline to be happy.
        struct VertexData
        {
            float mVertex[3];
            float mUV[2];
            float mNormals[3];
        };

        // 6 vertices for a fullscreen quad (2 triangles)
        std::vector<VertexData> CpuData = {
            { 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f},
            {-1.0f,  1.0f, 0.0f,  0.0f, 1.0f,  0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f,  1.0f, 0.0f,  1.0f, 1.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f, 0.0f,  1.0f, 0.0f,  0.0f, 0.0f, 1.0f},
        };

        mResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] =
            RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer, ERHIAccess::VertexOrIndexBuffer,
                CpuData.data(), CpuData.size() * sizeof(VertexData));

        mResourceBinding.mVertexCount = 6;
        mResourceBinding.mInstanceCount = 1;
    }

    void GridRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsBindingsInitialized())
            return;

        if (mMaterial == nullptr || !mMaterial->IsReady())
            return;

        ShaderPass* GridPass = mMaterial->GetPass(Name("Grid"));
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
