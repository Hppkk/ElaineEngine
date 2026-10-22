#pragma once
#include "ElaineRHIObjectPool.h"
#include "ElaineRHI.h"

namespace Elaine
{
#ifndef ALLOC_COMMAND
#define ALLOC_COMMAND(...) new( AllocCommand(sizeof(__VA_ARGS__), alignof(__VA_ARGS__)) ) __VA_ARGS__
#endif
#ifndef ALLOC_COMMAND_CL
#define ALLOC_COMMAND_CL(RHICmdList, ...) new ( (RHICmdList).AllocCommand(sizeof(__VA_ARGS__), alignof(__VA_ARGS__)) ) __VA_ARGS__
#endif
#ifndef RHI_COMMAND_DEFINE
#define RHI_COMMAND_DEFINE(Command) \
	struct RHICommand##Command :public RHICommand
#endif
#ifndef RHI_COMMAND_TYPE
#define RHI_COMMAND_TYPE(Command) RHICommand##Command
#endif

#ifndef GET_DEFAULT_CMDLIST
#define GET_DEFAULT_CMDLIST RHICommandListManager::instance()->GetDefaultCommandList();
#endif

	class RHICommandContext;
	class RHICommandListManager;
	class ThreadWrap;
	class RHICommandList;

	class ElaineCoreExport RHICommand
	{
	public:
		virtual void Execute(RHICommandList* InCmdList) { }
		virtual ~RHICommand() { }
        void Keep(RHIResource* Resource) { if (Resource) mReferences.emplace_back(Resource); }
        std::vector<RHIHandle> mReferences;
		RHICommand* mNext = nullptr;
		EM_RHICommand mCmd;
	};

	class ElaineCoreExport RHICommandList
	{
	public:
		RHICommandList();
		~RHICommandList();
	public:
        void* AllocCommand(size_t InSize, size_t InAlignment);
        void DiscardCommands(std::exception_ptr Failure = {}) noexcept;
        void SetCompletion(std::shared_ptr<std::promise<void>> Completion) { mCompletion = std::move(Completion); }

		RHICommand* PopCommand();
		void ExecuteCommands();
		RHICommandContext* GetCmdContext() const { return mGraphicsContext; }
		RHICommandContext* GetComputeContext() const { return mComputeContext; }

