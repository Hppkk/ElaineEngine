#include "ElainePrecompiledHeader.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include "render/common/ElaineRHITypes.h"

namespace RenderGraph
{
	//=============================================================================
	// GraphResource 实现
	//=============================================================================
	GraphResource::GraphResource(const std::string& InName)
		: mName(InName)
	{
	}

	GraphResource::~GraphResource()
	{
		// RHI 资源的生命周期由资源池管理
		mResourceRHI = nullptr;
	}

	//=============================================================================
	// GraphTexture 实现
	//=============================================================================
	GraphTexture::GraphTexture(const std::string& InName, const RGTextureDesc& InDesc)
		: GraphResource(InName)
		, mDesc(InDesc)
	{
	}

	GraphTexture::~GraphTexture()
	{
	}

	RHITexture* GraphTexture::GetRHITexture() const
	{
		return static_cast<RHITexture*>(mResourceRHI);
	}

	void GraphTexture::SetRHITexture(RHITexture* InTexture)
	{
		mResourceRHI = InTexture;
	}

	//=============================================================================
	// GraphBuffer 实现
	//=============================================================================
	GraphBuffer::GraphBuffer(const std::string& InName, const RGBufferDesc& InDesc)
		: GraphResource(InName)
		, mDesc(InDesc)
	{
	}

	GraphBuffer::~GraphBuffer()
	{
	}

	RHIBuffer* GraphBuffer::GetRHIBuffer() const
	{
		return static_cast<RHIBuffer*>(mResourceRHI);
	}

	void GraphBuffer::SetRHIBuffer(RHIBuffer* InBuffer)
	{
		mResourceRHI = InBuffer;
	}
}