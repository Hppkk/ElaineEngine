#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "render/common/ElaineRHICommandContext.h"
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
#include "common/ElaineRHI.h"
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

	VulkanCommandContext::~VulkanCommandContext() { Deinitilize(); }

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
	}

	void VulkanCommandContext::Deinitilize()
	{
        mUniformPages.clear();

        mDrawSets.clear(); mDrawPools.clear();
		SAFE_DELETE(mCmdBufferManager);
        Elaine::FlushRHIResourceReleases();
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
        vkDeviceWaitIdle(mDevice->GetDevice());
        mCmdBufferManager->RefreshCommandBufferState();
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
            mHasAcquiredSwapchain = true;
			return VKSwapchain->AcquireNextTexture();
		}

		return nullptr;
	}

    void VulkanCommandContext::RHIBindResourceBinding(const RHI_DRAW_RESOURCE_BINDING& Binding)
    {
        CheckRHIContextThread();
        auto* Cmd = mCmdBufferManager->GetActiveCmdBuffer();
        for (uint32 i = 0; i < STREAM_INPUT_MAX; ++i)
            if (Binding.mDrawData.mStreamInput.mIStreamBuffer[i])
            {
                auto* Buffer = static_cast<VulkanBuffer*>(Binding.mDrawData.mStreamInput.mIStreamBuffer[i].get());
                VkBuffer Handle = Buffer->GetHandle(); VkDeviceSize Offset = Buffer->GetOffset();
                vkCmdBindVertexBuffers(Cmd->GetHandle(), i, 1, &Handle, &Offset);
            }
        if (Binding.mIndexBuffer)
        {
            auto* Buffer = static_cast<VulkanBuffer*>(Binding.mIndexBuffer.get());
            vkCmdBindIndexBuffer(Cmd->GetHandle(), Buffer->GetHandle(), Buffer->GetOffset() + Binding.mIndexOffset, EngineToVkIndexType(Binding.mIndexType));
        }
        const auto& Layouts = mCurrentGfxPipeline->GetDescriptorSetLayouts();
        const auto& Limits = mDevice->GetPhyDevice()->GetDeviceProperties().limits;
        std::vector<VkDescriptorSet> Handles;
        for (uint32 Index = 0; Index < Layouts.size(); ++Index)
        {
            if (mDrawPools.empty() || !mDrawPools.back()->CanAllocate())
                mDrawPools.push_back(std::make_unique<VulkanDescriptorPool>(mDevice, 64));
            auto Set = std::make_unique<VulkanDescriptorSet>(Index);
            VkDescriptorSetAllocateInfo Info{}; Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            Info.descriptorSetCount = 1; Info.pSetLayouts = &Layouts[Index];
            if (!mDrawPools.back()->AllocateDescriptorSets(Info, &Set->GetHandle()))
            {
                mDrawPools.push_back(std::make_unique<VulkanDescriptorPool>(mDevice, 64));
                if (!mDrawPools.back()->AllocateDescriptorSets(Info, &Set->GetHandle())) throw std::runtime_error("Draw descriptor allocation failed");
            }
            const auto& Bindings = mCurrentGfxPipeline->GetDescriptorBindings();
            const auto Declarations = Bindings.find(Index);
            if (Declarations != Bindings.end()) for (const auto& Declaration : Declarations->second)
            {
                const auto& Group = Binding.Parameters[Index];
                const auto& Name = Declaration.name;
                VkDescriptorBufferInfo BufferInfo{};
                if (Declaration.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                {
                    auto It = Group.Uniforms.find(Name);
                    if (It == Group.Uniforms.end() || !It->second) throw std::runtime_error("Required uniform block is not bound: " + Name);
                    const auto& View = It->second;
                    auto* Buffer = static_cast<VulkanUniformBuffer*>(View.Buffer.get());
                    if (View.Offset % Limits.minUniformBufferOffsetAlignment || View.Size < Declaration.blockSize ||
                        View.Size > Limits.maxUniformBufferRange || View.Offset > Buffer->GetBufferSize() || View.Size > Buffer->GetBufferSize() - View.Offset)
                        throw std::runtime_error("Invalid uniform view: " + Name);
                    BufferInfo = {Buffer->GetHandle(), Buffer->GetOffset() + View.Offset, View.Size};
                    Cmd->Keep(View.Buffer);
                }
                else if (Declaration.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
                {
                    auto It = Group.Buffers.find(Name);
                    if (It == Group.Buffers.end() || !It->second.Buffer) throw std::runtime_error("Required read-only buffer is not bound: " + Name);
                    const auto& View = It->second;
                    const auto& Desc = View.Buffer->GetDesc();
                    const uint64 Size = View.Size ? View.Size : (View.Offset <= Desc.Size ? Desc.Size - View.Offset : 0);
                    const uint32 ShaderStride = Declaration.ResourceLayout.ElementStride;
                    if (!EnumHasAnyFlags(Desc.Usage, BufferUsageFlags::StructuredBuffer) || !Size || Size < Declaration.blockSize ||
                        View.Offset > Desc.Size || Size > Desc.Size - View.Offset || Size > Limits.maxStorageBufferRange ||
                        (ShaderStride && ((Size - Declaration.blockSize) % ShaderStride || (View.Stride && View.Stride != ShaderStride) || (Desc.Stride && Desc.Stride != ShaderStride))) ||
                        View.Offset % Limits.minStorageBufferOffsetAlignment || (View.Stride && (Size % View.Stride || (Desc.Stride && View.Stride != Desc.Stride))))
                        throw std::runtime_error("Invalid read-only buffer view: " + Name);
                    auto* Buffer = static_cast<VulkanBuffer*>(View.Buffer.get());
                    BufferInfo = {Buffer->GetHandle(), Buffer->GetOffset() + View.Offset, Size};
                    Cmd->Keep(View.Buffer);
                }
                else if (Declaration.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
                {
                    auto It = Group.Textures.find(Name);
                    if (It == Group.Textures.end() || !It->second.Texture) throw std::runtime_error("Required sampled texture is not bound: " + Name);
                    auto* Texture = static_cast<VulkanTexture*>(It->second.Texture.get());
                    const auto& Desc = Texture->GetDesc();
                    const auto& Layout = Declaration.ResourceLayout;
                    const auto Format = Texture->GetVkFormat();
                    bool IntegerFormat = false, SignedFormat = false;
                    switch (Format)
                    {
                    case VK_FORMAT_R8_SINT: case VK_FORMAT_R8G8_SINT: case VK_FORMAT_R8G8B8_SINT: case VK_FORMAT_R8G8B8A8_SINT:
                    case VK_FORMAT_R16_SINT: case VK_FORMAT_R16G16_SINT: case VK_FORMAT_R16G16B16_SINT: case VK_FORMAT_R16G16B16A16_SINT:
                    case VK_FORMAT_R32_SINT: case VK_FORMAT_R32G32_SINT: case VK_FORMAT_R32G32B32_SINT: case VK_FORMAT_R32G32B32A32_SINT:
                        IntegerFormat = SignedFormat = true; break;
                    case VK_FORMAT_R8_UINT: case VK_FORMAT_R8G8_UINT: case VK_FORMAT_R8G8B8_UINT: case VK_FORMAT_R8G8B8A8_UINT:
                    case VK_FORMAT_R16_UINT: case VK_FORMAT_R16G16_UINT: case VK_FORMAT_R16G16B16_UINT: case VK_FORMAT_R16G16B16A16_UINT:
                    case VK_FORMAT_R32_UINT: case VK_FORMAT_R32G32_UINT: case VK_FORMAT_R32G32B32_UINT: case VK_FORMAT_R32G32B32A32_UINT:
                    case VK_FORMAT_A2B10G10R10_UINT_PACK32: case VK_FORMAT_A2R10G10B10_UINT_PACK32:
                        IntegerFormat = true; break;
                    default: break;
                    }
                    if (Desc.mDimension != Layout.Dimension || (Desc.mNumSamples > 1) != Layout.Multisampled || IntegerFormat != Layout.IntegerSample ||
                        (IntegerFormat && SignedFormat != Layout.SignedSample) ||
                        !EnumHasAnyFlags(Desc.mFlags, TextureCreateFlags::ShaderResource) || (Layout.DepthSample && !(Texture->GetFullAspectMask() & VK_IMAGE_ASPECT_DEPTH_BIT)))
                        throw std::runtime_error("Sampled texture type mismatch: " + Name);
                    Cmd->Keep(It->second.Texture);
                    const auto Sampler = IntegerFormat ? SAMPLER_NEAREST_CLAMP : (Layout.DepthSample ? SAMPLER_SHADOW :
                        (Layout.Dimension == TextureDimension::TextureCube && It->second.Sampler == SAMPLER_DEFAULT ? SAMPLER_CUBEMAP : It->second.Sampler));
                    mDescriptorSetManager->WriteImageToDescriptorSet(Texture, mSamplerCache->GetSampler(Sampler), Set.get(), Declaration.binding);
                    continue;
                }
                else throw std::runtime_error("Unsupported draw descriptor type: " + Name);
                VkWriteDescriptorSet Write{}; Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                Write.dstSet = Set->GetHandle(); Write.dstBinding = Declaration.binding;
                Write.descriptorCount = 1; Write.descriptorType = Declaration.type; Write.pBufferInfo = &BufferInfo;
                vkUpdateDescriptorSets(mDevice->GetDevice(), 1, &Write, 0, nullptr);
            }
            Handles.push_back(Set->GetHandle()); mDrawSets.push_back(std::move(Set));
        }
        if (!Handles.empty()) vkCmdBindDescriptorSets(Cmd->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            mCurrentGfxPipeline->GetLayout().GetPipelineLayoutHandle(), 0, static_cast<uint32>(Handles.size()), Handles.data(), 0, nullptr);
    }

	void VulkanCommandContext::RHIPresentSwapchain(RHISwapchain* InSwapchain, bool bVsync)
	{
		if (!InSwapchain)
		{
			return;
		}

		VulkanSwapChain* VKSwapchain = static_cast<VulkanSwapChain*>(InSwapchain);
		VKSwapchain->Present(mQueue, mRenderFinishedSemaphores[mPresentFrameIndex]);
		mCmdBufferManager->RefreshCommandBufferState();
	}

	void VulkanCommandContext::RHIBeginFrame()
	{
        CheckRHIContextThread();
        auto& Previous = mFrameSubmissions[mCurrentFrameIndex];
        if (Previous.Buffer)
        {
            Previous.Buffer->RefreshFenceStatus();
            if (Previous.Buffer->GetCompletedFenceCounter() <= Previous.Counter)
                mCmdBufferManager->WaitForCmdBuffer(Previous.Buffer, UINT64_MAX);
        }
        mHasAcquiredSwapchain = false;
        mDrawPools.clear(); mDrawSets.clear();

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
		if (mCurrentRenderPass) RHIEndRenderPass();
        if (CurrentCmdBuffer->IsInsideRenderPass()) CurrentCmdBuffer->EndRenderPass();
        CurrentCmdBuffer->End();
        if (mHasAcquiredSwapchain) CurrentCmdBuffer->AddWaitSemaphore(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, mImageAvailableSemaphores[mCurrentFrameIndex]);
        for (auto& Set : mDrawSets) CurrentCmdBuffer->KeepDescriptorSet(std::move(Set));
        for (auto& Pool : mDrawPools) CurrentCmdBuffer->KeepDescriptorPool(std::move(Pool));
        mFrameSubmissions[mCurrentFrameIndex] = {CurrentCmdBuffer, CurrentCmdBuffer->GetCompletedFenceCounter()};
        mCmdBufferManager->SubmitActiveCmdBuffer(mHasAcquiredSwapchain ? std::vector<VulkanSemaphore*>{mRenderFinishedSemaphores[mCurrentFrameIndex]} : std::vector<VulkanSemaphore*>{});
        mDrawPools.clear(); mDrawSets.clear();
        mPresentFrameIndex = mCurrentFrameIndex;
        mCurrentFrameIndex = (mCurrentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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
        if (OldSrcLayout == VK_IMAGE_LAYOUT_UNDEFINED || OldSrcLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
            throw std::logic_error("Cannot copy an uninitialized texture");
        if (OldDstLayout == VK_IMAGE_LAYOUT_UNDEFINED || OldDstLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
            OldDstLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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

    void VulkanCommandContext::RHIPrepareGfxPipeline(RHIPipeline* Pipeline, const RHIRenderPassInfo& Pass)
    {
        CheckRHICreationThread();
        auto* Base = static_cast<VulkanGfxPipeline*>(Pipeline);
        auto Key = VulkanRenderPassCache::BuildRenderPassKey(Pass);
        if (!mDevice->GetPiplineManager()->GetOrCreateGfxPipelineForRenderPass(Base->GetPipelineState(), Key, Base))
            throw std::runtime_error("Failed to prepare pass-compatible pipeline");
    }

	void VulkanCommandContext::RHIBindGfxPipeline(RHIPipeline* InPipeline)
	{
		VulkanCommandBuffer* CurrentCmdBuffer = mCmdBufferManager->GetActiveCmdBuffer();
		VulkanGfxPipeline* VkGfxPipeline = static_cast<VulkanGfxPipeline*>(InPipeline);
		
		// 如果当前有活动的 RenderPass，检查是否需要使用 Pipeline 变体
		if (mCurrentRenderPassKey.NumColorTargets > 0 || mCurrentRenderPassKey.DepthFormat != VK_FORMAT_UNDEFINED)
		{
			// 从 Pipeline State 获取或创建与当前 RenderPass 兼容的变体
			auto Variant = VkGfxPipeline->mVariants.find(mCurrentRenderPassKey);
            if (Variant == VkGfxPipeline->mVariants.end()) throw std::logic_error("Pipeline variant was not prepared on RenderThread");
            VulkanGfxPipeline* CompatiblePipeline = Variant->second.get();
			
			if (CompatiblePipeline)
			{
				VkGfxPipeline = CompatiblePipeline;
			}
		}
		
		mCurrentGfxPipeline = VkGfxPipeline;
		vkCmdBindPipeline(CurrentCmdBuffer->GetHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, VkGfxPipeline->GetHandle());
	}

    RHIBufferHandle VulkanCommandContext::RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData)
    {
        CheckRHICreationThread();
        if (!InSize) throw std::invalid_argument("Buffer size must be positive");
        if (InData && !EnumHasAnyFlags(Usage, BufferUsageFlags::KeepCPUAccessible))
            throw std::logic_error("GPU buffer data must use UploadBuffer and the RHI command list");
        RHIBufferHandle Buffer = new VulkanBuffer(mDevice, InSize, Usage, ResourceState, Stride);
        if (InData) static_cast<VulkanBuffer*>(Buffer.get())->MapMemoryDataToBuffer(InData, InSize);
        return Buffer;
    }

	RHIBufferHandle VulkanCommandContext::RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
        CheckRHICreationThread();
		return RHICreateBuffer(Size, Usage, Stride, ResourceState, InData);
	}



	RHIBufferHandle VulkanCommandContext::RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData)
	{
        CheckRHICreationThread();
		return RHICreateBuffer(Size, Usage, 0, ResourceState, InData);
	}

    RHIUniformBufferView VulkanCommandContext::RHIAllocateUniformData(const void* Data, size_t Size)
    {
        CheckRHICreationThread();
        const auto& Limits = mDevice->GetPhyDevice()->GetDeviceProperties().limits;
        if (!Data || !Size || Size > Limits.maxUniformBufferRange) throw std::invalid_argument("Invalid uniform allocation size");
        const size_t Alignment = std::max<size_t>(16, Limits.minUniformBufferOffsetAlignment);
        const size_t Aligned = (Size + Alignment - 1) / Alignment * Alignment;
        UniformPage* Selected = nullptr;
        for (auto& Page : mUniformPages)
        {
            // CPU command payloads and GPU submissions both own this same real page.
            // Sole pool ownership proves there are no recorded or in-flight readers.
            if (Page.Buffer->UseCount() == 1) Page.Used = 0;
            if (Page.Capacity - Page.Used >= Aligned) { Selected = &Page; break; }
        }
        if (!Selected)
        {
            const size_t Capacity = std::max<size_t>(256 * 1024, Aligned);
            RHIUniformBufferDesc Desc; Desc.Size = Capacity;
            mUniformPages.push_back({RHICreateUniformBufferWithSlot(Desc), 0, Capacity});
            Selected = &mUniformPages.back();
        }
        RHIUniformBufferView View{Selected->Buffer, Selected->Used, Size};
        static_cast<VulkanUniformBuffer*>(Selected->Buffer.get())->UpdateBuffer(Data, View.Offset, Size);
        Selected->Used += Aligned;
        return View;
    }

    RHITextureHandle VulkanCommandContext::RHICreateTexture(const RHITextureDesc& Desc, void* Data)
    {
        CheckRHICreationThread();
        if (Data) throw std::logic_error("Texture data must use UploadTexture and the RHI command list");
        if (Desc.mExtent.x <= 0 || Desc.mExtent.y <= 0 || !Desc.mNumMips || !Desc.mNumSamples || !Desc.mArraySize)
            throw std::invalid_argument("Invalid texture dimensions");
        if (Desc.mDimension == TextureDimension::Texture3D || Desc.mDimension == TextureDimension::TextureCubeArray)
            throw std::invalid_argument("Unsupported texture dimension");
        return new VulkanTexture(mDevice, Desc);
    }
    RHITextureHandle VulkanCommandContext::RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData)
    {
        RHITextureDesc Desc; Desc.mExtent = Vector2(SizeX, SizeY); Desc.mFormat = static_cast<PixelFormat>(Format);
        Desc.mNumMips = NumMips; Desc.mNumSamples = NumSamples; Desc.mFlags = Flags;
        return RHICreateTexture(Desc, InData);
    }
    RHITextureHandle VulkanCommandContext::RHICreateTexture3D(uint32, uint32, uint32, uint8, uint32, TextureCreateFlags, ERHIAccess, void*)
    { CheckRHICreationThread(); throw std::invalid_argument("3D textures are not supported by this upload path"); }

	RHIPipelineHandle VulkanCommandContext::RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState)
	{
        CheckRHICreationThread();
        auto Pipeline = RHIPipelineHandle(mDevice->GetPiplineManager()->GetOrCreateGfxPipeline(InPipelineState));
        if (Pipeline) Pipeline->SetDescription(std::make_shared<const GRAPHICS_PIPELINE_STATE_DESC>(InPipelineState));
        return Pipeline;
	}

	RHIPipelineHandle VulkanCommandContext::RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState)
	{
        CheckRHICreationThread();
		return nullptr;
	}

	RHISwapchainHandle VulkanCommandContext::RHICreateSwapchain(uint32 InWidth, uint32 InHeight, bool InbIsFullscreen, PixelFormat InFormat)
	{
        CheckRHICreationThread();
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

	RHIUniformBufferHandle VulkanCommandContext::RHICreateUniformBuffer(size_t InSize, void* InContents)
	{
        CheckRHICreationThread();
		RHIUniformBuffer* NewUniformBuffer = new  VulkanUniformBuffer(mDevice, InSize, WritableMask, InContents);
		return NewUniformBuffer;
	}



	//==========================================================================
	// Slot-based UniformBuffer API
	//==========================================================================

	RHIUniformBufferHandle VulkanCommandContext::RHICreateUniformBufferWithSlot(const RHIUniformBufferDesc& InDesc)
	{
        CheckRHICreationThread();
		VulkanUniformBuffer* buffer = new VulkanUniformBuffer(
			mDevice,
			InDesc.Slot,
			InDesc.Size,
			ERHIAccess::VertexOrIndexBuffer,
			const_cast<void*>(InDesc.InitialData));
		
        buffer->SetDescription(InDesc);
		return buffer;
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
			vkCmdBindIndexBuffer(CurrentCmdBuffer->GetHandle(), VkIndexBuffer->GetHandle(), VkIndexBuffer->GetOffset(), VkIndexBuffer->GetStride() == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
			
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
    void VulkanCommandContext::RHIKeepResources(const std::vector<RHIHandle>& Resources)
    {
        CheckRHIContextThread();
        for (const auto& Resource : Resources) mCmdBufferManager->GetActiveCmdBuffer()->Keep(Resource);
    }
    void VulkanCommandContext::RHICopyBufferRegion(RHIBuffer* Dest, uint64 DstOffset, RHIBuffer* Source, uint64 SrcOffset, uint64 Size)
    {
        CheckRHIContextThread();
        auto* Target = static_cast<VulkanBuffer*>(Dest); auto* Staging = static_cast<VulkanBuffer*>(Source);
        if (!Size || DstOffset > Target->GetBufferSize() || Size > Target->GetBufferSize() - DstOffset || SrcOffset > Staging->GetBufferSize() || Size > Staging->GetBufferSize() - SrcOffset)
            throw std::out_of_range("Buffer copy exceeds allocation");
        auto* Cmd = mCmdBufferManager->GetActiveCmdBuffer();
        Cmd->Keep(Dest); Cmd->Keep(Source);
        Cmd->CopyBuffer(Staging->GetHandle(), Target->GetHandle(), Size, Staging->GetOffset() + SrcOffset, Target->GetOffset() + DstOffset);
    }
    void VulkanCommandContext::RHICopyBufferToTexture(RHIBuffer* Source, RHITexture* Target)
    {
        CheckRHIContextThread();
        auto* Texture = static_cast<VulkanTexture*>(Target); auto* Staging = static_cast<VulkanBuffer*>(Source);
        const auto& Desc = Texture->GetDesc();
        const uint32 Layers = Desc.mDimension == TextureDimension::TextureCube ? 6 : Desc.mDimension == TextureDimension::Texture2DArray ? Desc.mArraySize : 1;
        const uint64 Expected = uint64(Desc.mExtent.x) * uint64(Desc.mExtent.y) * Layers * GPixelFormats[Desc.mFormat].BlockBytes;
        if (Desc.mNumMips != 1 || Expected != Staging->GetBufferSize()) throw std::invalid_argument("Invalid buffer-to-texture copy layout");
        auto* Cmd = mCmdBufferManager->GetActiveCmdBuffer(); Cmd->Keep(Source); Cmd->Keep(Target);
        auto Range = VulkanPipelineBarrier::MakeSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, Layers);
        VulkanSetImageLayout(Cmd->GetHandle(), Texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, Range);
        VkBufferImageCopy Region{}; Region.bufferOffset = Staging->GetOffset();
        Region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, Layers};
        Region.imageExtent = {static_cast<uint32>(Desc.mExtent.x), static_cast<uint32>(Desc.mExtent.y), 1};
        vkCmdCopyBufferToImage(Cmd->GetHandle(), Staging->GetHandle(), Texture->getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &Region);
        VulkanSetImageLayout(Cmd->GetHandle(), Texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, Range);
        Texture->SetAccess(ERHIAccess::SRVGraphics);
    }

    void VulkanCommandContext::RHISubmitUploads()
    {
        CheckRHIContextThread();
        mCmdBufferManager->SubmitActiveCmdBuffer({});
    }

}
