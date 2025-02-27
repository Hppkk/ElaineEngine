#include "ElainePrecompiledHeader.h"
#include "ElaineSky.h"
#include "ElaineRenderQueue.h"
#include "ElaineMaterial.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineDataStreamMgr.h"

namespace Elaine
{
    SkyObject::SkyObject(SceneNode* InParent, SceneManager* InSceneMgr)
        :RenderableObject(InParent, InSceneMgr)
    {
        m_RenderGeneralData.mVertexDatas = {
            {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f},
            { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f},
            { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f},
            { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f},
            {-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f},
            {-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f},
                                                                 
            {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f},
            { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f},
            { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f},
            { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f},
            {-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f},
            {-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f},
                                                                 
            {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
            {-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
            {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            {-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            {-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f},
            {-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
                                                               
            { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
            { 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f},
            { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f},
            { 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f},
            { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f},
                                                                
            {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f},
            { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f},
            { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f},
            { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f},
            {-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f},
            {-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f},
                                                                
            {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f},
            { 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f},
            { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f},
            { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f},
            {-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f},
            {-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f},
        };

        m_RenderGeneralData.mDrawData.mStreamInput.mIStreamBuffer[STREAM_VERTEXBUFFER] =
            RenderSystem::instance()->CreateBuffer(BufferUsageFlags::VertexBuffer, ERHIAccess::VertexOrIndexBuffer,
            m_RenderGeneralData.mVertexDatas.data(), m_RenderGeneralData.mVertexDatas.size() * sizeof(VertexData));

        InitilizeMaterial();
    }

    SkyObject::~SkyObject()
    {

    }

    void SkyObject::InitilizeMaterial()
    {
        mMaterial = new Material();
        mMaterial->BeginPass(NormalPass);
        Pass* CurrentPass = mMaterial->GetPass(NormalPass);

        {
            ResourceBasePtr res = DataStreamMgr::instance()->getDataStreamFromFile(Root::instance()->getResourcePath() + "shader/vulkan/sky_vs.txt");
            DataStream* ds = static_cast<DataStream*>(res.get());
            auto stream = ds->getDataStream();
            CurrentPass->mRHIDesc.mVSShaderCode = stream;
        }

        {
            ResourceBasePtr res = DataStreamMgr::instance()->getDataStreamFromFile(Root::instance()->getResourcePath() + "shader/vulkan/sky_ps.txt");
            DataStream* ds = static_cast<DataStream*>(res.get());
            auto stream = ds->getDataStream();
            CurrentPass->mRHIDesc.mPSShaderCode = stream;
        }

        CurrentPass->mRHIDesc.mRHIDrawData = &m_RenderGeneralData.mDrawData;

        CurrentPass->mRHIDesc.mVertexAttribute.mAttributeSize = 3;
        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[0] = sizeof(float) * 3;
        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[1] = sizeof(float) * 2;
        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[2] = sizeof(float) * 3;
        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[0] = VET_Float3;
        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[1] = VET_Float2;
        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[2] = VET_Float3;
        CurrentPass->mRHIDesc.mVertexAttribute.mStride = sizeof(VertexData);
        //CurrentPass->mRHIDesc.
        mMaterial->EndPass(NormalPass);
    }

    void SkyObject::SynchRenderData()
    {

    }

    void SkyObject::NotifyCurrentCamera(Camera* InCamera)
    {
        m_bIsCulled = false;
    }

    void SkyObject::UpdateRenderQueue(RenderQueueSet* InRenderQueueSet)
    {
        RenderQueue* CurrentRenderQueue = InRenderQueueSet->GetRenderQueue(RenderQueue_Sky);
        CurrentRenderQueue->UpdateRenderQueue(mMaterial->GetPass(NormalPass), this, -1000);
    }

    void SkyObject::RecordRenderCommand(RHICommandList* InRHICommandList)
    {
        InRHICommandList->DrawPrimitive(0, 1, 1);
    }
}