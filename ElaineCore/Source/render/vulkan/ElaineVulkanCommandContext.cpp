#include "ElainePrecompiledHeader.h"
#include "common/ElaineRHICommandList.h"
#include "vulkan/ElaineVulkanQueue.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanTexture.h"
#include "vulkan/ElaineVulkanBuffer.h"
#include "vulkan/ElaineVulkanMemory.h"
#include "vulkan/ElaineVulkanBarrier.h"
#include "vulkan/ElaineVulkanViewport.h"
#include "vulkan/ElaineVulkanInstance.h"
#include "vulkan/ElaineVulkanPipeline.h"
#include "vulkan/ElaineVulkanSwapChain.h"
#include "vulkan/ElaineVulkanRenderPass.h"
#include "vulkan/ElaineVulkanSamplerCache.h"
#include "vulkan/ElaineVulkanDescriptorSet.h"
#include "vulkan/ElaineVulkanUniformBuffer.h"
#include "vulkan/ElaineVulkanCommandContext.h"
#include "vulkan/ElaineVulkanPhysicalDevice.h"
#ifdef WIN32
#include "vulkan_win32.h"
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID 
#include "vulkan_android.h"
#endif

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

	void VulkanCommandContext::Initialize()
	{
		mCmdBufferManager = new VulkanCommandBufferManager(mDevice, mQueue);
		mCmdBufferManager->Initialize();
		mDescriptorSetManager = new VulkanDescriptorSetManager(mDevice);
		for (int Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index)
		{
			mImageAvailableSemaphores[Index] = new VulkanSemaphore(mDevice);
			mRenderFinishedSemaphores[Index] = new VulkanSemaphore(mDevice);
		}

		mDefaultViewport = new VulkanViewport(mDevice, 1920, 1080);

		mSamplerCache = new VulkanSamplerCache(mDevice);
		mSamplerCache->Initialize();

		mRenderPassCache = new VulkanRenderPassCache(mDevice);
		mFramebufferCache = new VulkanFramebufferCache(mDevice);
		mFrameDescriptorAllocator = new VulkanFrameDescriptorAllocator(mDevice, MAX_FRAMES_IN_FLIGHT, mDescriptorSetManager);
	}

	void VulkanCommandContext::Deinitilize()
	{
		SAFE_DELETE(mCmdBufferManager);
		SAFE_DELETE(mDescriptorSetManager);
		SAFE_DELETE(mDefaultViewport);
		if (mSamplerCache)
		{
			delete mSamplerCache;
			mSamplerCache = nullptr;
		}

		SAFE_DELETE(mRenderPassCache);
		SAFE_DELETE(mFramebufferCache);
		SAFE_DELETE(mFrameDescriptorAllocator);

		for (int Index = 0; Index < MAX_FRAMES_IN_FLIGHT; ++Index)
		{
			SAFE_DELETE(mImageAvailableSemaphores[Index]);
			SAFE_DELETE(mRenderFinishedSemaphores[Index]);
		}
	}

	void VulkanCommandContext::RHIWaitIdle()
	{
		vkQueueWaitIdle(mQueue->GetHandle());
	}

	void VulkanCommandContext::RHIWindowResize(RHISwapchain* InSwapchain, uint32 InWidth, uint32 InHeight)
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
		std::vector<VkViewport> Viewports;
		Viewports.resize(Count);
		for (int i = 0; i < Count; ++i)
		{
			Viewports[i].x = Data[i].TopLeftX;
			Viewports[i].y = Data[i].TopLeftY;
			Viewports[i].width = Data[i].Width;
			Viewports[i].height = Data[i].Height;
			Viewports[i].minDepth = Data[i].MinDepth;
			Viewports[i].maxDepth = Data[i].MaxDepth;
		}

		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdSetViewport(CurrentCmdBuffer->GetHandle(), 0, Count, Viewports.data());
	}

	void VulkanCommandContext::RHISetSwapchain(RHISwapchain* InSwapchain)
	{
		mSwapchain = static_cast<VulkanSwapChain*>(InSwapchain);
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

	RHITexture* VulkanCommandContext::RHIAcquireSwapchainImage(RHISwapchain* InSwapchain)
	{
		if (!InSwapchain)
		{
			return nullptr;
		}

		VulkanSwapChain* VKSwapchain = static_cast<VulkanSwapChain*>(InSwapchain);
		bool AcquireSucceed = VKSwapchain->AcquireImage(mImageAvailableSemaphores[mCurrentFrameIndex], mCurrentImageIndex);

		if (AcquireSucceed)
		{
			return VKSwapchain->AcquireNextTexture();
		}

		return nullptr;
	}

	void VulkanCommandContext::RHIBindResourceBinding(const RHI_DRAW_RESOURCE_BINDING& InResourceBinding)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();

		// ========== 1. Bind Vertex Buffer ==========
		for (int Index = 0; Index < STREAM_INPUT_MAX; ++Index)
		{
			if (InResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[Index] == nullptr)
				continue;

			VulkanBuffer* VkVertexBuffer = static_cast<VulkanBuffer*>(
				InResourceBinding.mDrawData.mStreamInput.mIStreamBuffer[Index]);
			VkBuffer pVKBuffer = VkVertexBuffer->GetHandle();
			size_t VertexOffset = VkVertexBuffer->GetOffset();
			vkCmdBindVertexBuffers(CurrentCmdBuffer->GetHandle(), Index, 1, &pVKBuffer, &VertexOffset);
		}
		
		// ========== 2. Bind Index Buffer ==========
		if (InResourceBinding.mIndexBuffer != nullptr)
		{
			VulkanBuffer* VkIndexBuffer = static_cast<VulkanBuffer*>(InResourceBinding.mIndexBuffer);
			vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), VkIndexBuffer->GetHandle(),
				InResourceBinding.mIndexOffset, EngineToVkIndexType(InResourceBinding.mIndexType));
		}
		// ========== 3. Update Image Descriptor Set ==========
		for (uint32 i = 0; i < InResourceBinding.mTextureCount; ++i)
		{
			if (InResourceBinding.mTextures[i] != nullptr)
			{
				VulkanTexture* VkTexture = static_cast<VulkanTexture*>(InResourceBinding.mTextures[i]);
				VkSampler Sampler = mSamplerCache->GetSampler(InResourceBinding.mSamplerTypes[i]);

				// 获取对应纹理槽位的描述符集 (假设Set=2用于纹理)
				auto& DescriptorSets = mCurrentGfxPipeline->GetDescriptorSets();
				bool bFound = false;
				for (auto* CurrentDescriptorSet : DescriptorSets)
				{
					if (CurrentDescriptorSet && CurrentDescriptorSet->GetSet() == 2)
					{
						// 如果是静态资源（IsStaticResourceBinding），则直接更新
						// WriteImageToDescriptorSet 内部已添加去重检测，所以静态资源其实只会在第一次真写
						mDescriptorSetManager->WriteImageToDescriptorSet(VkTexture, Sampler, CurrentDescriptorSet);
						bFound = true;
						break;
					}
				}
				
				// 如果没有预分配的 Set 或者需要完全动态分配 (预留扩展)
				if (!bFound && mFrameDescriptorAllocator)
				{
					// 示例：动态分配 Set=2
					// VkDescriptorSetLayout Layout = ...; 
					// VulkanDescriptorSet* DynamicSet = mFrameDescriptorAllocator->AllocateForFrame(mCurrentFrameIndex, Layout, 2);
					// mDescriptorSetManager->WriteImageToDescriptorSet(VkTexture, Sampler, DynamicSet);
				}
			}
		}
		// ========== 4. Update Uniform Buffer Descriptor Set ==========
		// 4.1 Common Uniform (Set=0)
		if (InResourceBinding.mCommonUniformBuffer != nullptr)
		{
			VulkanUniformBuffer* VkCommonUB = static_cast<VulkanUniformBuffer*>(InResourceBinding.mCommonUniformBuffer);
			mDescriptorSetManager->WriteUniformBufferToDescriptorSet(VkCommonUB, mCommonDescriptorSet);
		}

		// 4.2 VS Uniform (Set=1)
		for (uint32 i = 0; i < RHI_MAX_VS_BUFFER_COUNT; ++i)
		{
			if (InResourceBinding.mVSUniformBuffers[i] != nullptr)
			{
				VulkanUniformBuffer* VkVSUB = static_cast<VulkanUniformBuffer*>(InResourceBinding.mVSUniformBuffers[i]);
				// TODO: 获取VS对应的描述符集并写入
			}
		}

		// 4.3 PS Uniform
		for (uint32 i = 0; i < RHI_MAX_PS_BUFFER_COUNT; ++i)
		{
			if (InResourceBinding.mPSUniformBuffers[i] != nullptr)
			{
				VulkanUniformBuffer* VkPSUB = static_cast<VulkanUniformBuffer*>(InResourceBinding.mPSUniformBuffers[i]);
				// TODO: 获取PS对应的描述符集并写入
			}
		}
		// ========== 5. Bind Descriptor Sets ==========
		std::vector<VkDescriptorSet> DescriptorSets;
		size_t BindCount = 1;
		DescriptorSets.push_back(mCommonDescriptorSet->GetHandle());

		for (auto* CurrentDescriptorSet : mCurrentGfxPipeline->GetDescriptorSets())
		{
			if (CurrentDescriptorSet != nullptr)
			{
				DescriptorSets.push_back(CurrentDescriptorSet->GetHandle());
				++BindCount;
			}
		}

		vkCmdBindDescriptorSets(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
			mCurrentGfxPipeline->GetLayout().GetPipelineLayoutHandle(), 0, BindCount,
			DescriptorSets.data(), 0, RHI_NULL_HANDLE);
	}

	void VulkanCommandContext::RHIPresentSwapchain(RHISwapchain* InSwapchain, bool bVsync)
	{
		if (!InSwapchain)
		{
			return;
		}

		VulkanSwapChain* VKSwapchain = static_cast<VulkanSwapChain*>(InSwapchain);
		VKSwapchain->Present(mQueue, mRenderFinishedSemaphores[mCurrentFrameIndex]);

		mCurrentFrameIndex = (mCurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
		mCmdBufferManager->RefreshCommandBufferState();
	}

	void VulkanCommandContext::RHIBeginFrame()
	{
		mFrameDescriptorAllocator->ResetFrame(mCurrentFrameIndex);
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		mCmdBufferManager->WaitForCmdBuffer(CurrentCmdBuffer, UINT64_MAX);

		//bool AcquireSucceed = mSwapchain->AcquireImage(mImageAvailableSemaphores[mCurrentFrameIndex], mCurrentImageIndex);
		//if (!AcquireSucceed)
		//{
		//	return;
		//}
		//
		//VkRenderPassBeginInfo RenderPassBeginInfo;
		//Memory::MemoryZero(RenderPassBeginInfo);
		//RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//RenderPassBeginInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
		//RenderPassBeginInfo.framebuffer = mSwapchain->GetIndexFrameBuffer(mCurrentImageIndex);
		//RenderPassBeginInfo.renderArea = mDefaultViewport->GetDefaultScissor();
		//RenderPassBeginInfo.clearValueCount = 2;
		//VkClearValue ClearVal1 { };
		//ClearVal1.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		//VkClearValue ClearVal2 { };
		//ClearVal2.depthStencil = { 1.0f, 0 };
		//std::vector<VkClearValue> TempClearVals = { ClearVal1, ClearVal2 };
		//RenderPassBeginInfo.pClearValues = TempClearVals.data();
		//
		//vkCmdBeginRenderPass(CurrentCmdBuffer->GetHandle(), &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		//vkCmdSetViewport(CurrentCmdBuffer->GetHandle(), 0, 1, &mDefaultViewport->GetDefaultViewPort());
		//vkCmdSetScissor(CurrentCmdBuffer->GetHandle(), 0, 1, &mDefaultViewport->GetDefaultScissor());
	}

	void VulkanCommandContext::RHIEndFrame()
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		//vkCmdEndRenderPass(CurrentCmdBuffer->GetHandle());
		CurrentCmdBuffer->End();
		CurrentCmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, mImageAvailableSemaphores[mCurrentFrameIndex]);
		mCmdBufferManager->SubmitActiveCmdBuffer(std::vector<VulkanSemaphore*>{mRenderFinishedSemaphores[mCurrentFrameIndex]});
		//VulkanSwapChain* VKSwapChain = mDefaultViewport->GetSwapChain();
		//VkPresentInfoKHR PresentInfo;
		//Memory::MemoryZero(PresentInfo);
		//PresentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		//PresentInfo.waitSemaphoreCount = 1;
		//PresentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrameIndex]->GetHandle();
		//PresentInfo.swapchainCount = 1;
		//PresentInfo.pSwapchains = &mSwapchain->GetSwapChain();
		//PresentInfo.pImageIndices = &mCurrentImageIndex;
		//vkQueuePresentKHR(mQueue->GetHandle(), &PresentInfo);
		//mCurrentFrameIndex = (mCurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

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
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		
		// 获取当前 Swapchain 的 backbuffer 和 depth 纹理
		VulkanTexture* BackBufferTexture = static_cast<VulkanTexture*>(mSwapchain->AcquireNextTexture());
		VulkanTexture* DepthTexture = static_cast<VulkanTexture*>(mSwapchain->GetDepthStencilTexture());

		// Resolve Proxy for legacy path (if used)
		if (BackBufferTexture && BackBufferTexture->IsProxy())
		{
			VulkanTexture* RealBackBuffer = BackBufferTexture->GetOwningSwapchain()->GetActiveBackBufferTexture();
			if (RealBackBuffer) BackBufferTexture = RealBackBuffer;
		}
		if (DepthTexture && DepthTexture->IsProxy())
		{
			VulkanTexture* RealDepth = DepthTexture->GetOwningSwapchain()->GetActiveBackBufferTexture();
			if (RealDepth) DepthTexture = RealDepth;
		}
		
		if (!BackBufferTexture)
		{
			return;
		}
		
		// 使用 FramebufferCache 获取或创建 Framebuffer
		FramebufferKey FbKey;
		FbKey.RenderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
		FbKey.Attachments[FbKey.NumAttachments++] = BackBufferTexture->GetTextureView().mView;
		if (DepthTexture)
		{
			FbKey.Attachments[FbKey.NumAttachments++] = DepthTexture->GetTextureView().mView;
		}
		FbKey.Width = BackBufferTexture->GetWidth();
		FbKey.Height = BackBufferTexture->GetHeight();
		FbKey.Layers = 1;
		
		VkFramebuffer CachedFramebuffer = mFramebufferCache->GetOrCreateFramebuffer(FbKey);
		
		// 构建 RenderPass Begin Info
		VkRenderPassBeginInfo RenderPassBeginInfo;
		Memory::MemoryZero(RenderPassBeginInfo);
		RenderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RenderPassBeginInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();
		RenderPassBeginInfo.framebuffer = CachedFramebuffer;
		RenderPassBeginInfo.renderArea = mDefaultViewport->GetDefaultScissor();
		
		std::vector<VkClearValue> ClearValues;
		VkClearValue ColorClear = { 0.0f, 0.0f, 0.0f, 1.0f };
		ClearValues.push_back(ColorClear);
		if (DepthTexture)
		{
			VkClearValue DepthClear = {};
			DepthClear.depthStencil = { 1.0f, 0 };
			ClearValues.push_back(DepthClear);
		}
		RenderPassBeginInfo.clearValueCount = static_cast<uint32>(ClearValues.size());
		RenderPassBeginInfo.pClearValues = ClearValues.data();
		
		vkCmdBeginRenderPass(CurrentCmdBuffer->GetHandle(), &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	}

	void VulkanRHI::VulkanCommandContext::RHIBeginRenderPass(const RHIRenderPassInfo& InRenderPassInfo, const char * InName)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();

		VkRenderPass CachedRenderPass = mRenderPassCache->GetOrCreateRenderPass(InRenderPassInfo);

		FramebufferKey FbKey;
		FbKey.RenderPass = CachedRenderPass;
		uint32 Width = 0, Height = 0;

		for (uint32 i = 0; i < MaxSimultaneousRenderTargets; ++i)
		{
			const auto& ColorRT = InRenderPassInfo.ColorRenderTargets[i];
			if (ColorRT.RenderTarget == nullptr) break;

			VulkanTexture* VkTexture = static_cast<VulkanTexture*>(ColorRT.RenderTarget);
			
			// Resolve Proxy
			if (VkTexture->IsProxy())
			{
				VulkanTexture* RealTex = VkTexture->GetOwningSwapchain()->GetActiveBackBufferTexture();
				if (RealTex)
				{
					VkTexture = RealTex;
				}
			}
			
			if (Width == 0) Width = VkTexture->GetWidth();
			if (Height == 0) Height = VkTexture->GetHeight();
			
			FbKey.Attachments[FbKey.NumAttachments++] = VkTexture->GetTextureView().mView;
		}

		if (InRenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget != nullptr)
		{
			VulkanTexture* VkDepthTexture = static_cast<VulkanTexture*>(InRenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget);
			
			// Resolve Proxy
			if (VkDepthTexture->IsProxy())
			{
				VulkanTexture* RealDepth = VkDepthTexture->GetOwningSwapchain()->GetActiveBackBufferTexture();
				if (RealDepth)
				{
					VkDepthTexture = RealDepth;
				}
			}

			if (Width == 0) Width = VkDepthTexture->GetWidth();
			if (Height == 0) Height = VkDepthTexture->GetHeight();
			
			FbKey.Attachments[FbKey.NumAttachments++] = VkDepthTexture->GetTextureView().mView;
		}

		FbKey.Width = Width;
		FbKey.Height = Height;
		FbKey.Layers = 1;

		VkFramebuffer CachedFramebuffer = mFramebufferCache->GetOrCreateFramebuffer(FbKey);

		std::vector<VkClearValue> ClearValues;
		for (uint32 i = 0; i < MaxSimultaneousRenderTargets; ++i)
		{
			if (InRenderPassInfo.ColorRenderTargets[i].RenderTarget == nullptr) break;
			VkClearValue Clear = {};
			const auto& ClearColor = InRenderPassInfo.ColorRenderTargets[i].ClearColor;
			Clear.color = { ClearColor.R, ClearColor.G, ClearColor.B, ClearColor.A };
			ClearValues.push_back(Clear);
		}
		if (InRenderPassInfo.DepthStencilRenderTarget.DepthStencilTarget != nullptr)
		{
			VkClearValue Clear = {};
			Clear.depthStencil = { InRenderPassInfo.DepthStencilRenderTarget.ClearDepth, InRenderPassInfo.DepthStencilRenderTarget.ClearStencil };
			ClearValues.push_back(Clear);
		}

		VkRenderPassBeginInfo BeginInfo = {};
		BeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		BeginInfo.renderPass = CachedRenderPass;
		BeginInfo.framebuffer = CachedFramebuffer;
		BeginInfo.renderArea.offset = { 0, 0 };
		BeginInfo.renderArea.extent = { Width, Height };
		BeginInfo.clearValueCount = static_cast<uint32>(ClearValues.size());
		BeginInfo.pClearValues = ClearValues.data();

		vkCmdBeginRenderPass(CurrentCmdBuffer->GetHandle(), &BeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		mCurrentRenderPass = CachedRenderPass;
		mCurrentRenderPassKey = VulkanRenderPassCache::BuildRenderPassKey(InRenderPassInfo);
	}

	void VulkanCommandContext::RHIEndRenderPass()
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdEndRenderPass(CurrentCmdBuffer->GetHandle());

		mCurrentRenderPass = VK_NULL_HANDLE;
		mCurrentRenderPassKey = RenderPassKey();
	}

	void VulkanCommandContext::RHINextSubpass()
	{
	}

	void VulkanCommandContext::RHICopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo)
	{
		VulkanTexture* VkSource = static_cast<VulkanTexture*>(SourceTexture);
		VulkanTexture* VkDest = static_cast<VulkanTexture*>(DestTexture);

		// Resolve Proxies
		if (VkSource->IsProxy())
		{
			VulkanTexture* RealSource = VkSource->GetOwningSwapchain()->GetActiveBackBufferTexture();
			if (RealSource) VkSource = RealSource;
		}
		if (VkDest->IsProxy())
		{
			VulkanTexture* RealDest = VkDest->GetOwningSwapchain()->GetActiveBackBufferTexture();
			if (RealDest) VkDest = RealDest;
		}

		VulkanCommandBuffer* CmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();

		std::vector<VkImageCopy> Regions;
		Regions.reserve(CopyInfo.NumMips);

		Vector3 CurrentSize = CopyInfo.Size;
		if (CurrentSize == Vector3::ZERO)
		{
			CurrentSize = Vector3(VkSource->GetWidth(), VkSource->GetHeight(), VkSource->GetDesc().mDepth);
			// Adjust for SourceMipIndex if Size was implicit full-resource
			for (uint32 i = 0; i < CopyInfo.SourceMipIndex; ++i)
			{
				CurrentSize.x = Math::max(1.0f, std::floor(CurrentSize.x / 2.0f));
				CurrentSize.y = Math::max(1.0f, std::floor(CurrentSize.y / 2.0f));
				CurrentSize.z = Math::max(1.0f, std::floor(CurrentSize.z / 2.0f));
			}
		}

		for (uint32 MipIndex = 0; MipIndex < CopyInfo.NumMips; ++MipIndex)
		{
			VkImageCopy Region;
			Memory::MemoryZero(Region);

			Region.srcSubresource.aspectMask = VkSource->GetFullAspectMask();
			Region.srcSubresource.mipLevel = CopyInfo.SourceMipIndex + MipIndex;
			Region.srcSubresource.baseArrayLayer = CopyInfo.SourceSliceIndex;
			Region.srcSubresource.layerCount = CopyInfo.NumSlices;
			Region.srcOffset = { (int32)CopyInfo.SourcePosition.x, (int32)CopyInfo.SourcePosition.y, (int32)CopyInfo.SourcePosition.z };

			Region.dstSubresource.aspectMask = VkDest->GetFullAspectMask();
			Region.dstSubresource.mipLevel = CopyInfo.DestMipIndex + MipIndex;
			Region.dstSubresource.baseArrayLayer = CopyInfo.DestSliceIndex;
			Region.dstSubresource.layerCount = CopyInfo.NumSlices;
			Region.dstOffset = { (int32)CopyInfo.DestPosition.x, (int32)CopyInfo.DestPosition.y, (int32)CopyInfo.DestPosition.z };

			Region.extent = { (uint32)CurrentSize.x, (uint32)CurrentSize.y, (uint32)CurrentSize.z };

			Regions.push_back(Region);

			// Downscale for next mip
			CurrentSize.x = Math::max(1.0f, std::floor(CurrentSize.x / 2.0f));
			CurrentSize.y = Math::max(1.0f, std::floor(CurrentSize.y / 2.0f));
			CurrentSize.z = Math::max(1.0f, std::floor(CurrentSize.z / 2.0f));
		}

		VkImageLayout OldSrcLayout = VkSource->GetImageLayout();
		VkImageLayout OldDstLayout = VkDest->GetImageLayout();
		bool IsSrcChange = false;
		bool IsDstChange = false;

		if (OldSrcLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			VkImageSubresourceRange SrcSubRange = VulkanPipelineBarrier::MakeSubresourceRange(VkSource->GetFullAspectMask(), CopyInfo.SourceMipIndex, CopyInfo.NumMips, CopyInfo.SourceSliceIndex, CopyInfo.NumSlices);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), VkSource, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SrcSubRange);
			IsSrcChange = true;
		}

		if (OldDstLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			VkImageSubresourceRange DstSubRange = VulkanPipelineBarrier::MakeSubresourceRange(VkDest->GetFullAspectMask(), CopyInfo.DestMipIndex, CopyInfo.NumMips, CopyInfo.DestSliceIndex, CopyInfo.NumSlices);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), VkDest, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, DstSubRange);
			IsDstChange = true;
		}

		vkCmdCopyImage(CmdBuffer->GetHandle(),
			VkSource->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VkDest->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32>(Regions.size()), Regions.data());

		if (IsSrcChange)
		{
			VkImageSubresourceRange SrcSubRange = VulkanPipelineBarrier::MakeSubresourceRange(VkSource->GetFullAspectMask(), CopyInfo.SourceMipIndex, CopyInfo.NumMips, CopyInfo.SourceSliceIndex, CopyInfo.NumSlices);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), VkSource, OldSrcLayout, SrcSubRange);
		}

		if (IsDstChange)
		{
			VkImageSubresourceRange DstSubRange = VulkanPipelineBarrier::MakeSubresourceRange(VkDest->GetFullAspectMask(), CopyInfo.DestMipIndex, CopyInfo.NumMips, CopyInfo.DestSliceIndex, CopyInfo.NumSlices);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), VkDest, OldDstLayout, DstSubRange);
		}

		//mCmdBufferManager->SubmitUploadCmdBuffer();
	}

	void VulkanCommandContext::RHICopyBufferRegion(RHIBuffer* DestBuffer, uint64 DstOffset, RHIBuffer* SourceBuffer, uint64 SrcOffset, uint64 NumBytes)
	{
	}

	void VulkanCommandContext::RHIResourceBarrier(const RHIResourceBarrierDesc& Barrier)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();

		if (Barrier.Texture != nullptr)
		{
			VulkanTexture* VkTexture = static_cast<VulkanTexture*>(Barrier.Texture);
			VulkanTexture* ProxyTexture = nullptr;

			// Resolve Proxy
			if (VkTexture->IsProxy())
			{
				ProxyTexture = VkTexture;
				VulkanTexture* RealTex = VkTexture->GetOwningSwapchain()->GetActiveBackBufferTexture();
				if (RealTex)
				{
					VkTexture = RealTex;
				}
			}
			
			VkImageLayout OldLayout = VkTexture->GetImageLayout();
			VkImageLayout NewLayout = ERHIAccessToVkImageLayout(Barrier.StateAfter);

			// 如果布局相同，只需要内存屏障
			if (OldLayout == NewLayout && OldLayout != VK_IMAGE_LAYOUT_UNDEFINED)
			{
				// 只需要访问同步，不需要布局转换
				VkMemoryBarrier MemoryBarrier = {};
				MemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				MemoryBarrier.srcAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateBefore);
				MemoryBarrier.dstAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateAfter);

				vkCmdPipelineBarrier(
					CurrentCmdBuffer->GetHandle(),
					ERHIAccessToVkPipelineStageFlags(Barrier.StateBefore),
					ERHIAccessToVkPipelineStageFlags(Barrier.StateAfter),
					0,
					1, &MemoryBarrier,
					0, nullptr,
					0, nullptr);
			}
			else
			{
				VkImageMemoryBarrier ImageBarrier = {};
				ImageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				ImageBarrier.srcAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateBefore);
				ImageBarrier.dstAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateAfter);
				ImageBarrier.oldLayout = OldLayout;
				ImageBarrier.newLayout = NewLayout;
				ImageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				ImageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				ImageBarrier.image = VkTexture->getHandle();
				
				ImageBarrier.subresourceRange.aspectMask = VkTexture->GetFullAspectMask();
				ImageBarrier.subresourceRange.baseMipLevel = 0;
				ImageBarrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				ImageBarrier.subresourceRange.baseArrayLayer = 0;
				ImageBarrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

				vkCmdPipelineBarrier(
					CurrentCmdBuffer->GetHandle(),
					ERHIAccessToVkPipelineStageFlags(Barrier.StateBefore),
					ERHIAccessToVkPipelineStageFlags(Barrier.StateAfter),
					0,
					0, nullptr,
					0, nullptr,
					1, &ImageBarrier);
			}
			VkTexture->SetAccess(Barrier.StateAfter);
			VkTexture->SetImageLayout(NewLayout);
			
			// Update Proxy state as well to keep logical state consistent
			if (ProxyTexture)
			{
				ProxyTexture->SetAccess(Barrier.StateAfter);
				ProxyTexture->SetImageLayout(NewLayout);
			}
		}
		else if (Barrier.Buffer != nullptr)
		{
			VulkanBuffer* VkBuffer = static_cast<VulkanBuffer*>(Barrier.Buffer);

			VkBufferMemoryBarrier BufferBarrier = {};
			BufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			BufferBarrier.srcAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateBefore);
			BufferBarrier.dstAccessMask = ERHIAccessToVkAccessFlags(Barrier.StateAfter);
			BufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			BufferBarrier.buffer = VkBuffer->GetHandle();
			BufferBarrier.offset = 0;
			BufferBarrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(
				CurrentCmdBuffer->GetHandle(),
				ERHIAccessToVkPipelineStageFlags(Barrier.StateBefore),
				ERHIAccessToVkPipelineStageFlags(Barrier.StateAfter),
				0,
				0, nullptr,
				1, &BufferBarrier,
				0, nullptr);
		}
	}

	void VulkanCommandContext::RHIBindGfxPipeline(RHIPipeline* InPipeline)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanGfxPipeline* VkGfxPipeline = static_cast<VulkanGfxPipeline*>(InPipeline);
		
		// 如果当前有活动的 RenderPass，检查是否需要使用 Pipeline 变体
		if (mCurrentRenderPassKey.NumColorTargets > 0 || mCurrentRenderPassKey.DepthFormat != VK_FORMAT_UNDEFINED)
		{
			// 从 Pipeline State 获取或创建与当前 RenderPass 兼容的变体
			VulkanGfxPipeline* CompatiblePipeline = mDevice->GetPiplineManager()->GetOrCreateGfxPipelineForRenderPass(
				VkGfxPipeline->GetPipelineState(), mCurrentRenderPassKey);
			
			if (CompatiblePipeline)
			{
				VkGfxPipeline = CompatiblePipeline;
			}
		}
		
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

	void VulkanCommandContext::RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* InContents, size_t InSize)
	{
		if (UniformBufferRHI == nullptr)
			return;

		VulkanUniformBuffer* VkUniformBuffer = static_cast<VulkanUniformBuffer*>(UniformBufferRHI);
		mDescriptorSetManager->WriteUniformBufferToDescriptorSet(VkUniformBuffer, mCurrentGfxPipeline->GetDescriptorSet((int32)VkUniformBuffer->GetSlot()));
		VkUniformBuffer->UpdateBuffer(InContents, 0, InSize);
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
			Region.imageSubresource.layerCount = ImageLayerCount;
			//Region.imageOffset = {}
			Region.imageExtent = { static_cast<uint32_t>(InDesc.mExtent.x), static_cast<uint32_t>(InDesc.mExtent.y), 1 };
			vkCmdCopyBufferToImage(PUploadCommandBuffer->GetHandle(), VkStagingBuffer->GetHandle(), Result->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SubRange);
			Result->SetAccess(ERHIAccess::SRVGraphics);
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
		TexDescRHI.mDimension = TextureDimension::Texture2D;
		TexDescRHI.mArraySize = 1;

		VulkanTexture* Result = new VulkanTexture(mDevice, TexDescRHI);

		// 如果有初始数据，使用 staging buffer 上传
		if (InData != nullptr)
		{
			size_t AllocSize = SizeX * SizeY * GPixelFormats[(PixelFormat)Format].BlockBytes;
			VulkanStagingBuffer* VkStagingBuffer = mDevice->GetStagingManager()->AcquireBuffer(AllocSize);
			Memory::MemoryCopy(VkStagingBuffer->GetMappingPointer(), InData, AllocSize);

			VulkanCommandBuffer* PUploadCommandBuffer = mCmdBufferManager->GetUploadCmdBuffer();

			// 转换到传输目标布局
			VkImageSubresourceRange SubRange = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, NumMips, 0, 1);
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubRange);

			// 拷贝 buffer 到 image
			VkBufferImageCopy Region;
			Memory::MemoryZero(Region);
			Region.bufferOffset = VkStagingBuffer->GetBufferOffset();
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = 0;
			Region.imageSubresource.baseArrayLayer = 0;
			Region.imageSubresource.layerCount = 1;
			Region.imageExtent = { SizeX, SizeY, 1 };

			vkCmdCopyBufferToImage(PUploadCommandBuffer->GetHandle(), VkStagingBuffer->GetHandle(), 
				Result->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

			// 转换到 shader 读取布局
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SubRange);

			mDevice->GetStagingManager()->ReleaseBuffer(PUploadCommandBuffer, VkStagingBuffer);
			mCmdBufferManager->SubmitUploadCmdBuffer();
		}

		return Result;
	}

	RHITexture* VulkanCommandContext::RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData)
	{
		RHITextureDesc TexDescRHI;
		TexDescRHI.mExtent.x = SizeX;
		TexDescRHI.mExtent.y = SizeY;
		TexDescRHI.mFormat = (PixelFormat)Format;
		TexDescRHI.mNumMips = NumMips;
		TexDescRHI.mNumSamples = 1;
		TexDescRHI.mFlags = Flags;
		TexDescRHI.mDimension = TextureDimension::Texture3D;
		TexDescRHI.mArraySize = 1;

		VulkanTexture* Result = new VulkanTexture(mDevice, TexDescRHI);

		// 如果有初始数据，使用 staging buffer 上传
		if (InData != nullptr)
		{
			size_t AllocSize = SizeX * SizeY * SizeZ * GPixelFormats[(PixelFormat)Format].BlockBytes;
			VulkanStagingBuffer* VkStagingBuffer = mDevice->GetStagingManager()->AcquireBuffer(AllocSize);
			Memory::MemoryCopy(VkStagingBuffer->GetMappingPointer(), InData, AllocSize);

			VulkanCommandBuffer* PUploadCommandBuffer = mCmdBufferManager->GetUploadCmdBuffer();

			// 转换到传输目标布局
			VkImageSubresourceRange SubRange = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, NumMips, 0, 1);
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, SubRange);

			// 拷贝 buffer 到 3D image
			VkBufferImageCopy Region;
			Memory::MemoryZero(Region);
			Region.bufferOffset = VkStagingBuffer->GetBufferOffset();
			Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			Region.imageSubresource.mipLevel = 0;
			Region.imageSubresource.baseArrayLayer = 0;
			Region.imageSubresource.layerCount = 1;
			Region.imageExtent = { SizeX, SizeY, SizeZ };

			vkCmdCopyBufferToImage(PUploadCommandBuffer->GetHandle(), VkStagingBuffer->GetHandle(),
				Result->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);

			// 转换到 shader 读取布局
			VulkanSetImageLayout(PUploadCommandBuffer->GetHandle(), Result, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, SubRange);

			mDevice->GetStagingManager()->ReleaseBuffer(PUploadCommandBuffer, VkStagingBuffer);
			mCmdBufferManager->SubmitUploadCmdBuffer();
		}

		return Result;
	}

	RHIPipeline* VulkanCommandContext::RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState)
	{
		return mDevice->GetPiplineManager()->GetOrCreateGfxPipeline(InPipelineState);
	}

	RHIPipeline* VulkanCommandContext::RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState)
	{
		return nullptr;
	}

	RHISwapchain* VulkanCommandContext::RHICreateSwapchain(uint32 InWidth, uint32 InHeight, bool InbIsFullscreen, PixelFormat InFormat)
	{
		return new VulkanSwapChain(mDevice, InWidth, InHeight, InbIsFullscreen, nullptr, InFormat);
	}

	void VulkanCommandContext::RHICreateSurface(void* InNativeHandle)
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		mWindowHandle = InNativeHandle;
		VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo;
		Elaine::Memory::MemoryZero(SurfaceCreateInfo);
		SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		SurfaceCreateInfo.hinstance = GetModuleHandle(NULL);
		SurfaceCreateInfo.hwnd = (HWND)mWindowHandle;
		vkCreateWin32SurfaceKHR(mInstance->GetInstance(), &SurfaceCreateInfo, VULKAN_CPU_ALLOCATOR, &mSurface);
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID
		//todo
