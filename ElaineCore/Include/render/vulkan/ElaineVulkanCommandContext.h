#pragma once
#include "render/common/ElaineRHICommandContext.h"


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

	class ElaineCoreExport VulkanCommandContext :public Elaine::RHICommandContext
	{
	public:
		VulkanCommandContext(VulkanDevice* InDevice, VulkanQueue* InQueue);
		~VulkanCommandContext();
		void Initilize();
		void Deinitilize();
		//void AllocCommand(EM_RHICommand InCmd, void* InRHIResourceHandle);
		VulkanDevice* GetDevice() { return mDevice; }
		VulkanInstance* GetInstance() { return mInstance; }
		VulkanPhysicalDevice* GetPhyDevice() { return mPhyDevice; } 

		virtual void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) override;

		virtual void RHIDispatchIndirectComputeShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) override;

		// Useful when used with geometry shader (emit polygons to different viewports), otherwise SetViewPort() is simpler
		// @param Count >0
		// @param Data must not be 0
		virtual void RHISetMultipleViewports(uint32 Count, const ViewportBounds* Data) override;

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

		virtual void RHIEndRenderPass() override;

		virtual void RHINextSubpass() override;

		virtual void RHICopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo) override;

		virtual void RHICopyBufferRegion(RHIBuffer* DestBuffer, uint64 DstOffset, RHIBuffer* SourceBuffer, uint64 SrcOffset, uint64 NumBytes) override;

		virtual void RHIBindGfxPipeline(RHIPipeline* InPipeline);

		virtual RHIBuffer* RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData) override;

		virtual RHIBuffer* RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) override;

		virtual void RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* Contents) override;

		virtual RHIBuffer* RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) override;

		virtual RHITexture* RHICreateTexture(const RHITextureDesc& InDesc) override;

		virtual RHITexture* RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) override;

		virtual RHITexture* RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) override;
		
		virtual RHIPipeline* RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState) override;
		virtual RHIPipeline* RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState) override;

		void RHIWriteGPUFence(RHIGPUFence* FenceRHI) override;
		VulkanCommandBufferManager* GetCommandBufferManager() const { return mCmdBufferManager; }
		VulkanGfxPipeline* GetCurrentGfxPipeline() const { return mCurrentGfxPipeline; }
		VulkanComputePipeline* GetCurrentComputePipeline() const { return mCurrentComputePipeline; }
		void SetCurrentGfxPipeline(VulkanGfxPipeline* InGfxPipeline) { mCurrentGfxPipeline = InGfxPipeline; }
		void SetCurrentComputePipeline(VulkanComputePipeline* InPipeline) { mCurrentComputePipeline = InPipeline; }
	private:
		VulkanDevice* mDevice = nullptr;
		VulkanPhysicalDevice* mPhyDevice = nullptr;
		VulkanInstance* mInstance = nullptr;
		VulkanCommandBufferManager* mCmdBufferManager = nullptr;
		VulkanQueue* mQueue;
		VulkanGfxPipeline* mCurrentGfxPipeline = nullptr;
		VulkanComputePipeline* mCurrentComputePipeline = nullptr;
		void* mWindowHandle = nullptr;
		friend class VulkanDynamicRHI;
	};

}