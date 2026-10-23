#include "ElainePrecompiledHeader.h"
#include "TaskGraph/ElaineTask.h"
#include "render/ElaineShaderParameters.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHI.h"
#include "render/ElaineRenderSystem.h"
#include <cstring>
#include "../RenderDoc/ElaineRenderDocCaptureInternal.h"

namespace Elaine
{
	RHICommandList::RHICommandList()
	{
		
	}

    RHICommandList::~RHICommandList() { DiscardCommands(); }

    void RHICommandList::DiscardCommands(std::exception_ptr Failure) noexcept
    {
        while (mCommandLinkBegin)
        {
            auto* Command = mCommandLinkBegin;
            mCommandLinkBegin = Command->mNext;
            Command->~RHICommand();
            free(Command);
        }
        mCommandLinkEnd = nullptr; mCommandNum = 0;
        mUniformCache.clear(); mRecordingPipeline.reset(); mFrameParameters = {};
        if (mSubmissionEvent)
        {
            auto Event = std::move(mSubmissionEvent);
            if (Failure) Event->Fail(Failure); else Event->Cancel();
        }
        if (mCompletion)
        {
            try { mCompletion->set_exception(Failure ? Failure : std::make_exception_ptr(std::runtime_error("RHI command batch abandoned"))); } catch (...) {}
            mCompletion.reset();
        }
    }

	void* RHICommandList::AllocCommand(size_t InSize, size_t InAlignment)
	{
		
		RHICommand* Result = (RHICommand*)malloc(InSize);
		++mCommandNum;
		if (mCommandLinkBegin == nullptr)
			mCommandLinkBegin = Result;

		if (mCommandLinkEnd)
			mCommandLinkEnd->mNext = Result;
		mCommandLinkEnd = Result;

		//mCommandLinks = &Result->mNext;
		
		return Result;
	}

	RHICommand* RHICommandList::PopCommand()
	{
		if (mCommandNum == 0)
		{
			mOwner->mUploadCmdLists.erase(mOwner->mUploadCmdLists.begin() + mUploadIndex);
			mUploadIndex = 0;
		}
		return nullptr;
	}

    void RHICommandList::ExecuteCommands()
    {
        try
        {
            while (mCommandLinkBegin)
            {
                auto* Command = mCommandLinkBegin;
                // Keep the CPU payload until execution ends; the backend pins GPU references.
                if (mGraphicsContext && !Command->mReferences.empty()) mGraphicsContext->RHIKeepResources(Command->mReferences);
                Command->Execute(this);
                mCommandLinkBegin = Command->mNext;
                --mCommandNum;
                Command->~RHICommand();
                free(Command);
            }
            mCommandLinkEnd = nullptr;
            mUniformCache.clear(); mRecordingPipeline.reset(); mFrameParameters = {};
            if (mSubmissionEvent) { auto Event = std::move(mSubmissionEvent); Event->Succeed(); }
            if (mCompletion) { mCompletion->set_value(); mCompletion.reset(); }
        }
        catch (...)
        {
            auto Failure = std::current_exception();
            RenderDocInternal::AbortFrame("RHI command batch failed");
            // Submit any partially recorded work so its fence can retire references.
            try { if (mGraphicsContext) mGraphicsContext->RHIEndFrame(); } catch (...) {}
            DiscardCommands(Failure);
            throw;
        }
    }

