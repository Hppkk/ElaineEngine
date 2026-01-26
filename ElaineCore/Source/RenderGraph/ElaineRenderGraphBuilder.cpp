#include "ElainePrecompiledHeader.h"
#include "RenderGraph/ElaineRenderGraphBuilder.h"
#include "RenderGraph/ElaineRenderGraph.h"
#include "render/common/ElaineRHITypes.h"

namespace RenderGraph
{
	//=============================================================================
	// 构造/析构
	//=============================================================================
	RenderGraphBuilder::RenderGraphBuilder()
		: mGraph(nullptr)
		, mCurrentPass(nullptr)
	{
	}

	RenderGraphBuilder::RenderGraphBuilder(RenderDependencyGraph* Graph)
		: mGraph(Graph)
		, mCurrentPass(nullptr)
	{
	}

	RenderGraphBuilder::~RenderGraphBuilder()
	{
	}

	//=============================================================================
	// 资源创建
	//=============================================================================
	RGTextureHandle RenderGraphBuilder::CreateTexture(const std::string& Name, const RGTextureDesc& Desc)
	{
		if (!mGraph)
		{
			return RGTextureHandle{};
		}
		return mGraph->CreateTexture(Name, Desc);
	}

	RGBufferHandle RenderGraphBuilder::CreateBuffer(const std::string& Name, const RGBufferDesc& Desc)
	{
		if (!mGraph)
		{
			return RGBufferHandle{};
		}
		return mGraph->CreateBuffer(Name, Desc);
	}

	RGTextureHandle RenderGraphBuilder::CreatePersistentTexture(const std::string& Name, const RGTextureDesc& Desc)
	{
		if (!mGraph)
		{
			return RGTextureHandle{};
		}
		return mGraph->CreatePersistentTexture(Name, Desc);
	}

	RGBufferHandle RenderGraphBuilder::CreatePersistentBuffer(const std::string& Name, const RGBufferDesc& Desc)
	{
		if (!mGraph)
		{
			return RGBufferHandle{};
		}
		return mGraph->CreatePersistentBuffer(Name, Desc);
	}

	//=============================================================================
	// 外部资源导入
	//=============================================================================
	RGTextureHandle RenderGraphBuilder::ImportTexture(const std::string& Name, 
		Elaine::RHITexture* Texture, const RGTextureDesc& Desc)
	{
		if (!mGraph)
		{
			return RGTextureHandle{};
		}
		return mGraph->ImportTexture(Name, Texture, Desc);
	}

	RGBufferHandle RenderGraphBuilder::ImportBuffer(const std::string& Name, 
		Elaine::RHIBuffer* Buffer, const RGBufferDesc& Desc)
	{
		if (!mGraph)
		{
			return RGBufferHandle{};
		}
		return mGraph->ImportBuffer(Name, Buffer, Desc);
	}

	RGTextureHandle RenderGraphBuilder::ImportExtractedTexture(const std::string& Name)
	{
		if (!mGraph)
		{
			return RGTextureHandle{};
		}
		return mGraph->ImportExtractedTexture(Name);
	}

	bool RenderGraphBuilder::HasExtractedTexture(const std::string& Name) const
	{
		if (!mGraph)
		{
			return false;
		}
		return mGraph->HasExtractedTexture(Name);
	}

	void RenderGraphBuilder::QueueTextureExtraction(const std::string& Name, RGTextureHandle Handle)
	{
		if (mGraph)
		{
			mGraph->QueueTextureExtraction(Name, Handle);
		}
	}

	void RenderGraphBuilder::MarkForPresent(RGTextureHandle Handle)
	{
		if (mGraph)
		{
			mGraph->MarkForPresent(Handle);
		}
	}

	//=============================================================================
	// 资源访问声明
	//=============================================================================
	RGTextureHandle RenderGraphBuilder::ReadTexture(RGTextureHandle Handle, Elaine::ERHIAccess ReadAccess)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddTextureInput(Handle, ERGResourceAccess::Read, ReadAccess);

