#pragma once
#include "render/common/ElaineRHICommandContext.h"
#include "render/vulkan/ElaineVulkanTypes.h"
#include "render/vulkan/ElaineVulkanRenderPassCache.h"


namespace VulkanRHI
{
	class VulkanDevice;
	class VulkanPhysicalDevice;
	class VulkanInstance;
	class VulkanCommandList;
	class VulkanDynamicRHI;
	class VulkanQueue;
	class VulkanCommandBufferManager;
	class VulkanGfxPipeline;
	class VulkanComputePipeline;
	class VulkanFence;
	class VulkanSemaphore;
	class VulkanDescriptorSet;
	class VulkanUniformBuffer;
	class VulkanDescriptorSetManager;
	class VulkanSwapChain;
	class VulkanSamplerCache;
	class VulkanRenderPassCache;
	class VulkanRenderPassCache;
	class VulkanFramebufferCache;
	class VulkanFrameDescriptorAllocator;

	class ElaineCoreExport VulkanCommandContext :public Elaine::RHICommandContext
	{
	public:
		VulkanCommandContext(VulkanDevice* InDevice, VulkanQueue* InQueue);
		~VulkanCommandContext();
		void Initialize();
		void Deinitilize();
		//void AllocCommand(EM_RHICommand InCmd, void* InRHIResourceHandle);
		VulkanDevice* GetDevice() { return mDevice; }
		VulkanInstance* GetInstance() { return mInstance; }
		VulkanPhysicalDevice* GetPhyDevice() { return mPhyDevice; } 

		virtual void RHIWaitIdle() override;

		virtual void RHIWindowResize(RHISwapchain* InSwapchain, uint32 InWidth, uint32 InHeight) override;

		virtual void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;

		virtual void RHIDispatchIndirectComputeShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) override;

		// Useful when used with geometry shader (emit polygons to different viewports), otherwise SetViewPort() is simpler
		// @param Count >0
		// @param Data must not be 0
		virtual void RHISetMultipleViewports(uint32 Count, const ViewportBounds* Data) override;

		virtual void RHISetSwapchain(RHISwapchain* InSwapchain) override;

		/**
		* Resolves from one texture to another.
		* @param SourceTexture - texture to resolve from, 0 is silently ignored
		* @param DestTexture - texture to resolve to, 0 is silently ignored
		* @param ResolveParams - optional resolve params
		* @param Fence - optional fence, will be set once copy is completed by GPU
		*/
		virtual void RHICopyToResolveTarget(RHITexture* SourceTexture, RHITexture* DestTexture, const ResolveParams& ResolveParams) override;

		virtual void RHIBeginRenderQuery(RHIRenderQuery* RenderQuery) override;

		virtual void RHIEndRenderQuery(RHIRenderQuery* RenderQuery) override;

