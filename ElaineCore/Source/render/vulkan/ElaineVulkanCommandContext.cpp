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
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanUniformBuffer.h"
#include "render/vulkan/ElaineVulkanDescriptorSet.h"
#include "render/vulkan/ElaineVulkanBarrier.h"

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
		mCmdBufferManager->Initilize();
		mDescriptorSetManager = new VulkanDescriptorSetManager(mDevice);
		for (int Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index)
		{
			mImageAvailableSemaphores[Index] = new VulkanSemaphore(mDevice);
			mRenderFinishedSemaphores[Index] = new VulkanSemaphore(mDevice);
		}
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
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		mCmdBufferManager->WaitForCmdBuffer(CurrentCmdBuffer, UINT64_MAX);


		VulkanSwapChain* VKSwapChain = GetVulkanDynamicRHI()->GetViewport()->GetSwapChain();
		bool AcquireSucceed = VKSwapChain->AcquireImage(mImageAvailableSemaphores[mCurrentFrameIndex], mCurrentImageIndex);
		if (!AcquireSucceed)
		{
			GetVulkanDynamicRHI()->GetViewport()->RecreateSwapchain();
			return;
		}
		
		VkRenderPassBeginInfo RenderPassBeginInfo;
		Memory::MemoryZero(RenderPassBeginInfo);
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
		RenderPassBeginInfo.framebuffer = GetVulkanDynamicRHI()->GetViewport()->
			GetIndexFrameBuffer(GetVulkanDynamicRHI()->GetViewport()->GetSwapChain()->GetCurrentImageIndex());
		RenderPassBeginInfo.renderArea = GetVulkanDynamicRHI()->GetViewport()->GetDefaultScissor();
		RenderPassBeginInfo.clearValueCount = 2;
		VkClearValue ClearVal1;
		ClearVal1.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue ClearVal2;
		ClearVal2.depthStencil = { 1.0f, 0 };
		std::vector<VkClearValue> TempClearVals = { ClearVal1, ClearVal2 };
		RenderPassBeginInfo.pClearValues = TempClearVals.data();

		
		vkCmdBeginRenderPass(CurrentCmdBuffer->GetHandle(), &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(CurrentCmdBuffer->GetHandle(), 0, 1, &GetVulkanDynamicRHI()->GetViewport()->GetDefaultViewPort());
		vkCmdSetScissor(CurrentCmdBuffer->GetHandle(), 0, 1, &GetVulkanDynamicRHI()->GetViewport()->GetDefaultScissor());
	}

	void VulkanCommandContext::RHIEndFrame()
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdEndRenderPass(CurrentCmdBuffer->GetHandle());
		CurrentCmdBuffer->End();
		CurrentCmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, mImageAvailableSemaphores[mCurrentFrameIndex]);
		mCmdBufferManager->SubmitActiveCmdBuffer(std::vector<VulkanSemaphore*>{mRenderFinishedSemaphores[mCurrentFrameIndex]});
		VulkanSwapChain* VKSwapChain = GetVulkanDynamicRHI()->GetViewport()->GetSwapChain();
		VkPresentInfoKHR PresentInfo;
		Memory::MemoryZero(PresentInfo);
		PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		PresentInfo.waitSemaphoreCount = 1;
		PresentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrameIndex]->GetHandle();
		PresentInfo.swapchainCount = 1;
		PresentInfo.pSwapchains = &VKSwapChain->GetSwapChain();
		PresentInfo.pImageIndices = &mCurrentImageIndex;
		vkQueuePresentKHR(mQueue->GetHandle(), &PresentInfo);
		mCurrentFrameIndex = (mCurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

		mCmdBufferManager->RefreshCommandBufferState();
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
		mCurrentGfxPipeline = VkGfxPipeline;
		vkCmdBindPipeline(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, VkGfxPipeline->GetHandle());
	}

	RHIBuffer* VulkanCommandContext::RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData)
	{
		VulkanBuffer* NewBuffer = new VulkanBuffer(mDevice, InSize, Usage, ResourceState, Stride);
		NewBuffer->MapMemoryDataToBuffer(InData, InSize);
		return NewBuffer;
	}

	RHIBuffer* VulkanCommandContext::RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
		return RHICreateBuffer(Size, Usage, Stride, ResourceState, InData);
	}

	void VulkanCommandContext::RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* InContents)
	{
		if (UniformBufferRHI == nullptr)
			return;

		VulkanUniformBuffer* VkUniformBuffer = static_cast<VulkanUniformBuffer*>(UniformBufferRHI);
		VkUniformBuffer->UpdateBuffer(InContents);
	}

	RHIBuffer* VulkanCommandContext::RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
		return RHICreateBuffer(Size, Usage, 0, ResourceState, InData);
	}

	RHITexture* VulkanCommandContext::RHICreateTexture(const RHITextureDesc& InDesc, void* InContent)
	{
		//todo managing the life cycle
		VulkanTexture* Result = new VulkanTexture(mDevice, InDesc);
		size_t ImageLayerCount = 1;
		if (InDesc.mDimension == TextureDimension::Texture2DArray)
		{
			ImageLayerCount = InDesc.mArraySize;
		}
		else if (InDesc.mDimension == TextureDimension::TextureCube)
		{
			ImageLayerCount = 6;
		}
		else if (InDesc.mDimension == TextureDimension::TextureCubeArray)
		{
			ImageLayerCount = InDesc.mArraySize * 6;
		}
		size_t LayerSize = InDesc.mExtent.x * InDesc.mExtent.y * GPixelFormats[InDesc.mFormat].BlockBytes;
		size_t AllocSize = LayerSize * ImageLayerCount;
		VulkanStagingBuffer* VkStagingBuffer = mDevice->GetStagingManager()->AcquireBuffer(AllocSize);
		Memory::MemoryCopy(VkStagingBuffer->GetMappingPointer(), InContent, AllocSize);
		VulkanCommandBuffer* PUploadCommandBuffer = mCmdBufferManager->GetUploadCmdBuffer();
		
		//todo
		//VkBufferImageCopy Regions[6] = { };
		//for (uint32_t i = 0; i < 6; ++i)
		{
			VkImageSubresourceRange SubRange = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6);
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubRange);
			VkBufferImageCopy Region;
			Memory::MemoryZero(Region);
			Region.bufferOffset = VkStagingBuffer->GetBufferOffset();// +i * LayerSize;
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = 0;
			Region.imageSubresource.baseArrayLayer = 0;  // 当前面索引
			Region.imageSubresource.layerCount = 6;
			//Region.imageOffset = {}
			Region.imageExtent = { static_cast<uint32_t>(InDesc.mExtent.x), static_cast<uint32_t>(InDesc.mExtent.y), 1 };
			vkCmdCopyBufferToImage(PUploadCommandBuffer->GetHandle(), VkStagingBuffer->GetHandle(), Result->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SubRange);
		}
		
		
		mDevice->GetStagingManager()->ReleaseBuffer(PUploadCommandBuffer, VkStagingBuffer);
		mCmdBufferManager->SubmitUploadCmdBuffer();
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; 
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		vkCreateSampler(mDevice->GetDevice(), &samplerInfo, nullptr, &skyboxSampler);
		return Result;
	}

	RHITexture* VulkanCommandContext::RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData)
	{
		RHITextureDesc TexDescRHI;
		TexDescRHI.mExtent.x = SizeX;
		TexDescRHI.mExtent.y = SizeY;
		TexDescRHI.mFormat = (PixelFormat)Format;
		TexDescRHI.mNumMips = NumMips;
		TexDescRHI.mNumSamples = NumSamples;
		TexDescRHI.mFlags = Flags;
		VulkanTexture* Result = new VulkanTexture(mDevice, TexDescRHI);
		size_t AllocSize = SizeX * SizeY * GPixelFormats[(PixelFormat)Format].BlockBytes;
		VulkanStagingBuffer* VkStagingBuffer = mDevice->GetStagingManager()->AcquireBuffer(AllocSize);
		//Result->
		return Result;
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

	RHIUniformBuffer* VulkanCommandContext::RHICreateUniformBuffer(size_t InSize, void* InContents)
	{
		RHIUniformBuffer* NewUniformBuffer = new  VulkanUniformBuffer(mDevice, InSize, WritableMask, InContents);
		return NewUniformBuffer;
	}

	void VulkanCommandContext::RHIUpdateCommonUniformBuffer(size_t InSize, void* InContents)
	{
		if (mCommonUniformBuffer[mCurrentFrameIndex] == nullptr)
		{
			mCommonUniformBuffer[mCurrentFrameIndex] = (VulkanUniformBuffer*)RHICreateUniformBuffer(InSize, InContents);
			mDescriptorSetManager->WriteUniformBufferToDescriptorSet(mCommonUniformBuffer[mCurrentFrameIndex], mCommonDescriptorSets[mCurrentFrameIndex]);
		}

		mCommonUniformBuffer[mCurrentFrameIndex]->UpdateBuffer(InContents);
	}

	void VulkanCommandContext::RHIWriteGPUFence(RHIGPUFence* InFenceRHI)
	{
		VulkanCommandBuffer* CurrentCommandBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanFence* CurrentFence = static_cast<VulkanFence*>(InFenceRHI);
		//CurrentFence-
	}

	void VulkanCommandContext::SetCommonDescriptorSets(VulkanDescriptorSet* InDescroptorSet, size_t InIndex)
	{
		mCommonDescriptorSets[InIndex] = InDescroptorSet;
		mIsCreateCommonDescriptorSets = true;
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
			VulkanBuffer* VkVertexBuffer = static_cast<VulkanBuffer*>(InDrawData->mRHIDrawData->mStreamInput.mIStreamBuffer[Index]);
			VkBuffer pVKBuffer = VkVertexBuffer->GetHandle();
			if (Index == STREAM_INDEXBUFFER)
			{
				vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), pVKBuffer, 0, EngineToVkIndexType(InDrawData->mIndexType));
				continue;
			}
			size_t VertexOffset = VkVertexBuffer->GetOffset();
			vkCmdBindVertexBuffers(CurrentCmdBuffer->GetHandle(), Index, 1, &pVKBuffer, &VertexOffset);
		}

		//todo
		std::vector<VkDescriptorSet> DescriptorSets;
		size_t BindCount = 1;
		static bool isWrite = false;
		DescriptorSets.push_back(mCommonDescriptorSets[mCurrentFrameIndex]->GetHandle());
		for (auto&& CurrentDescriptorSet : mCurrentGfxPipeline->GetDescriptorSets())
		{
			if (CurrentDescriptorSet != nullptr)
			{
				DescriptorSets.push_back(CurrentDescriptorSet->GetHandle());
				if (CurrentDescriptorSet->GetSet() == 2 && !isWrite)
				{
					VulkanTexture* VkTexture = static_cast<VulkanTexture*>(InDrawData->mTextures[0]);
					mDescriptorSetManager->WriteImageToDescriptorSet(VkTexture, skyboxSampler, CurrentDescriptorSet);
					isWrite = true;
				}
				++BindCount;
			}
			else
			{
				DescriptorSets.push_back(nullptr);
			}

		}

		vkCmdBindDescriptorSets(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, 
			mCurrentGfxPipeline->GetLayout().GetPipelineLayoutHandle(), 0, BindCount,
			DescriptorSets.data(), 0, RHI_NULL_HANDLE);
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