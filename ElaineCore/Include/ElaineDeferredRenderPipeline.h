#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineRenderPipeline.h"

namespace Elaine
{
	class PostProcessChain;
	class SceneManager;
	class RenderView;
	class RHITexture;
	class VTIndirectionTexture;

	/**
	 * Deferred Rendering Pipeline (UE5-style)
	 *
	 * Rendering flow:
	 *   1. [VT Feedback Pass]     - Reduced resolution, output tile IDs (optional)
	 *   2. [VT CPU Update]        - Analyze feedback, stream tiles, update indirection
	 *   3. [Shadow Pass]          - Render shadow maps
	 *   4. [GBuffer Pass]         - Render scene to GBuffer (BaseColor+M, Normal+R, Emissive+AO, Depth)
	 *   5. [Deferred Lighting]    - Full-screen pass, read GBuffer, apply PBR lighting
	 *   6. [Transparent Forward]  - Forward render transparent objects on top
	 *   7. [Post Process]         - Tone mapping, bloom, etc.
	 *   8. [Copy to BackBuffer]   - Final blit to swapchain
	 *
	 * GBuffer Layout:
	 *   RT0: RGBA8  - BaseColor.rgb + Metallic.a
	 *   RT1: RGB10A2 or RGBA16F - WorldNormal.rgb + Roughness.a
	 *   RT2: RGBA8  - Emissive.rgb + AO.a
	 *   Depth: D32F - Scene depth
	 */
	class ElaineCoreExport DeferredRenderPipeline : public RenderPipeline
	{
	public:
		DeferredRenderPipeline();
		virtual ~DeferredRenderPipeline();
		virtual void Initialize() override;
		virtual void Render(RenderView* InRenderView) override;

	private:
		// Virtual Texture per-frame CPU-side update
		void UpdateVirtualTextures(RHICommandList* CmdList, uint32 FrameNumber);

		PostProcessChain* mPostProcessChain = nullptr;

		//=========================================================================
		// GBuffer Textures
		//=========================================================================
		// RT0: BaseColor(RGB) + Metallic(A) - RGBA8_UNORM (sRGB for color)
		RHITexture* mGBufferA = nullptr;

		// RT1: WorldNormal(RGB) + Roughness(A) - RGBA16F for precision
		RHITexture* mGBufferB = nullptr;

		// RT2: Emissive(RGB) + AO(A) - RGBA8_UNORM
		RHITexture* mGBufferC = nullptr;

		// Depth: D32_SFLOAT
		RHITexture* mSceneDepthTexture = nullptr;

		//=========================================================================
		// Other Persistent Textures
		//=========================================================================
		// Shadow map
		RHITexture* mShadowMapTexture = nullptr;

		// Scene color (output of lighting pass, input of post-process)
		RHITexture* mSceneColorTexture = nullptr;

		// Virtual Texture feedback
		RHITexture* mVTFeedbackTexture = nullptr;
		RHITexture* mVTFeedbackDepthTexture = nullptr;
		bool mVTEnabled = false;

		//=========================================================================
		// State
		//=========================================================================
		uint32 mLastViewportWidth = 0;
		uint32 mLastViewportHeight = 0;
		uint32 mShadowMapSize = 2048;
	};
}