		// Not all RHIs need this (Mobile specific)
		virtual void RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask) override;

		
		virtual void RHIBeginDrawingViewport(RHIViewport* Viewport, RHITexture* RenderTargetRHI) override;

		
		virtual void RHIEndDrawingViewport(RHIViewport* Viewport, bool bPresent, bool bLockToVsync) override;

		
		virtual void RHIBeginFrame() override;

		
		virtual void RHIEndFrame() override;

		//=========================================================================
		// 交换链控制接口实现
		//=========================================================================
		virtual RHITexture* RHIAcquireSwapchainImage(RHISwapchain* InSwapchain) override;
		virtual void RHIPresentSwapchain(RHISwapchain* InSwapchain, bool bVsync = true) override;


		/**
		* Signals the beginning of scene rendering. The RHI makes certain caching assumptions between
		* calls to BeginScene/EndScene. Currently the only restriction is that you can't update texture
		* references.
		*/
		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIBeginScene() override;

		/**
		* Signals the end of scene rendering. See RHIBeginScene.
		*/
		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIEndScene() override;

		/**
		* Signals the beginning and ending of rendering to a resource to be used in the next frame on a multiGPU system
		*/
		virtual void RHIBeginUpdateMultiFrameResource(RHITexture* Texture) override;

		virtual void RHIEndUpdateMultiFrameResource(RHITexture* Texture) override;

		virtual void RHISetStreamSource(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset) override;

		// @param MinX including like Win32 RECT
		// @param MinY including like Win32 RECT
		// @param MaxX excluding like Win32 RECT
		// @param MaxY excluding like Win32 RECT
		virtual void RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ) override;

		virtual void RHISetStereoViewport(float LeftMinX, float RightMinX, float LeftMinY, float RightMinY, float MinZ, float LeftMaxX, float RightMaxX, float LeftMaxY, float RightMaxY, float MaxZ) override;

		// @param MinX including like Win32 RECT
		// @param MinY including like Win32 RECT
		// @param MaxX excluding like Win32 RECT
		// @param MaxY excluding like Win32 RECT
		virtual void RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) override;

		virtual void RHISetGraphicsPipelineState(RHIGraphicsPipelineState* GraphicsState, uint32 StencilRef, bool bApplyAdditionalState) override;

		/** Set the shader resource view of a surface. */
		virtual void RHISetShaderTexture(RHIShader* Shader, uint32 TextureIndex, RHITexture* NewTexture) override;

		/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
		virtual void RHISetPixelShaderTexture(RHIShader* PixelShader, uint32 TextureIndex, RHITexture* NewTexture) override;

		/**
		* Sets sampler state.
		* @param ComputeShader		The compute shader to set the sampler for.
		* @param SamplerIndex		The index of the sampler.
		* @param NewState			The new sampler state.
		*/
		virtual void RHISetComputeShaderSampler(RHIShader* ComputeShader, uint32 SamplerIndex, RHISampler* NewState) override;

		/**
		* Sets sampler state.
		* @param Shader				The shader to set the sampler for.
		* @param SamplerIndex		The index of the sampler.
		* @param NewState			The new sampler state.
		*/
		virtual void RHISetShaderSampler(RHIShader* Shader, uint32 SamplerIndex, RHISampler* NewState) override;

		virtual void RHISetShaderUniformBuffer(RHIShader* Shader, uint32 BufferIndex, RHIUniformBuffer* Buffer) override;

		virtual void RHISetComputeShaderUniformBuffer(RHIShader* ComputeShader, uint32 BufferIndex, RHIUniformBuffer* Buffer) override;

		virtual void RHISetShaderParameter(RHIShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) override;

		virtual void RHISetComputeShaderParameter(RHIShader* ComputeShader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) override;

		virtual void RHISetStencilRef(uint32 StencilRef) {}

		virtual void RHISetBlendFactor(const LinearColor& BlendFactor) {}

		virtual void RHIBindDrawData(GRAPHICS_PIPELINE_STATE_DESC* InDrawData) override;

		virtual void RHIBindResourceBinding(const RHI_DRAW_RESOURCE_BINDING& InResourceBinding) override;

		virtual void RHIDrawPrimitive(uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances) override;

		virtual void RHIDrawPrimitiveIndirect(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) override;

		virtual void RHIDrawIndexedIndirect(RHIBuffer* IndexBufferRHI, RHIBuffer* ArgumentsBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances) override;

		// @param NumPrimitives need to be >0 
		virtual void RHIDrawIndexedPrimitive(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances) override;

		virtual void RHIDrawIndexedPrimitiveIndirect(RHIBuffer* IndexBuffer, RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) override;

		virtual void RHIDispatchMeshShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;

		virtual void RHIDispatchIndirectMeshShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) override;

		/**
		* Sets Depth Bounds range with the given min/max depth.
		* @param MinDepth	The minimum depth for depth bounds test
		* @param MaxDepth	The maximum depth for depth bounds test.
		*					The valid values for fMinDepth and fMaxDepth are such that 0 <= fMinDepth <= fMaxDepth <= 1
		*/
		virtual void RHISetDepthBounds(float MinDepth, float MaxDepth) override;

		virtual void RHIBeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState/*const RHIRenderPassInfo& InInfo, const TCHAR* InName*/) override;

		// 新增：支持 RenderGraph 动态设置渲染目标
		virtual void RHIBeginRenderPass(const RHIRenderPassInfo& InRenderPassInfo, const char * InName) override;

		virtual void RHIEndRenderPass() override;

		virtual void RHINextSubpass() override;

		virtual void RHICopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo) override;

		virtual void RHICopyBufferRegion(RHIBuffer* DestBuffer, uint64 DstOffset, RHIBuffer* SourceBuffer, uint64 SrcOffset, uint64 NumBytes) override;

		// 资源屏障
		virtual void RHIResourceBarrier(const RHIResourceBarrierDesc& Barrier) override;

		virtual void RHIBindGfxPipeline(RHIPipeline* InPipeline);
		
		// 获取当前 RenderPass 的 Key（用于 Pipeline 变体查找）
		const RenderPassKey& GetCurrentRenderPassKey() const { return mCurrentRenderPassKey; }

		virtual RHIBuffer* RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData) override;

		virtual RHIBuffer* RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) override;

		virtual void RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* Contents, size_t InSize) override;

		virtual RHIBuffer* RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) override;

		virtual RHITexture* RHICreateTexture(const RHITextureDesc& InDesc, void* InContent) override;

		virtual RHITexture* RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) override;

		virtual RHITexture* RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) override;
		
		virtual RHIPipeline* RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState) override;
		virtual RHIPipeline* RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState) override;
		virtual RHISwapchain* RHICreateSwapchain(uint32 InWidth, uint32 InHeight, bool InbIsFullscreen, PixelFormat InFormat) override;

		virtual void RHICreateSurface(void* InNativeHandle) override;

		virtual RHIUniformBuffer* RHICreateUniformBuffer(size_t InSize, void* InContents) override;
		virtual void RHIUpdateCommonUniformBuffer(RHIUniformBuffer* InUniformBufferRHI, size_t InSize, void* InContents) override;

		// 新增：带语义槽位的 UniformBuffer 创建和绑定
		virtual RHIUniformBuffer* RHICreateUniformBufferWithSlot(const RHIUniformBufferDesc& InDesc) override;
		virtual void RHIBindUniformBuffer(RHIUniformSlot InSlot, RHIUniformBuffer* InBuffer) override;

		void RHIWriteGPUFence(RHIGPUFence* FenceRHI) override;
		VulkanCommandBufferManager* GetCommandBufferManager() const { return mCmdBufferManager; }
		VulkanGfxPipeline* GetCurrentGfxPipeline() const { return mCurrentGfxPipeline; }
		VulkanComputePipeline* GetCurrentComputePipeline() const { return mCurrentComputePipeline; }
		void SetCurrentGfxPipeline(VulkanGfxPipeline* InGfxPipeline) { mCurrentGfxPipeline = InGfxPipeline; }
		void SetCurrentComputePipeline(VulkanComputePipeline* InPipeline) { mCurrentComputePipeline = InPipeline; }
		void SetCommonDescriptorSets(VulkanDescriptorSet* InDescroptorSet, size_t InIndex);
		bool IsCreateCommonDescriptorSets() const { return mIsCreateCommonDescriptorSets; }
		VulkanDescriptorSetManager* GetDescriptorSetManager() const { return mDescriptorSetManager; }
		VkSurfaceKHR GetVulkanSurface() const { return mSurface; }
		VulkanSamplerCache* GetSamplerCache() const { return mSamplerCache; }
		uint32_t GetCurrentFrameIndex() const { return mCurrentFrameIndex; }
		VkRenderPass GetCurrentRenderPass() const { return mCurrentRenderPass; }
		VulkanRenderPassCache* GetRenderPassCache() const { return mRenderPassCache; }
	private:
		void* mWindowHandle = nullptr;
		VulkanDevice* mDevice = nullptr;
		VulkanPhysicalDevice* mPhyDevice = nullptr;
		VulkanInstance* mInstance = nullptr;
		VulkanCommandBufferManager* mCmdBufferManager = nullptr;
		VulkanQueue* mQueue = nullptr;
		VulkanGfxPipeline* mCurrentGfxPipeline = nullptr;
		VulkanComputePipeline* mCurrentComputePipeline = nullptr;
		VulkanDescriptorSetManager* mDescriptorSetManager = nullptr;
		VulkanSemaphore* mImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
		VulkanSemaphore* mRenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
		VulkanDescriptorSet* mCommonDescriptorSet = nullptr;
		RHIUniformBuffer* mBoundUniformBuffers[(size_t)RHIUniformSlot::Count] = { nullptr };
		bool mIsCreateCommonDescriptorSets = false;
		int mCurrentFrameIndex = 0;
		uint32_t mCurrentImageIndex = 0;
		VulkanSamplerCache* mSamplerCache = nullptr;
		VulkanSwapChain* mSwapchain = nullptr;
		VulkanViewport* mDefaultViewport = nullptr;
		VkSurfaceKHR mSurface = nullptr;

		// RenderPass 和 Framebuffer 缓存
		VulkanRenderPassCache* mRenderPassCache = nullptr;
		VulkanFramebufferCache* mFramebufferCache = nullptr;
		VkRenderPass mCurrentRenderPass = VK_NULL_HANDLE;
		RenderPassKey mCurrentRenderPassKey;  // 当前 RenderPass 的 Key，用于 Pipeline 变体选择

		// 动态资源描述符分配器
		VulkanFrameDescriptorAllocator* mFrameDescriptorAllocator = nullptr;

		friend class VulkanDynamicRHI;
	};

}