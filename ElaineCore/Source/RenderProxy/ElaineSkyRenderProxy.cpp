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

        // 从 RenderMaterialProxy 获取已解析的 RHI 纹理
        RHITexture* CubeRHI = mMaterialProxy.GetResolvedTexture(BaseColor);
        if (!CubeRHI) CubeRHI = mCubeTextureRHI.get();

        if (CubeRHI)
            mResourceBinding.SetTexture(0, CubeRHI, SAMPLER_CUBEMAP);
    }

    void SkyRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsBindingsInitialized())
            return;

        // 使用 RenderMaterialProxy 的 IsReady 检查
        if (!mMaterialProxy.IsReady())
            return;

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
        if (Update.IsTexture && mCubeTexture.get() == Update.Resource.get()) mCubeTextureRHI = Update.Texture;
        RHITexture* Cube = mMaterialProxy.GetResolvedTexture(BaseColor);
        mResourceBinding.SetTexture(0, Cube ? Cube : mCubeTextureRHI.get(), SAMPLER_CUBEMAP);
    }
    void SkyRenderProxy::PrepareResourceBinding()
    {

    }

}