	void RHICommandList::BeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BeginRenderPass))(InGfxState);
	}

	void RHICommandList::BeginRenderPassInfo(const RHIRenderPassInfo& InInfo, const char* InName)
	{
        mRecordingPass = InInfo; mHasRecordingPass = true;
        ALLOC_COMMAND(RHI_COMMAND_TYPE(BeginRenderPassInfo))(InInfo, InName);
	}

	void RHICommandList::EndRenderPass()
	{
        mHasRecordingPass = false;
        ALLOC_COMMAND(RHI_COMMAND_TYPE(EndRenderPass))();
		//{
		//	auto Iter = std::upper_bound(mOwner->mLogicCmdList.rbegin(), mOwner->mLogicCmdList.rend(), this);
		//	if (Iter == mOwner->mLogicCmdList.rend())
		//	{
		//		mOwner->mLogicCmdList.insert(mOwner->mLogicCmdList.begin(), this);
		//		mUploadIndex = 0u;
		//	}
		//	else
		//	{
		//		size_t FindIndex = mOwner->mLogicCmdList.rbegin() - Iter;

		//		mOwner->mLogicCmdList.insert(mOwner->mLogicCmdList.begin() + FindIndex + 1, this);
		//		mUploadIndex = FindIndex + 1;
		//	}
		//}
	}

	void RHICommandList::BindGfxPipeline(RHIPipeline* InPipeline)
	{
        mRecordingPipeline = InPipeline;
        if (mHasRecordingPass && mGraphicsContext) mGraphicsContext->RHIPrepareGfxPipeline(InPipeline, mRecordingPass);
        ALLOC_COMMAND(RHI_COMMAND_TYPE(BindGfxPipeline))(InPipeline);
	}

	void RHICommandList::DrawPrimitive(uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(DrawPrimitive))(InBaseVertexIndex, InNumPrimitives, InNumInstances);
	}

	void RHICommandList::BindDrawData(RHI_DRAW_RESOURCE_BINDING* InDrawData)
	{
        if (!InDrawData || !mRecordingPipeline) throw std::logic_error("Draw binding requires a pipeline");
        auto Binding = *InDrawData;
        auto& Frame = Binding.GetParameters(RHIResourceGroup::PerFrame);
        Frame.Values.insert(mFrameParameters.Values.begin(), mFrameParameters.Values.end());
        Frame.BlockValues.insert(mFrameParameters.BlockValues.begin(), mFrameParameters.BlockValues.end());
        Frame.Textures.insert(mFrameParameters.Textures.begin(), mFrameParameters.Textures.end());
        Frame.Buffers.insert(mFrameParameters.Buffers.begin(), mFrameParameters.Buffers.end());
        Frame.Uniforms.insert(mFrameParameters.Uniforms.begin(), mFrameParameters.Uniforms.end());
        Frame.TextureDefaults.insert(mFrameParameters.TextureDefaults.begin(), mFrameParameters.TextureDefaults.end());
        for (const auto& Layout : mRecordingPipeline->GetResourceLayout())
        {
            auto& Group = Binding.Parameters[Layout.Group];
            if (Layout.Type == RHIShaderResourceType::Texture)
            {
                auto It = Group.Textures.find(Layout.Name);
                if (It == Group.Textures.end() || !It->second.Texture)
                {
                    auto Default = Group.TextureDefaults.find(Layout.Name);
                    if (Default == Group.TextureDefaults.end() || Default->second == RHIDefaultTexture::None ||
                        Layout.Dimension != TextureDimension::Texture2D || Layout.IntegerSample || Layout.DepthSample || Layout.Multisampled)
                        throw std::runtime_error("Required texture is not bound: " + Layout.Name);
                    Group.Textures[Layout.Name].Texture = RenderSystem::instance()->GetDefaultTexture(Default->second);
                }
                continue;
            }
            if (Layout.Type != RHIShaderResourceType::Uniform) continue;
            if (Group.Uniforms.count(Layout.Name)) continue;
            auto Bytes = PackShaderUniform(Layout, Group);
            const std::string Key(reinterpret_cast<const char*>(Bytes.data()), Bytes.size());
            auto Cached = mUniformCache.find(Key);
            if (Cached == mUniformCache.end()) Cached = mUniformCache.emplace(Key, mGraphicsContext->RHIAllocateUniformData(Bytes.data(), Bytes.size())).first;
            Group.Uniforms[Layout.Name] = Cached->second;
        }
        ALLOC_COMMAND(RHI_COMMAND_TYPE(BindDrawData))(&Binding);
	}

	void RHICommandList::SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(SetViewport))(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
	}

	void RHICommandList::SetSwapchain(RHISwapchain* InSwapchain)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(SetSwapchain))(InSwapchain);
	}

	void RHICommandList::AcquireSwapchainImage(RHISwapchain* InSwapchain)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(AcquireSwapchainImage))(InSwapchain);
	}

	void RHICommandList::PresentSwapchain(RHISwapchain* InSwapchain, bool bVsync)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(PresentSwapchain))(InSwapchain, bVsync);
	}

	void RHICommandList::BeginFrame()
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BeginFrame))();
	}

	void RHICommandList::EndFrame()
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(EndFrame))();
	}

	void RHICommandList::BeginScene()
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BeginScene))();
	}

	void RHICommandList::EndScene()
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(EndScene))();
	}

	void RHICommandList::SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(SetScissorRect))(bEnable, MinX, MinY, MaxX, MaxY);
	}

	void RHICommandList::DrawIndexedPrimitive(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(DrawIndexedPrimitive))(IndexBuffer, BaseVertexIndex, FirstInstance, NumVertices, StartIndex, NumPrimitives, NumInstances);
	}

	void RHICommandList::SetStreamSource(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(SetStreamSource))(StreamIndex, VertexBuffer, Offset);
	}

	void RHICommandList::CopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(CopyTexture))(SourceTexture, DestTexture, CopyInfo);
	}

	void RHICommandList::ResourceBarrier(const RHIResourceBarrierDesc& Barrier)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(ResourceBarrier))(Barrier);
	}

	bool RHICommandList::HasCommand()
	{
		return mCommandNum != 0;
	}

	void RHICommandList::SetPriority(uint32 InPriority)
	{
		mPriority = InPriority;
		if (HasCommand())
		{
			mOwner->mLogicCmdList.erase(mOwner->mLogicCmdList.begin() + mUploadIndex);
			auto Iter = std::upper_bound(mOwner->mLogicCmdList.rbegin(), mOwner->mLogicCmdList.rend(), this);
			if (Iter == mOwner->mLogicCmdList.rend())
			{
				mOwner->mLogicCmdList.insert(mOwner->mLogicCmdList.begin(), this);
				mUploadIndex = 0u;
			}
			else
			{
				size_t FindIndex = mOwner->mLogicCmdList.rbegin() - Iter;
				
				mOwner->mLogicCmdList.insert(mOwner->mLogicCmdList.begin() + FindIndex + 1, this);
				mUploadIndex = FindIndex + 1;
			}
		}
	}

	RHICommandListManager::RHICommandListManager(RHICommandContext* InCtx)
		: mDefaultCommandList(nullptr)
		, mRHICommandCtx(InCtx)
	{
		//mDefaultCommandList = CreateCommandList();
	}

    RHICommandListManager::~RHICommandListManager() { Clear(); }
    void RHICommandListManager::Clear()
    {
        for (auto* List : mCmdLists) delete List;
        mCmdLists.clear(); mLogicCmdList.clear(); mUploadCmdLists.clear(); mFreeCmdList.clear();
        mDefaultCommandList = mCurrentCommandList = nullptr;
    }
	RHICommandList* RHICommandListManager::CreateCommandList()
	{
		//todo freelist

		RHICommandList* NewCommandList = new RHICommandList(); //mRHICmdListAllocation.CreateObject();
		mCurrentCommandList = NewCommandList;
		NewCommandList->mListIndex = mCmdLists.size();
		mCmdLists.push_back(NewCommandList);
		mLogicCmdList.push_back(NewCommandList);
		NewCommandList->mGraphicsContext = mRHICommandCtx;
		NewCommandList->mOwner = this;
		return NewCommandList;
	}

    void RHICommandListManager::DestroyCommandList(RHICommandList* List)
    {
        auto Remove = [List](auto& Lists) { Lists.erase(std::remove(Lists.begin(), Lists.end(), List), Lists.end()); };
        Remove(mCmdLists); Remove(mLogicCmdList); Remove(mUploadCmdLists); Remove(mFreeCmdList); Remove(mWaitDetroyCmdLists);
        if (mCurrentCommandList == List) mCurrentCommandList = nullptr;
        if (mDefaultCommandList == List) mDefaultCommandList = nullptr;
        delete List;
    }

	void RHICommandListManager::SwapCommands()
	{
        auto Completed = std::move(mFreeCmdList);
        mFreeCmdList.clear();
        for (auto* List : Completed) DestroyCommandList(List);
        std::swap(mLogicCmdList, mUploadCmdLists);
	}

    void RHICommandListManager::ExecuteCommands()
    {
        std::exception_ptr Failure;
        for (auto* List : mUploadCmdLists)
        {
            try { List->ExecuteCommands(); } catch (...) { if (!Failure) Failure = std::current_exception(); }
            mFreeCmdList.push_back(List);
        }
        mUploadCmdLists.clear();
        if (Failure) std::rethrow_exception(Failure);
    }

    //-----------------------------RHI Command Define-------------------------------------------

	void RHI_COMMAND_TYPE(DrawPrimitive)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIDrawPrimitive(mBaseVertexIndex, mNumPrimitives, mNumInstances);
	}

	void RHI_COMMAND_TYPE(BeginRenderPass)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBeginRenderPass(mGfxState);
	}

	void RHI_COMMAND_TYPE(EndRenderPass)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIEndRenderPass();
	}

	void RHI_COMMAND_TYPE(BeginRenderPassInfo)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBeginRenderPass(mRenderPassInfo, mName.c_str());
	}

	void RHI_COMMAND_TYPE(BindGfxPipeline)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBindGfxPipeline(mGfxPipeline.get());
	}

	void RHI_COMMAND_TYPE(BindDrawData)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBindResourceBinding(mRenderData);
	}

	void RHI_COMMAND_TYPE(SetViewport)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHISetViewport(mMinX, mMinY, mMinZ, mMaxX, mMaxY, mMaxZ);
	}

	void RHI_COMMAND_TYPE(SetSwapchain)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHISetSwapchain(mSwapchain.get());
	}

	void RHI_COMMAND_TYPE(AcquireSwapchainImage)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIAcquireSwapchainImage(mSwapchain.get());
	}

	void RHI_COMMAND_TYPE(PresentSwapchain)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIPresentSwapchain(mSwapchain.get(), mbVsync);
	}

	//-----------------------------新增命令 Execute-------------------------------------------

	void RHI_COMMAND_TYPE(BeginFrame)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBeginFrame();
	}

	void RHI_COMMAND_TYPE(EndFrame)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIEndFrame();
	}

	void RHI_COMMAND_TYPE(BeginScene)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBeginScene();
	}

	void RHI_COMMAND_TYPE(EndScene)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIEndScene();
	}

	void RHI_COMMAND_TYPE(SetScissorRect)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHISetScissorRect(mbEnable, mMinX, mMinY, mMaxX, mMaxY);
	}

	void RHI_COMMAND_TYPE(DrawIndexedPrimitive)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIDrawIndexedPrimitive(mIndexBuffer.get(), mBaseVertexIndex, mFirstInstance, mNumVertices, mStartIndex, mNumPrimitives, mNumInstances);
	}

	void RHI_COMMAND_TYPE(SetStreamSource)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHISetStreamSource(mStreamIndex, mVertexBuffer.get(), mOffset);
	}

	void RHI_COMMAND_TYPE(CopyTexture)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHICopyTexture(mSourceTexture.get(), mDestTexture.get(), mCopyInfo);
	}

	void RHI_COMMAND_TYPE(ResourceBarrier)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIResourceBarrier(mBarrier);
	}
    void RHICommandList::CopyBufferRegion(RHIBuffer* Dest, uint64 DstOffset, RHIBuffer* Source, uint64 SrcOffset, uint64 Size)
    { ALLOC_COMMAND(RHI_COMMAND_TYPE(CopyBufferRegion))(Dest, DstOffset, Source, SrcOffset, Size); }
    void RHICommandList::CopyBufferToTexture(RHIBuffer* Source, RHITexture* Target)
    { ALLOC_COMMAND(RHI_COMMAND_TYPE(CopyBufferToTexture))(Source, Target); }
    void RHI_COMMAND_TYPE(CopyBufferRegion)::Execute(RHICommandList* List)
    { List->GetCmdContext()->RHICopyBufferRegion(mDest.get(), mDstOffset, mSource.get(), mSrcOffset, mSize); }
    void RHI_COMMAND_TYPE(CopyBufferToTexture)::Execute(RHICommandList* List)
    { List->GetCmdContext()->RHICopyBufferToTexture(mSource.get(), mTarget.get()); }

    void RHICommandList::SubmitUploads() { ALLOC_COMMAND(RHI_COMMAND_TYPE(SubmitUploads))(); }
    void RHI_COMMAND_TYPE(SubmitUploads)::Execute(RHICommandList* List) { List->GetCmdContext()->RHISubmitUploads(); }

}