#endif
	}

	RHIUniformBuffer* VulkanCommandContext::RHICreateUniformBuffer(size_t InSize, void* InContents)
	{
		RHIUniformBuffer* NewUniformBuffer = new  VulkanUniformBuffer(mDevice, InSize, WritableMask, InContents);
		return NewUniformBuffer;
	}

	void VulkanCommandContext::RHIUpdateCommonUniformBuffer(RHIUniformBuffer* InUniformBufferRHI, size_t InSize, void* InContents)
	{
		if (InUniformBufferRHI == nullptr)
			return;

		//if (InUniformBufferRHI == nullptr)
		
			//mCommonUniformBuffer[mCurrentFrameIndex] = (VulkanUniformBuffer*)RHICreateUniformBuffer(InSize, InContents);
			VulkanUniformBuffer* VKUniformBuf = static_cast<VulkanUniformBuffer*>(InUniformBufferRHI);
			mDescriptorSetManager->WriteUniformBufferToDescriptorSet(VKUniformBuf, mCommonDescriptorSet);
		

		VKUniformBuf->UpdateBuffer(InContents);
	}

	//==========================================================================
	// Slot-based UniformBuffer API
	//==========================================================================

	RHIUniformBuffer* VulkanCommandContext::RHICreateUniformBufferWithSlot(const RHIUniformBufferDesc& InDesc)
	{
		VulkanUniformBuffer* buffer = new VulkanUniformBuffer(
			mDevice,
			InDesc.Slot,
			InDesc.Size,
			ERHIAccess::VertexOrIndexBuffer,
			const_cast<void*>(InDesc.InitialData));
		
		return buffer;
	}

	void VulkanCommandContext::RHIBindUniformBuffer(RHIUniformSlot InSlot, RHIUniformBuffer* InBuffer)
	{
		if (InSlot == RHIUniformSlot::Invalid || InSlot >= RHIUniformSlot::Count)
		{
			LOG_ERROR("VulkanRHI: Invalid uniform slot for binding.");
			return;
		}
		
		mBoundUniformBuffers[(size_t)InSlot] = InBuffer;
		
		// 注意：实际的 DescriptorSet 更新在 Draw 时根据当前 Pipeline 的布局来完成
		// 这里只记录绑定状态，延迟绑定策略
	}


	void VulkanCommandContext::RHIWriteGPUFence(RHIGPUFence* InFenceRHI)
	{
		VulkanCommandBuffer* CurrentCommandBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanFence* CurrentFence = static_cast<VulkanFence*>(InFenceRHI);
		//CurrentFence-
	}

	void VulkanCommandContext::SetCommonDescriptorSets(VulkanDescriptorSet* InDescroptorSet, size_t InIndex)
	{
		mCommonDescriptorSet = InDescroptorSet;
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
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		if (CurrentCmdBuffer && VertexBuffer)
		{
			VulkanBuffer* VkBuf = static_cast<VulkanBuffer*>(VertexBuffer);
			VkBuffer BufferHandle = VkBuf->GetHandle();
			VkDeviceSize OffsetVal = static_cast<VkDeviceSize>(Offset);
			vkCmdBindVertexBuffers(CurrentCmdBuffer->GetHandle(), StreamIndex, 1, &BufferHandle, &OffsetVal);
		}
	}

	void VulkanCommandContext::RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ)
	{
		mDefaultViewport->SetSize(MinX, MinY, MinZ, MaxX, MaxY, MaxZ);
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		vkCmdSetViewport(CurrentCmdBuffer->GetHandle(), 0, 1, &mDefaultViewport->GetDefaultViewPort());
	}

	void VulkanCommandContext::RHISetStereoViewport(float LeftMinX, float RightMinX, float LeftMinY, float RightMinY, float MinZ, float LeftMaxX, float RightMaxX, float LeftMaxY, float RightMaxY, float MaxZ)
	{

	}

	void VulkanCommandContext::RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		if (CurrentCmdBuffer)
		{
			VkRect2D Scissor;
			if (bEnable)
			{
				Scissor.offset = { (int32_t)MinX, (int32_t)MinY };
				Scissor.extent = { MaxX - MinX, MaxY - MinY };
			}
			else
			{
				// 禁用裁剪时使用完整 viewport
				Scissor.offset = { 0, 0 };
				Scissor.extent = { 0xFFFF, 0xFFFF };
			}
			vkCmdSetScissor(CurrentCmdBuffer->GetHandle(), 0, 1, &Scissor);
		}
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
		//VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		////VulkanGfxPipeline* VkGfxPipeline = static_cast<VulkanGfxPipeline*>(InDrawData);
		//for (int Index = 0; Index < STREAM_INPUT_MAX; ++Index)
		//{
		//	if (InDrawData->mRHIDrawData->mStreamInput.mIStreamBuffer[Index] == nullptr)
		//		continue;
		//	VulkanBuffer* VkVertexBuffer = static_cast<VulkanBuffer*>(InDrawData->mRHIDrawData->mStreamInput.mIStreamBuffer[Index]);
		//	VkBuffer pVKBuffer = VkVertexBuffer->GetHandle();
		//	if (Index == STREAM_INDEXBUFFER)
		//	{
		//		vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), pVKBuffer, 0, EngineToVkIndexType(InDrawData->mIndexType));
		//		continue;
		//	}
		//	size_t VertexOffset = VkVertexBuffer->GetOffset();
		//	vkCmdBindVertexBuffers(CurrentCmdBuffer->GetHandle(), Index, 1, &pVKBuffer, &VertexOffset);
		//}

		////todo
		//std::vector<VkDescriptorSet> DescriptorSets;
		//size_t BindCount = 1;
		//static bool isWrite = false;
		//DescriptorSets.push_back(mCommonDescriptorSets[mCurrentFrameIndex]->GetHandle());
		//for (auto&& CurrentDescriptorSet : mCurrentGfxPipeline->GetDescriptorSets())
		//{
		//	if (CurrentDescriptorSet != nullptr)
		//	{
		//		DescriptorSets.push_back(CurrentDescriptorSet->GetHandle());
		//		if (CurrentDescriptorSet->GetSet() == 2 && !isWrite)
		//		{
		//			VulkanTexture* VkTexture = static_cast<VulkanTexture*>(InDrawData->mTextures[0]);
		//			mDescriptorSetManager->WriteImageToDescriptorSet(VkTexture, skyboxSampler, CurrentDescriptorSet);
		//			isWrite = true;
		//		}
		//		++BindCount;
		//	}
		//	else
		//	{
		//		DescriptorSets.push_back(nullptr);
		//	}

		//}

		//vkCmdBindDescriptorSets(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, 
		//	mCurrentGfxPipeline->GetLayout().GetPipelineLayoutHandle(), 0, BindCount,
		//	DescriptorSets.data(), 0, RHI_NULL_HANDLE);
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
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		if (CurrentCmdBuffer && IndexBuffer)
		{
			// 绑定 Index Buffer
			VulkanBuffer* VkIndexBuffer = static_cast<VulkanBuffer*>(IndexBuffer);
			vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), VkIndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
			
			// 绘制
			uint32 IndexCount = NumPrimitives * 3; // 假设三角形
			vkCmdDrawIndexed(CurrentCmdBuffer->GetHandle(), IndexCount, NumInstances, StartIndex, BaseVertexIndex, FirstInstance);
		}
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