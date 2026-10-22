#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineRenderQueue.h"

namespace Elaine
{
    SkyRenderProxy::SkyRenderProxy()
    {
        mType = EProxyType::Sky;
        mWorldAABB = AxisAlignedBox(Vector3(-2000), Vector3(2000));
    }

    SkyRenderProxy::~SkyRenderProxy()
    {
        CancelResourceNotifications();
    }

    void SkyRenderProxy::InitializeResourceBinding()
    {
        struct VertexData
        {
            float mVertex[3];
            float mUV[2];
            float mNormals[3];
        };

        std::vector<VertexData> CpuData = {
            {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f},
            { 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f},
            { 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f},
            {-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f},

            {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f},
            { 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f},
            { 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f},
            {-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f},

            {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
            {-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
            {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f},
            {-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f},

            { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
            { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f},
            { 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f},

            {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f},
            { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f},
            { 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f},
            {-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f},
            {-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f},

            {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f},
            { 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f},
            { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f},
            { 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f},
            {-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f},
            {-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f},
        };

        //@TODO RHI thread.
        auto VertexBuffer = RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer, ERHIAccess::VertexOrIndexBuffer,
                CpuData.data(), CpuData.size() * sizeof(VertexData));
        mResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] = VertexBuffer.get();

        mResourceBinding.mVertexCount = 36;
        mResourceBinding.mInstanceCount = 1;
        mMaterialProxy.BindToResourceBinding(mResourceBinding);


    }

    void SkyRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsBindingsInitialized())
            return;

        // 使用 RenderMaterialProxy 的 IsReady 检查
        if (!mMaterialProxy.IsReady())
            return;

        mMaterialProxy.BindToResourceBinding(mResourceBinding);
        ShaderPass* SkyPass = mMaterialProxy.GetPass(Name("Sky"));
        if (SkyPass == nullptr)
            return;

        RenderQueue* CurrentRenderQueue = InRenderQueue->GetRenderQueue(RenderQueue_Sky);
        CurrentRenderQueue->UpdateRenderQueue(SkyPass, this, -1000);
    }

    void SkyRenderProxy::ApplyResourceUpdate(const RenderResourceUpdate& Update)
    {
        mMaterialProxy.ApplyResourceUpdate(Update);
        mMaterialProxy.BindToResourceBinding(mResourceBinding);

    }
    void SkyRenderProxy::PrepareResourceBinding()
    {

    }

}