		// 更新资源生命周期
		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::Read);
		}

		return Handle;
	}

	RGTextureHandle RenderGraphBuilder::WriteTexture(RGTextureHandle Handle, Elaine::ERHIAccess WriteAccess)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddTextureOutput(Handle, WriteAccess);

		// 更新资源生命周期
		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::Write);
		}

		// 返回新版本的 Handle
		RGTextureHandle NewHandle = Handle;
		NewHandle.Version++;
		return NewHandle;
	}

	RGTextureHandle RenderGraphBuilder::ReadWriteTexture(RGTextureHandle Handle, Elaine::ERHIAccess Access)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddTextureInput(Handle, ERGResourceAccess::ReadWrite, Access);

		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::ReadWrite);
		}

		RGTextureHandle NewHandle = Handle;
		NewHandle.Version++;
		return NewHandle;
	}

	RGBufferHandle RenderGraphBuilder::ReadBuffer(RGBufferHandle Handle, Elaine::ERHIAccess ReadAccess)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddBufferInput(Handle, ERGResourceAccess::Read, ReadAccess);

		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::Read);
		}

		return Handle;
	}

	RGBufferHandle RenderGraphBuilder::WriteBuffer(RGBufferHandle Handle, Elaine::ERHIAccess WriteAccess)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddBufferOutput(Handle, WriteAccess);

		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::Write);
		}

		RGBufferHandle NewHandle = Handle;
		NewHandle.Version++;
		return NewHandle;
	}

	RGBufferHandle RenderGraphBuilder::ReadWriteBuffer(RGBufferHandle Handle, Elaine::ERHIAccess Access)
	{
		if (!mCurrentPass || !Handle.IsValid())
		{
			return Handle;
		}

		mCurrentPass->AddBufferInput(Handle, ERGResourceAccess::ReadWrite, Access);

		if (mGraph)
		{
			mGraph->UpdateResourceLifetime(Handle, mCurrentPass->GetIndex(), ERGResourceAccess::ReadWrite);
		}

		RGBufferHandle NewHandle = Handle;
		NewHandle.Version++;
		return NewHandle;
	}

	//=============================================================================
	// 资源别名
	//=============================================================================
	RGTextureHandle RenderGraphBuilder::CreateTextureAlias(RGTextureHandle SourceHandle,
		const std::string& AliasName, Elaine::PixelFormat ViewFormat,
		uint32 MipLevel, uint32 ArraySlice)
	{
		if (!mGraph || !SourceHandle.IsValid())
		{
			return RGTextureHandle{};
		}

		RGResourceAlias Alias;
		Alias.SourceHandle = SourceHandle;
		Alias.ViewFormat = ViewFormat;
		Alias.MipLevel = MipLevel;
		Alias.ArraySlice = ArraySlice;

		return mGraph->CreateTextureAlias(AliasName, Alias);
	}

	//=============================================================================
	// 渲染目标设置
	//=============================================================================
	void RenderGraphBuilder::SetRenderTarget(uint32 Index, RGTextureHandle Texture,
		const RGRenderTargetDesc& Desc)
	{
		if (mCurrentPass)
		{
			mCurrentPass->SetRenderTarget(Index, Texture, Desc);

			if (mGraph && Texture.IsValid())
			{
				mGraph->UpdateResourceLifetime(Texture, mCurrentPass->GetIndex(), ERGResourceAccess::Write);
			}
		}
	}

	void RenderGraphBuilder::SetDepthStencil(RGTextureHandle Texture, const RGDepthStencilDesc& Desc)
	{
		if (mCurrentPass)
		{
			mCurrentPass->SetDepthStencil(Texture, Desc);

			if (mGraph && Texture.IsValid())
			{
				ERGResourceAccess Access = Desc.ReadOnly ? ERGResourceAccess::Read : ERGResourceAccess::Write;
				mGraph->UpdateResourceLifetime(Texture, mCurrentPass->GetIndex(), Access);
			}
		}
	}

	const RGTextureDesc* RenderGraphBuilder::GetTextureDesc(RGTextureHandle Handle) const
	{
		if (!mGraph || !Handle.IsValid())
		{
			return nullptr;
		}
		return mGraph->GetTextureDesc(Handle);
	}

	const RGBufferDesc* RenderGraphBuilder::GetBufferDesc(RGBufferHandle Handle) const
	{
		if (!mGraph || !Handle.IsValid())
		{
			return nullptr;
		}
		return mGraph->GetBufferDesc(Handle);
	}

	bool RenderGraphBuilder::IsValidHandle(RGResourceHandle Handle) const
	{
		if (!mGraph)
		{
			return false;
		}
		return mGraph->IsValidHandle(Handle);
	}

	void RenderGraphBuilder::AddPassToGraph(std::unique_ptr<RGPass> Pass)
	{
		if (mGraph)
		{
			mGraph->AddPass(std::move(Pass));
		}
	}
}
