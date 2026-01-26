#include "ElainePrecompiledHeader.h"
#include "RenderGraph/ElaineRenderPass.h"
#include "render/common/ElaineRHICommandContext.h"
#include "render/common/ElaineRHICommandList.h"

namespace RenderGraph
{
	//=============================================================================
	// RGPass 实现
	//=============================================================================
	RGPass::RGPass(const std::string& InName, PassType InType)
		: mName(InName)
		, mPassType(InType)
	{
		// 根据 Pass 类型设置默认标志
		if (InType == PassType::AsyncCompute)
		{
			mFlags = mFlags | PassFlags::AsyncCompute;
		}
		else if (InType == PassType::Copy)
		{
			mFlags = mFlags | PassFlags::Copy;
		}
	}

	RGPass::~RGPass()
	{
	}

	void RGPass::Execute(Elaine::RHICommandList* CmdList)
	{
		if (mExecuteCallback)
		{
			mExecuteCallback(CmdList);
		}
	}

	void RGPass::AddTextureInput(RGTextureHandle Handle, ERGResourceAccess Access, Elaine::ERHIAccess State)
	{
		RGResourceUsage Usage;
		Usage.Handle = Handle;
		Usage.Access = Access;
		Usage.RHIState = State;
		mTextureInputs.push_back(Usage);
	}

	void RGPass::AddTextureOutput(RGTextureHandle Handle, Elaine::ERHIAccess State)
	{
		RGResourceUsage Usage;
		Usage.Handle = Handle;
		Usage.Access = ERGResourceAccess::Write;
		Usage.RHIState = State;
		mTextureOutputs.push_back(Usage);
	}

	void RGPass::AddBufferInput(RGBufferHandle Handle, ERGResourceAccess Access, Elaine::ERHIAccess State)
	{
		RGResourceUsage Usage;
		Usage.Handle = Handle;
		Usage.Access = Access;
		Usage.RHIState = State;
		mBufferInputs.push_back(Usage);
	}

	void RGPass::AddBufferOutput(RGBufferHandle Handle, Elaine::ERHIAccess State)
	{
		RGResourceUsage Usage;
		Usage.Handle = Handle;
		Usage.Access = ERGResourceAccess::Write;
		Usage.RHIState = State;
		mBufferOutputs.push_back(Usage);
	}

	void RGPass::SetRenderTarget(uint32 Index, RGTextureHandle Handle, const RGRenderTargetDesc& Desc)
	{
		if (Index >= mRenderTargets.size())
		{
			mRenderTargets.resize(Index + 1);
		}
		mRenderTargets[Index] = { Handle, Desc };

		// 自动添加为输出
		AddTextureOutput(Handle, Elaine::ERHIAccess::RTV);
	}

	void RGPass::SetDepthStencil(RGTextureHandle Handle, const RGDepthStencilDesc& Desc)
	{
		mDepthStencil = { Handle, Desc };
		mHasDepthStencil = true;

		// 根据是否只读决定访问状态
		if (Desc.ReadOnly)
		{
			AddTextureInput(Handle, ERGResourceAccess::Read, Elaine::ERHIAccess::DSVRead);
		}
		else
		{
			AddTextureOutput(Handle, Elaine::ERHIAccess::DSVWrite);
		}
	}

	ERGQueueType RGPass::GetQueueType() const
	{
		if (HasFlag(mFlags, PassFlags::AsyncCompute) || mPassType == PassType::AsyncCompute)
		{
			return ERGQueueType::Compute;
		}
		if (HasFlag(mFlags, PassFlags::Copy) || mPassType == PassType::Copy)
		{
			return ERGQueueType::Copy;
		}
		return ERGQueueType::Graphics;
	}
}