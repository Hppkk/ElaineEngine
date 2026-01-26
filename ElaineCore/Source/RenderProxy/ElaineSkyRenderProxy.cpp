#include "ElainePrecompiledHeader.h"
#include "RenderProxy/ElaineSkyRenderProxy.h"
#include "ElaineRenderQueue.h"
#include "ElaineMaterial.h"
#include "ElaineMaterialInstanceDynamic.h"

namespace Elaine
{
    SkyRenderProxy::SkyRenderProxy()
    {
        mType = EProxyType::Sky;
        mWorldAABB = AxisAlignedBox(Vector3(-2000), Vector3(2000));
    }

    SkyRenderProxy::~SkyRenderProxy()
    {

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
        mResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] =
            RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer, ERHIAccess::VertexOrIndexBuffer,
                CpuData.data(), CpuData.size() * sizeof(VertexData));

        mResourceBinding.mVertexCount = 36;
        mResourceBinding.mInstanceCount = 1;
        mCubeTexture = mMaterial->GetTexture(BaseColor);
        if (!mCubeTexture.isNull() && mCubeTexture->GetTextureRHI())
            mResourceBinding.SetTexture(0, mCubeTexture->GetTextureRHI(), SAMPLER_CUBEMAP);

        //The RHI layer has not undergone matching verification, uses data obtained from Shader reflection.
        
        //CurrentPass->mRHIDesc.mDepthOp = COMPARE_OP_LESS_OR_EQUAL;
        //CurrentPass->mRHIDesc.mVertexAttribute.mAttributeSize = 3;
        //CurrentPass->mRHIDesc.mVertexAttribute.mOffset[0] = sizeof(float) * 3;
        //CurrentPass->mRHIDesc.mVertexAttribute.mOffset[1] = sizeof(float) * 2;
        //CurrentPass->mRHIDesc.mVertexAttribute.mOffset[2] = sizeof(float) * 3;
        //CurrentPass->mRHIDesc.mVertexAttribute.mFormat[0] = VET_Float3;
        //CurrentPass->mRHIDesc.mVertexAttribute.mFormat[1] = VET_Float2;
        //CurrentPass->mRHIDesc.mVertexAttribute.mFormat[2] = VET_Float3;
        //CurrentPass->mRHIDesc.mVertexAttribute.mStride = sizeof(VertexData);


    }

    void SkyRenderProxy::UpdateRenderQueue(RenderQueueSet* InRenderQueue)
    {
        if (!IsBindingsInitialized())
            return;

        // 使用IsReady确保Material完全加载完成
        if (mMaterial == nullptr || !mMaterial->IsReady())
            return;

        ShaderPass* SkyPass = mMaterial->GetPass(Name("Sky"));
        if (SkyPass == nullptr)
            return;

        RenderQueue* CurrentRenderQueue = InRenderQueue->GetRenderQueue(RenderQueue_Sky);
        CurrentRenderQueue->UpdateRenderQueue(SkyPass, this, -1000);
    }

    void SkyRenderProxy::PrepareResourceBinding()
    {

    }

}
