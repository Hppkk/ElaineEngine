#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"
#include "render/vulkan/ElaineVulkanInstance.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/common/ElaineRHICommandList.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanBuffer.h"
#include "render/vulkan/ElaineVulkanSwapChain.h"
#include "render/vulkan/ElaineVulkanViewport.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "render/vulkan/ElaineVulkanPipeline.h"
#include "render/vulkan/ElaineVulkanRenderPass.h"

namespace VulkanRHI
{

	VulkanCommandContext::VulkanCommandContext(VulkanDevice* InDevice, VulkanQueue* InQueue)
		: mDevice(InDevice)
		, mQueue(InQueue)
	{
		
	}

	VulkanCommandContext::~VulkanCommandContext()
	{
		
	}

	void VulkanCommandContext::Initilize()
	{
		mCmdBufferManager = new VulkanCommandBufferManager(mDevice, mQueue);
	}

	void VulkanCommandContext::Deinitilize()
	{

	}

	void VulkanCommandContext::RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
	}

	void VulkanCommandContext::RHIDispatchIndirectComputeShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)
	{
	}

	void VulkanCommandContext::RHISetMultipleViewports(uint32 Count, const ViewportBounds* Data)
	{
	}

	void VulkanCommandContext::RHICopyToResolveTarget(RHITexture* SourceTexture, RHITexture* DestTexture, const ResolveParams& ResolveParams)
	{
	}

	void VulkanCommandContext::RHIBeginRenderQuery(RHIRenderQuery* RenderQuery)
	{
	}

	void VulkanCommandContext::RHIEndRenderQuery(RHIRenderQuery* RenderQuery)
	{
	}

	void VulkanCommandContext::RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask)
	{
	}

	void VulkanCommandContext::RHIBeginDrawingViewport(RHIViewport* Viewport, RHITexture* RenderTargetRHI)
	{
	}

	void VulkanCommandContext::RHIEndDrawingViewport(RHIViewport* Viewport, bool bPresent, bool bLockToVsync)
	{
	}

	void VulkanCommandContext::RHIBeginFrame()
	{

	}

	void VulkanCommandContext::RHIEndFrame()
	{
		VulkanSwapChain* VKSwapChain = GetVulkanDynamicRHI()->GetViewport()->GetSwapChain();
		VkPresentInfoKHR PresentInfo;
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &VKSwapChain->GetSwapChain();
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores = &VKSwapChain->GetIndexVkSemaphore(VKSwapChain->GetCurrentImageIndex());
		vkQueuePresentKHR(mQueue->GetHandle(), &PresentInfo);
	}

	void VulkanCommandContext::RHIBeginScene()
	{
	}

	void VulkanCommandContext::RHIEndScene()
	{
	}

	void VulkanCommandContext::RHISetDepthBounds(float MinDepth, float MaxDepth)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdSetDepthBounds(CurrentCmdBuffer->GetHandle(), MinDepth, MaxDepth);
	}

	void VulkanCommandContext::RHIBeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState/*const RHIRenderPassInfo& InInfo, const TCHAR* InName*/)
	{
		VkRenderPassBeginInfo RenderPassBeginInfo;
		Memory::MemoryZero(RenderPassBeginInfo);
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
		RenderPassBeginInfo.framebuffer = GetVulkanDynamicRHI()->GetViewport()->
			GetIndexFrameBuffer(GetVulkanDynamicRHI()->GetViewport()->GetSwapChain()->GetCurrentImageIndex());
		RenderPassBeginInfo.renderArea = GetVulkanDynamicRHI()->GetViewport()->GetDefaultScissor();
		RenderPassBeginInfo.clearValueCount = 1;
		VkClearValue ClearVal = { 0.0f,0.0f,0.0f,1.0f };
		RenderPassBeginInfo.pClearValues = &ClearVal;
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdBeginRenderPass(CurrentCmdBuffer->GetHandle(), &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanCommandContext::RHIEndRenderPass()
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdEndRenderPass(CurrentCmdBuffer->GetHandle());
	}

	void VulkanCommandContext::RHINextSubpass()
	{
	}

	void VulkanCommandContext::RHICopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo)
	{
	}

	void VulkanCommandContext::RHICopyBufferRegion(RHIBuffer* DestBuffer, uint64 DstOffset, RHIBuffer* SourceBuffer, uint64 SrcOffset, uint64 NumBytes)
	{
	}

	void VulkanCommandContext::RHIBindGfxPipeline(RHIPipeline* InPipeline)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanGfxPipeline* VkGfxPipeline = static_cast<VulkanGfxPipeline*>(InPipeline);
		vkCmdBindPipeline(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, VkGfxPipeline->GetHandle());
	}

	RHIBuffer* VulkanCommandContext::RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData)
	{
		VulkanBuffer* NewBuffer = new VulkanBuffer(mDevice, InSize, Usage, ResourceState, Stride);
		return NewBuffer;
	}

	RHIBuffer* VulkanCommandContext::RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
		return RHICreateBuffer(Size, Usage, Stride, ResourceState, InData);
	}

	void VulkanCommandContext::RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* Contents)
	{
		if (UniformBufferRHI == nullptr)
			return;

		
	}

	RHIBuffer* VulkanCommandContext::RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
		return RHICreateBuffer(Size, Usage, 0, ResourceState, InData);
	}

	RHITexture* VulkanCommandContext::RHICreateTexture(const RHITextureDesc& InDesc)
	{
		//todo managing the life cycle
		RHITexture* Result = new VulkanTexture(mDevice, InDesc);
		return Result;
	}

	RHITexture* VulkanCommandContext::RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData)
	{
		return nullptr;
	}

	RHITexture* VulkanCommandContext::RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData)
	{
		return nullptr;
	}

	RHIPipeline* VulkanCommandContext::RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState)
	{
		return mDevice->GetPiplineManager()->GetOrCreateGfxPipeline(InPipelineState);
	}

	RHIPipeline* VulkanCommandContext::RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState)
	{
		return nullptr;
	}

	void VulkanCommandContext::RHIWriteGPUFence(RHIGPUFence* InFenceRHI)
	{
		VulkanCommandBuffer* CurrentCommandBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanFence* CurrentFence = static_cast<VulkanFence*>(InFenceRHI);
		//CurrentFence-
	}

	void VulkanCommandContext::RHIBeginUpdateMultiFrameResource(RHITexture* Texture)
	{
	}

	void VulkanCommandContext::RHIEndUpdateMultiFrameResource(RHITexture* Texture)
	{
	}

	void VulkanCommandContext::RHISetStreamSource(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset)
	{
	}

	void VulkanCommandContext::RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
	{
		if (mCurrentGfxPipeline)
		{

		}
	}

	void VulkanCommandContext::RHISetStereoViewport(float LeftMinX, float RightMinX, float LeftMinY, float RightMinY, float MinZ, float LeftMaxX, float RightMaxX, float LeftMaxY, float RightMaxY, float MaxZ)
	{

	}

	void VulkanCommandContext::RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{

	}

	void VulkanCommandContext::RHISetGraphicsPipelineState(RHIGraphicsPipelineState* GraphicsState, uint32 StencilRef, bool bApplyAdditionalState)
	{

	}

	void VulkanCommandContext::RHISetShaderTexture(RHIShader* Shader, uint32 TextureIndex, RHITexture* NewTexture)
	{
	}

	void VulkanCommandContext::RHISetPixelShaderTexture(RHIShader* PixelShader, uint32 TextureIndex, RHITexture* NewTexture)
	{
		VulkanTexture* CurrentTexture = static_cast<VulkanTexture*>(NewTexture);
		//CurrentTexture
	}

	void VulkanCommandContext::RHISetComputeShaderSampler(RHIShader* ComputeShader, uint32 SamplerIndex, RHISampler* NewState)
	{
	}

	void VulkanCommandContext::RHISetShaderSampler(RHIShader* ComputeShader, uint32 SamplerIndex, RHISampler* NewState)
	{
	}

	void VulkanCommandContext::RHISetShaderUniformBuffer(RHIShader* Shader, uint32 BufferIndex, RHIUniformBuffer* Buffer)
	{
	}

	void VulkanCommandContext::RHISetComputeShaderUniformBuffer(RHIShader* ComputeShader, uint32 BufferIndex, RHIUniformBuffer* Buffer)
	{
	}

	void VulkanCommandContext::RHISetShaderParameter(RHIShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
	{
	}

	void VulkanCommandContext::RHISetComputeShaderParameter(RHIShader* ComputeShader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue)
	{
	}

	void VulkanCommandContext::RHIBindDrawData(GRAPHICS_PIPELINE_STATE_DESC* InDrawData)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		//VulkanGfxPipeline* VkGfxPipeline = static_cast<VulkanGfxPipeline*>(InDrawData);
		for (int Index = 0; Index < STREAM_INPUT_MAX; ++Index)
		{
			if (InDrawData->mRHIDrawData->mStreamInput.mIStreamBuffer[Index] == nullptr)
				continue;
			VkBuffer pVKBuffer = static_cast<VulkanBuffer*>(InDrawData->mRHIDrawData->mStreamInput.mIStreamBuffer[Index])->GetHandle();
			if (Index == STREAM_INDEXBUFFER)
			{
				vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), pVKBuffer, 0, EngineToVkIndexType(InDrawData->mIndexType));
				continue;
			}
			
			vkCmdBindVertexBuffers(CurrentCmdBuffer->GetHandle(), Index, 1, &pVKBuffer, 0);
		}

		//todo
		//vkCmdBindDescriptorSets()
		//vkCmdSetScissor(CurrentCmdBuffer->GetHandle(),)

	}

	void VulkanCommandContext::RHIDrawPrimitive(uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		CurrentCmdBuffer->DrawPrimitive(NumPrimitives, NumInstances, BaseVertexIndex, 0);
	}

	void VulkanCommandContext::RHIDrawPrimitiveIndirect(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)
	{
	}

	void VulkanCommandContext::RHIDrawIndexedIndirect(RHIBuffer* IndexBufferRHI, RHIBuffer* ArgumentsBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances)
	{
	}

	void VulkanCommandContext::RHIDrawIndexedPrimitive(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances)
	{
		//VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		//CurrentCmdBuffer->DrawPrimitiveIndex()
	}

	void VulkanCommandContext::RHIDrawIndexedPrimitiveIndirect(RHIBuffer* IndexBuffer, RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)
	{
	}

	void VulkanCommandContext::RHIDispatchMeshShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
	{
	}

	void VulkanCommandContext::RHIDispatchIndirectMeshShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)
	{
	}
}