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

		template<class TCmd>
		void* AllocCommand()
		{
			return AllocCommand(sizeof(TCmd), alignof(TCmd));
		}
		RHICommand* PopCommand();
		void ExecuteCommands();
		RHICommandContext* GetCmdContext() const { return mGraphicsContext; }
		RHICommandContext* GetComputeContext() const { return mComputeContext; }

		void SetShaderUniformBuffer(RHIShader* Shader, uint32 BaseIndex, RHIUniformBuffer* UniformBuffer)
		{

		}
		void BeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState);
		void EndRenderPass();
		void BindGfxPipeline(RHIPipeline* InPipeline);
		void DrawPrimitive(uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances);
		void BindDrawData(GRAPHICS_PIPELINE_STATE_DESC* InDrawData);
		bool HasCommand();

		void SetPriority(uint32 InPriority);
		uint32 GetPriority() const { return mPriority; }
		bool operator<(RHICommandList* InCmdList) const 
		{
			return mPriority < InCmdList->mPriority;
		}
	protected:
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
		RHICommandList* GetDefaultCommandList() const { return mDefaultCommandList; }

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
		RHIObjectPool<RHICommandList> mRHICmdListAllocation;
		RHICommandContext* mRHICommandCtx = nullptr;
		friend class RHICommandList;
	};

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

	RHI_COMMAND_DEFINE(BindGfxPipeline)
	{
		RHI_COMMAND_TYPE(BindGfxPipeline)(RHIPipeline * InGfxPipeline)
			: mGfxPipeline(InGfxPipeline)
		{
		}

		void Execute(RHICommandList * InCmdList);
		RHIPipeline* mGfxPipeline = nullptr;
	};

	RHI_COMMAND_DEFINE(BindDrawData)
	{
		RHI_COMMAND_TYPE(BindDrawData)(GRAPHICS_PIPELINE_STATE_DESC* InRenderData)
			: mRenderData(InRenderData)
		{
		}

		void Execute(RHICommandList* InCmdList);

		GRAPHICS_PIPELINE_STATE_DESC* mRenderData = nullptr;
	};

}