		void SetShaderUniformBuffer(RHIShader* Shader, uint32 BaseIndex, RHIUniformBuffer* UniformBuffer)
		{

		}
		void BeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState);
		void BeginRenderPassInfo(const RHIRenderPassInfo& InInfo, const char* InName);
		void EndRenderPass();
		void BindGfxPipeline(RHIPipeline* InPipeline);
		void DrawPrimitive(uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances);
		void BindDrawData(RHI_DRAW_RESOURCE_BINDING* InDrawData);
        void SetFrameParameters(const RHIShaderParameterSet& Parameters) { mFrameParameters = Parameters; }
		void SetSwapchain(RHISwapchain* InSwapchain);
		void SetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ);
		void AcquireSwapchainImage(RHISwapchain* InSwapchain);
		void PresentSwapchain(RHISwapchain* InSwapchain, bool bVsync = true);
		void BeginFrame();
		void EndFrame();
        void SubmitUploads();
		void BeginScene();
		void EndScene();
		void SetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY);
		void DrawIndexedPrimitive(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances);
		void SetStreamSource(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset);
        void CopyBufferRegion(RHIBuffer* Dest, uint64 DstOffset, RHIBuffer* Source, uint64 SrcOffset, uint64 Size);
        void CopyBufferToTexture(RHIBuffer* Source, RHITexture* Target);
		void CopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo);
		void ResourceBarrier(const RHIResourceBarrierDesc& Barrier);

		bool HasCommand();

		void SetPriority(uint32 InPriority);
		uint32 GetPriority() const { return mPriority; }
		bool operator<(RHICommandList* InCmdList) const 
		{
			return mPriority < InCmdList->mPriority;
		}
	protected:
        RHIRenderPassInfo mRecordingPass;
        RHIPipelineHandle mRecordingPipeline;
        RHIShaderParameterSet mFrameParameters;
        std::unordered_map<std::string, RHIUniformBufferView> mUniformCache;
        bool mHasRecordingPass = false;
        std::shared_ptr<std::promise<void>> mCompletion;
		RHICommand* mCommandLinkBegin = nullptr;
		RHICommand* mCommandLinkEnd = nullptr;
		size_t	mCommandNum = 0u;
		RHICommandContext* mGraphicsContext = nullptr;
		RHICommandContext* mComputeContext = nullptr;
		uint32 mPriority = 0u;
		RHICommandListManager* mOwner = nullptr;
		size_t mUploadIndex = 0u;
		size_t mListIndex = 0u;
		friend class RHICommandListManager;
	};

	class ElaineCoreExport RHICommandListManager
	{
	public:
		//现阶段仅支持单个RHI线程执行命令队列
		enum EM_ExecuteMode
		{
			SingleThread,
			MultiThread,
		};
		RHICommandListManager(RHICommandContext* InCtx);
		~RHICommandListManager();
        void Clear();
		RHICommandList* GetDefaultCommandList() const { return mDefaultCommandList; }
		RHICommandList* GetCurrentCommandList() const { return mCurrentCommandList; }
		RHICommandList* CreateCommandList();
		void DestroyCommandList(RHICommandList* InCmdList);
		void SwapCommands();
		void ExecuteCommands();
	private:
		RHICommandList* mDefaultCommandList;
		EM_ExecuteMode  mExecuteMode = SingleThread;
		std::vector<RHICommandList*> mCmdLists;
		std::vector<RHICommandList*> mUploadCmdLists;
		std::vector<RHICommandList*> mLogicCmdList;
		std::vector<RHICommandList*> mFreeCmdList;
		std::vector<RHICommandList*> mWaitDetroyCmdLists;

		RHICommandContext* mRHICommandCtx = nullptr;
		RHICommandList* mCurrentCommandList = nullptr;
		friend class RHICommandList;
	};


	//------------------------RHI Commands Define--------------------------------

	RHI_COMMAND_DEFINE(DrawPrimitive)
	{
		RHI_COMMAND_TYPE(DrawPrimitive)(uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
			: mBaseVertexIndex(InBaseVertexIndex)
			, mNumPrimitives(InNumPrimitives)
			, mNumInstances(InNumInstances)
		{
		}

		void Execute(RHICommandList* InCmdList);

		uint32 mBaseVertexIndex = 0;
		uint32 mNumPrimitives = 0;
		uint32 mNumInstances = 0;
	};

	RHI_COMMAND_DEFINE(BeginRenderPass)
	{
		RHI_COMMAND_TYPE(BeginRenderPass)(const GRAPHICS_PIPELINE_STATE_DESC & InGfxState)
			: mGfxState(InGfxState)
		{}

		void Execute(RHICommandList * InCmdList);
		GRAPHICS_PIPELINE_STATE_DESC mGfxState;
	};

	RHI_COMMAND_DEFINE(EndRenderPass)
	{
		RHI_COMMAND_TYPE(EndRenderPass)() {}

		void Execute(RHICommandList * InCmdList);
	};

	RHI_COMMAND_DEFINE(BeginRenderPassInfo)
	{
		RHI_COMMAND_TYPE(BeginRenderPassInfo)(const RHIRenderPassInfo& InInfo, const char* InName)
			: mRenderPassInfo(InInfo)
			, mName(InName ? InName : "")
        {
            for (auto& Color : InInfo.ColorRenderTargets) { Keep(Color.RenderTarget); Keep(Color.ResolveTarget); }
            Keep(InInfo.DepthStencilRenderTarget.DepthStencilTarget);
            Keep(InInfo.DepthStencilRenderTarget.ResolveTarget);
        }

		void Execute(RHICommandList* InCmdList);
		RHIRenderPassInfo mRenderPassInfo;
		std::string mName;
	};

	RHI_COMMAND_DEFINE(BindGfxPipeline)
	{
		RHI_COMMAND_TYPE(BindGfxPipeline)(RHIPipeline * InGfxPipeline)
			: mGfxPipeline(InGfxPipeline)
		{
            Keep(InGfxPipeline);
		}

		void Execute(RHICommandList * InCmdList);
		RHIPipelineHandle mGfxPipeline = nullptr;
	};

	RHI_COMMAND_DEFINE(BindDrawData)
	{
		RHI_COMMAND_TYPE(BindDrawData)(RHI_DRAW_RESOURCE_BINDING * InRenderData)
			: mRenderData(*InRenderData)
		{
            for (const auto& Resource : mRenderData.mDrawData.mStreamInput.mIStreamBuffer) Keep(Resource.get());
            Keep(mRenderData.mIndexBuffer.get());
            for (const auto& Group : mRenderData.Parameters)
            {
                for (const auto& Pair : Group.Uniforms) Keep(Pair.second.Buffer.get());
                for (const auto& Pair : Group.Textures) Keep(Pair.second.Texture.get());
                for (const auto& Pair : Group.Buffers) Keep(Pair.second.Buffer.get());
            }
            for (const auto& Resource : mRenderData.mRenderTargets) Keep(Resource.get());
            Keep(mRenderData.mDepthStencilTarget.get());
		}

		void Execute(RHICommandList* InCmdList);

		RHI_DRAW_RESOURCE_BINDING mRenderData;
	};

	RHI_COMMAND_DEFINE(SetViewport)
	{
		RHI_COMMAND_TYPE(SetViewport)(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
		{
			mMinX = MinX;
			mMinY = MinY;
			mMinZ = MinZ;
			mMaxX = MaxX;
			mMaxY = MaxY;
			mMaxZ = MaxZ;
		}

		void Execute(RHICommandList * InCmdList);

		float mMinX; 
		float mMinY;
		float mMinZ;
		float mMaxX;
		float mMaxY;
		float mMaxZ;
	};

	RHI_COMMAND_DEFINE(SetSwapchain)
	{
		RHI_COMMAND_TYPE(SetSwapchain)(RHISwapchain* InSwapchain)
			: mSwapchain(InSwapchain)
		{
            Keep(InSwapchain);
		}

		void Execute(RHICommandList * InCmdList);

		RHISwapchainHandle mSwapchain;
	};

	RHI_COMMAND_DEFINE(AcquireSwapchainImage)
	{
		RHI_COMMAND_TYPE(AcquireSwapchainImage)(RHISwapchain * InSwapchain)
			: mSwapchain(InSwapchain)
		{
            Keep(InSwapchain);
		}

		void Execute(RHICommandList * InCmdList);
		RHISwapchainHandle mSwapchain;
	};

	RHI_COMMAND_DEFINE(PresentSwapchain)
	{
		RHI_COMMAND_TYPE(PresentSwapchain)(RHISwapchain * InSwapchain, bool bVsync = true)
			: mSwapchain(InSwapchain)
			, mbVsync(bVsync)
		{
            Keep(InSwapchain);
		}

		void Execute(RHICommandList * InCmdList);
		RHISwapchainHandle mSwapchain;
		bool mbVsync;
	};

	RHI_COMMAND_DEFINE(BeginFrame)
	{
		RHI_COMMAND_TYPE(BeginFrame)() {}
		void Execute(RHICommandList* InCmdList);
	};

	RHI_COMMAND_DEFINE(EndFrame)
	{
		RHI_COMMAND_TYPE(EndFrame)() {}
		void Execute(RHICommandList* InCmdList);
	};

	RHI_COMMAND_DEFINE(BeginScene)
	{
		RHI_COMMAND_TYPE(BeginScene)() {}
		void Execute(RHICommandList* InCmdList);
	};

	RHI_COMMAND_DEFINE(EndScene)
	{
		RHI_COMMAND_TYPE(EndScene)() {}
		void Execute(RHICommandList* InCmdList);
	};

	RHI_COMMAND_DEFINE(SetScissorRect)
	{
		RHI_COMMAND_TYPE(SetScissorRect)(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
			: mbEnable(bEnable)
			, mMinX(MinX)
			, mMinY(MinY)
			, mMaxX(MaxX)
			, mMaxY(MaxY)
		{}
		void Execute(RHICommandList* InCmdList);

		bool mbEnable;
		uint32 mMinX;
		uint32 mMinY;
		uint32 mMaxX;
		uint32 mMaxY;
	};

	RHI_COMMAND_DEFINE(DrawIndexedPrimitive)
	{
		RHI_COMMAND_TYPE(DrawIndexedPrimitive)(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
			: mIndexBuffer(IndexBuffer)
			, mBaseVertexIndex(BaseVertexIndex)
			, mFirstInstance(FirstInstance)
			, mNumVertices(NumVertices)
			, mStartIndex(StartIndex)
			, mNumPrimitives(NumPrimitives)
			, mNumInstances(NumInstances)
		{
            Keep(IndexBuffer);}
		void Execute(RHICommandList* InCmdList);

		RHIBufferHandle mIndexBuffer;
		int32 mBaseVertexIndex;
		uint32 mFirstInstance;
		uint32 mNumVertices;
		uint32 mStartIndex;
		uint32 mNumPrimitives;
		uint32 mNumInstances;
	};

	RHI_COMMAND_DEFINE(SetStreamSource)
	{
		RHI_COMMAND_TYPE(SetStreamSource)(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset)
			: mStreamIndex(StreamIndex)
			, mVertexBuffer(VertexBuffer)
			, mOffset(Offset)
		{
            Keep(VertexBuffer);}
		void Execute(RHICommandList* InCmdList);

		uint32 mStreamIndex;
		RHIBufferHandle mVertexBuffer;
		uint32 mOffset;
	};

	RHI_COMMAND_DEFINE(CopyTexture)
	{
		RHI_COMMAND_TYPE(CopyTexture)(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo)
			: mSourceTexture(SourceTexture)
			, mDestTexture(DestTexture)
			, mCopyInfo(CopyInfo)
		{
            Keep(SourceTexture);
            Keep(DestTexture);}
		void Execute(RHICommandList* InCmdList);

		RHITextureHandle mSourceTexture;
		RHITextureHandle mDestTexture;
		RHICopyTextureInfo mCopyInfo;
	};

	RHI_COMMAND_DEFINE(ResourceBarrier)
	{
		RHI_COMMAND_TYPE(ResourceBarrier)(const RHIResourceBarrierDesc& Barrier)
			: mBarrier(Barrier)
        { Keep(Barrier.Texture); Keep(Barrier.Buffer); }
		void Execute(RHICommandList* InCmdList);

		RHIResourceBarrierDesc mBarrier;
	};

    RHI_COMMAND_DEFINE(CopyBufferRegion)
    {
        RHI_COMMAND_TYPE(CopyBufferRegion)(RHIBuffer* Dest, uint64 DstOffset, RHIBuffer* Source, uint64 SrcOffset, uint64 Size)
            : mDest(Dest), mSource(Source), mDstOffset(DstOffset), mSrcOffset(SrcOffset), mSize(Size) { Keep(Dest); Keep(Source); }
        void Execute(RHICommandList* List);
        RHIBufferHandle mDest; RHIBufferHandle mSource;
        uint64 mDstOffset, mSrcOffset, mSize;
    };
    RHI_COMMAND_DEFINE(CopyBufferToTexture)
    {
        RHI_COMMAND_TYPE(CopyBufferToTexture)(RHIBuffer* Source, RHITexture* Target)
            : mSource(Source), mTarget(Target) { Keep(Source); Keep(Target); }
        void Execute(RHICommandList* List);
        RHIBufferHandle mSource; RHITextureHandle mTarget;
    };

    RHI_COMMAND_DEFINE(SubmitUploads)
    {
        void Execute(RHICommandList* List);
    };

}
