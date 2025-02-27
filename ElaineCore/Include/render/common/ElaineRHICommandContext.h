#pragma once
#include "render/common/ElaineRHIProtocol.h"

namespace Elaine
{
	class RHICommandListManager;

	class ElaineCoreExport RHICommandContext
	{
	public:
		RHICommandContext();
		virtual ~RHICommandContext();
		virtual void RHIDispatchComputeShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ) = 0;

		virtual void RHIDispatchIndirectComputeShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) = 0;

		// Useful when used with geometry shader (emit polygons to different viewports), otherwise SetViewPort() is simpler
		// @param Count >0
		// @param Data must not be 0
		virtual void RHISetMultipleViewports(uint32 Count, const ViewportBounds* Data) = 0;

		/**
		* Resolves from one texture to another.
		* @param SourceTexture - texture to resolve from, 0 is silently ignored
		* @param DestTexture - texture to resolve to, 0 is silently ignored
		* @param ResolveParams - optional resolve params
		* @param Fence - optional fence, will be set once copy is completed by GPU
		*/
		virtual void RHICopyToResolveTarget(RHITexture* SourceTexture, RHITexture* DestTexture, const ResolveParams& ResolveParams) = 0;

		virtual void RHIBeginRenderQuery(RHIRenderQuery* RenderQuery) = 0;

		virtual void RHIEndRenderQuery(RHIRenderQuery* RenderQuery) = 0;

		// Not all RHIs need this (Mobile specific)
		virtual void RHIDiscardRenderTargets(bool Depth, bool Stencil, uint32 ColorBitMask) {}

		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIBeginDrawingViewport(RHIViewport* Viewport, RHITexture* RenderTargetRHI) = 0;

		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIEndDrawingViewport(RHIViewport* Viewport, bool bPresent, bool bLockToVsync) = 0;

		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIBeginFrame() = 0;

		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIEndFrame() = 0;

		/**
		* Signals the beginning of scene rendering. The RHI makes certain caching assumptions between
		* calls to BeginScene/EndScene. Currently the only restriction is that you can't update texture
		* references.
		*/
		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIBeginScene() = 0;

		/**
		* Signals the end of scene rendering. See RHIBeginScene.
		*/
		// This method is queued with an RHIThread, otherwise it will flush after it is queued; without an RHI thread there is no benefit to queuing this frame advance commands
		virtual void RHIEndScene() = 0;

		/**
		* Signals the beginning and ending of rendering to a resource to be used in the next frame on a multiGPU system
		*/
		virtual void RHIBeginUpdateMultiFrameResource(RHITexture* Texture)
		{
			/* empty default implementation */
		}

		virtual void RHIEndUpdateMultiFrameResource(RHITexture* Texture)
		{
			/* empty default implementation */
		}

		virtual void RHISetStreamSource(uint32 StreamIndex, RHIBuffer* VertexBuffer, uint32 Offset) = 0;

		// @param MinX including like Win32 RECT
		// @param MinY including like Win32 RECT
		// @param MaxX excluding like Win32 RECT
		// @param MaxY excluding like Win32 RECT
		virtual void RHISetViewport(float MinX, float MinY, float MinZ, float MaxX, float MaxY, float MaxZ) = 0;

		virtual void RHISetStereoViewport(float LeftMinX, float RightMinX, float LeftMinY, float RightMinY, float MinZ, float LeftMaxX, float RightMaxX, float LeftMaxY, float RightMaxY, float MaxZ)
		{
			/* empty default implementation */
		}

		// @param MinX including like Win32 RECT
		// @param MinY including like Win32 RECT
		// @param MaxX excluding like Win32 RECT
		// @param MaxY excluding like Win32 RECT
		virtual void RHISetScissorRect(bool bEnable, uint32 MinX, uint32 MinY, uint32 MaxX, uint32 MaxY) = 0;

		virtual void RHISetGraphicsPipelineState(RHIGraphicsPipelineState* GraphicsState, uint32 StencilRef, bool bApplyAdditionalState) = 0;

		void RHISetGraphicsPipelineState(RHIGraphicsPipelineState* GraphicsState, bool bApplyAdditionalState)
		{
			RHISetGraphicsPipelineState(GraphicsState, 0, bApplyAdditionalState);
		}

		/** Set the shader resource view of a surface. */
		virtual void RHISetShaderTexture(RHIShader* Shader, uint32 TextureIndex, RHITexture* NewTexture) = 0;

		/** Set the shader resource view of a surface.  This is used for binding TextureMS parameter types that need a multi sampled view. */
		virtual void RHISetPixelShaderTexture(RHIShader* PixelShader, uint32 TextureIndex, RHITexture* NewTexture) = 0;

		/**
		* Sets sampler state.
		* @param ComputeShader		The compute shader to set the sampler for.
		* @param SamplerIndex		The index of the sampler.
		* @param NewState			The new sampler state.
		*/
		virtual void RHISetComputeShaderSampler(RHIShader* ComputeShader, uint32 SamplerIndex, RHISampler* NewState) = 0;

		/**
		* Sets sampler state.
		* @param Shader				The shader to set the sampler for.
		* @param SamplerIndex		The index of the sampler.
		* @param NewState			The new sampler state.
		*/
		virtual void RHISetShaderSampler(RHIShader* Shader, uint32 SamplerIndex, RHISampler* NewState) = 0;

		virtual void RHISetShaderUniformBuffer(RHIShader* Shader, uint32 BufferIndex, RHIUniformBuffer* Buffer) = 0;

		virtual void RHISetComputeShaderUniformBuffer(RHIShader* ComputeShader, uint32 BufferIndex, RHIUniformBuffer* Buffer) = 0;

		virtual void RHISetShaderParameter(RHIShader* Shader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) = 0;

		virtual void RHISetComputeShaderParameter(RHIShader* ComputeShader, uint32 BufferIndex, uint32 BaseIndex, uint32 NumBytes, const void* NewValue) = 0;

		virtual void RHISetStencilRef(uint32 StencilRef) {}

		virtual void RHISetBlendFactor(const LinearColor& BlendFactor) {}

		virtual void RHIBindDrawData(GRAPHICS_PIPELINE_STATE_DESC* InDrawData) = 0;

		virtual void RHIDrawPrimitive(uint32 BaseVertexIndex, uint32 NumPrimitives, uint32 NumInstances) = 0;

		virtual void RHIDrawPrimitiveIndirect(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) = 0;

		virtual void RHIDrawIndexedIndirect(RHIBuffer* IndexBufferRHI, RHIBuffer* ArgumentsBufferRHI, int32 DrawArgumentsIndex, uint32 NumInstances) = 0;

		// @param NumPrimitives need to be >0 
		virtual void RHIDrawIndexedPrimitive(RHIBuffer* IndexBuffer, int32 BaseVertexIndex, uint32 FirstInstance, uint32 NumVertices, uint32 StartIndex, uint32 NumPrimitives, uint32 NumInstances) = 0;

		virtual void RHIDrawIndexedPrimitiveIndirect(RHIBuffer* IndexBuffer, RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset) = 0;

		virtual void RHIDispatchMeshShader(uint32 ThreadGroupCountX, uint32 ThreadGroupCountY, uint32 ThreadGroupCountZ)
		{
			/* empty default implementation */
		}

		virtual void RHIDispatchIndirectMeshShader(RHIBuffer* ArgumentBuffer, uint32 ArgumentOffset)
		{
			/* empty default implementation */
		}

		/**
		* Sets Depth Bounds range with the given min/max depth.
		* @param MinDepth	The minimum depth for depth bounds test
		* @param MaxDepth	The maximum depth for depth bounds test.
		*					The valid values for fMinDepth and fMaxDepth are such that 0 <= fMinDepth <= fMaxDepth <= 1
		*/
		virtual void RHISetDepthBounds(float MinDepth, float MaxDepth) = 0;

		virtual void RHIBeginRenderPass(const GRAPHICS_PIPELINE_STATE_DESC& InGfxState/*const RHIRenderPassInfo& InInfo, const TCHAR* InName*/) = 0;

		virtual void RHIEndRenderPass() = 0;

		virtual void RHINextSubpass()
		{
		}

		virtual void RHICopyTexture(RHITexture* SourceTexture, RHITexture* DestTexture, const RHICopyTextureInfo& CopyInfo) = 0;

		virtual void RHICopyBufferRegion(RHIBuffer* DestBuffer, uint64 DstOffset, RHIBuffer* SourceBuffer, uint64 SrcOffset, uint64 NumBytes)
		{
			
		}
		virtual void RHIBindGfxPipeline(RHIPipeline* InPipeline) = 0;
		virtual RHIBuffer* RHICreateBuffer(uint32 InSize, BufferUsageFlags Usage, uint32 Stride, ERHIAccess ResourceState, void* InData) = 0;

		virtual RHIBuffer* RHICreateIndexBuffer(uint32 Stride, uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) = 0;

		virtual void RHIUpdateUniformBuffer(RHIUniformBuffer* UniformBufferRHI, const void* Contents)  = 0;
		
		virtual RHIBuffer* RHICreateVertexBuffer(uint32 Size, BufferUsageFlags Usage, ERHIAccess ResourceState, void* InData) = 0;

		virtual RHITexture* RHICreateTexture(const RHITextureDesc& InDesc) = 0;

		virtual RHITexture* RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) = 0;

		virtual RHITexture* RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, TextureCreateFlags Flags, ERHIAccess ResourceState, void* InData) = 0;

		virtual RHIPipeline* RHICreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InPipelineState) = 0;
		virtual RHIPipeline* RHICreateComputePipeline(const ComputePipelineStateDesc& InPipelineState) = 0;

		virtual void RHIWriteGPUFence(RHIGPUFence* FenceRHI) { }

		//virtual void RHIBeginUpdateMultiFrameResource(RHIUnorderedAccessView* UAV)
		//{
		//	/* empty default implementation */
		//}

		//virtual void RHIEndUpdateMultiFrameResource(RHIUnorderedAccessView* UAV)
		//{
		//	/* empty default implementation */
		//}

		//virtual void RHICalibrateTimers()
		//{
		//	/* empty default implementation */
		//}

		//virtual void RHICalibrateTimers(FRHITimestampCalibrationQuery* CalibrationQuery)
		//{
		//	/* empty default implementation */
		//}

		// Used for OpenGL to check and see if any occlusion queries can be read back on the RHI thread. If they aren't ready when we need them, then we end up stalling.
		//virtual void RHIPollOcclusionQueries()
		//{
		//	/* empty default implementation */
		//}


		/**
		* Sets a pixel shader UAV parameter.
		* @param PixelShader		The pixel shader to set the UAV for.
		* @param UAVIndex		The index of the UAVIndex.
		* @param UAV			The new UAV.
		*/
		//virtual void RHISetUAVParameter(RHIShader* PixelShader, uint32 UAVIndex, FRHIUnorderedAccessView* UAV) = 0;
		
		
		/**
		* Sets a compute shader UAV parameter.
		* @param ComputeShader	The compute shader to set the UAV for.
		* @param UAVIndex		The index of the UAVIndex.
		* @param UAV			The new UAV.
		*/
		//virtual void RHISetUAVParameter(RHIShader* ComputeShader, uint32 UAVIndex, FRHIUnorderedAccessView* UAV) = 0;
		
		/**
		* Sets a compute shader counted UAV parameter and initial count
		* @param ComputeShader	The compute shader to set the UAV for.
		* @param UAVIndex		The index of the UAVIndex.
		* @param UAV			The new UAV.
		* @param InitialCount	The initial number of items in the UAV.
		*/
		//virtual void RHISetUAVParameter(RHIShader* ComputeShader, uint32 UAVIndex, FRHIUnorderedAccessView* UAV, uint32 InitialCount) = 0;
		
		//virtual void RHISetShaderResourceViewParameter(RHIShader* ComputeShader, uint32 SamplerIndex, RHIShaderResourceView* SRV) = 0;
		
		//virtual void RHISetShaderResourceViewParameter(RHIShader* Shader, uint32 SamplerIndex, RHIShaderResourceView* SRV) = 0;
		RHICommandListManager* GetRHICommandListMgr() const { return mRHICmdListMgr; }
	private:
		RHICommandListManager* mRHICmdListMgr = nullptr;
	};
}