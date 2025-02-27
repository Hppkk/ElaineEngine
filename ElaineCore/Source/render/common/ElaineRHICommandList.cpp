#include "ElainePrecompiledHeader.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/common/ElaineRHICommandContext.h"

namespace Elaine
{
	RHICommandList::RHICommandList()
	{
		
	}

	RHICommandList::~RHICommandList()
	{

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
		if (!HasCommand())
			return;
		RHICommand* CurrentCommand = mCommandLinkBegin;
		while (CurrentCommand != nullptr)
		{
			CurrentCommand->Execute(this);
			CurrentCommand = CurrentCommand->mNext;
		}
		mCommandLinkBegin = nullptr;
		mCommandLinkEnd = nullptr;
		mCommandNum = 0;
	}

	void RHICommandList::BeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BeginRenderPass))(InGfxState);
	}

	void RHICommandList::EndRenderPass()
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(EndRenderPass))();
		{
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

	void RHICommandList::BindGfxPipeline(RHIPipeline* InPipeline)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BindGfxPipeline))(InPipeline);
	}

	void RHICommandList::DrawPrimitive(uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(DrawPrimitive))(InBaseVertexIndex, InNumPrimitives, InNumInstances);
	}

	void RHICommandList::BindDrawData(GRAPHICS_PIPELINE_STATE_DESC* InDrawData)
	{
		ALLOC_COMMAND(RHI_COMMAND_TYPE(BindDrawData))(InDrawData);
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
		: mRHICommandCtx(InCtx)
	{
		//mDefaultCommandList = CreateCommandList();
	}

	RHICommandListManager::~RHICommandListManager()
	{
		for (auto&& CmdList : mCmdLists)
		{
			DestroyCommandList(CmdList);
		}
		mCmdLists.clear();
		mDefaultCommandList = nullptr;
	}

	RHICommandList* RHICommandListManager::CreateCommandList()
	{
		//todo freelist

		RHICommandList* NewCommandList = new RHICommandList(); //mRHICmdListAllocation.CreateObject();
		NewCommandList->mListIndex = mCmdLists.size();
		mCmdLists.push_back(NewCommandList);
		mLogicCmdList.push_back(NewCommandList);
		NewCommandList->mGraphicsContext = mRHICommandCtx;
		NewCommandList->mOwner = this;
		return NewCommandList;
	}

	void RHICommandListManager::DestroyCommandList(RHICommandList* InCmdList)
	{
		//todo
		if (InCmdList->mCommandNum > 0u)
		{
			mWaitDetroyCmdLists.push_back(InCmdList);
			mCmdLists.erase(mCmdLists.begin() + InCmdList->mListIndex);
			InCmdList->mListIndex = 0u;
			return;
		}
		SAFE_DELETE(InCmdList);
		//mRHICmdListAllocation.ReleaseObject(InCmdList);
	}

	void RHICommandListManager::SwapCommands()
	{
		std::swap(mLogicCmdList, mUploadCmdLists);
	}

	void RHICommandListManager::ExecuteCommands()
	{
		for (auto&& CurrCommandList : mUploadCmdLists)
		{
			CurrCommandList->ExecuteCommands();
			mFreeCmdList.push_back(CurrCommandList);
		}
		mUploadCmdLists.clear();
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

	void RHI_COMMAND_TYPE(BindGfxPipeline)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBindGfxPipeline(mGfxPipeline);
	}

	void RHI_COMMAND_TYPE(BindDrawData)::Execute(RHICommandList* InCmdList)
	{
		InCmdList->GetCmdContext()->RHIBindDrawData(mRenderData);
	}
}