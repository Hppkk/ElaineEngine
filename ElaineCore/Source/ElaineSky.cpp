#include "ElainePrecompiledHeader.h"
#include "ElaineSky.h"
#include "ElaineRenderQueue.h"
#include "ElaineMaterial.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineDataStreamMgr.h"
#include "ElaineTextureResource.h"
#include "TaskGraph/ElaineTaskGraph.h"

namespace Elaine
{
    SkyObject::SkyObject(SceneNode* InParent, SceneManager* InSceneMgr)
        :RenderableObject(InParent, InSceneMgr)
    {
        m_RenderGeneralData.mVertexDatas = {
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
        //mMaterial = new Material();
        ////mMaterial->BeginPass(NormalPass);
        ////Pass* CurrentPass = mMaterial->GetPass(NormalPass);

        //struct ResourcePtrHolder
        //{
        //    ResourceBasePtr vs;
        //    ResourceBasePtr ps;
        //};

        //std::shared_ptr<ResourcePtrHolder> ResourcePtrHolderPtr = std::make_shared<ResourcePtrHolder>();
        //TaskGraph::GraphTaskDesc desc;
        //desc.mTaskFunction = [ResourcePtrHolderPtr]
        //    {
        //        ResourcePtrHolderPtr->vs = DataStreamMgr::instance()->getDataStreamFromFile(Root::instance()->GetResourcePath() + "shader/vulkan/sky_vs.txt");
        //        ResourcePtrHolderPtr->ps = DataStreamMgr::instance()->getDataStreamFromFile(Root::instance()->GetResourcePath() + "shader/vulkan/sky_ps.txt");
        //    };
        //desc.mExecutedThread = NamedThread::StreamingThread1;

        //TaskGraph::GraphTaskDesc callback;
        //callback.mTaskFunction = [ResourcePtrHolderPtr, this]
        //    {
        //        mMaterial->BeginPass(NormalPass);
        //        ShaderPass* CurrentPass = mMaterial->GetPass(NormalPass);

        //        {
        //            DataStream* ds = static_cast<DataStream*>(ResourcePtrHolderPtr->vs.get());
        //            auto stream = ds->GetDataStream();
        //            CurrentPass->mRHIDesc.mVSShaderCode = stream;
        //            CurrentPass->mRHIDesc.mVSPath = Root::instance()->GetResourcePath() + "shader/vulkan/sky_vs.txt";
        //        }

        //        {
        //            DataStream* ds = static_cast<DataStream*>(ResourcePtrHolderPtr->ps.get());
        //            auto stream = ds->GetDataStream();
        //            CurrentPass->mRHIDesc.mPSShaderCode = stream;
        //            CurrentPass->mRHIDesc.mPSPath = Root::instance()->GetResourcePath() + "shader/vulkan/sky_ps.txt";
        //        }

        //        CurrentPass->mRHIDesc.mDepthOp = COMPARE_OP_LESS_OR_EQUAL;

        //        CurrentPass->mRHIDesc.mRHIDrawData = &m_RenderGeneralData.mDrawData;

        //        CurrentPass->mRHIDesc.mVertexAttribute.mAttributeSize = 3;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[0] = sizeof(float) * 3;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[1] = sizeof(float) * 2;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mOffset[2] = sizeof(float) * 3;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[0] = VET_Float3;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[1] = VET_Float2;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mFormat[2] = VET_Float3;
        //        CurrentPass->mRHIDesc.mVertexAttribute.mStride = sizeof(VertexData);
        //        //CurrentPass->mRHIDesc.
        //        mMaterial->EndPass(NormalPass);
        //        std::vector<std::string> CubePaths{
        //            "texture/sky/right.jpg",
        //            "texture/sky/left.jpg", 
        //            "texture/sky/top.jpg",
        //            "texture/sky/bottom.jpg",
        //            "texture/sky/front.jpg",
        //            "texture/sky/back.jpg"
        //        };

        //        Texture* FTexture = new Texture(nullptr, TextureDimension::TextureCube, "", CubePaths);
        //        FTexture->LoadResource();
        //        CurrentPass->mRHIDesc.mTextures[0] = FTexture->GetTextureRHI();
        //    };
        //callback.mExecutedThread = NamedThread::RenderThread;
        //desc.SubsequentTask(callback);
        //TaskGraph::GraphTaskCreateDesc createDesc;
        //createDesc.mDirectTasks.push_back(desc);
        //TaskGraph::TaskGraph::instance()->CreateAndDispatchWhenReady(createDesc);
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
        //if (mMaterial->GetPass(NormalPass) == nullptr)
        //    return;

        //RenderQueue* CurrentRenderQueue = InRenderQueueSet->GetRenderQueue(RenderQueue_Sky);
        //CurrentRenderQueue->UpdateRenderQueue(mMaterial->GetPass(NormalPass), this, -1000);
    }

    void SkyObject::RecordRenderCommand(RHICommandList* InRHICommandList)
    {
        InRHICommandList->DrawPrimitive(0, m_RenderGeneralData.mVertexDatas.size(), 1);
    }
}