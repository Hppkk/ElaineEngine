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

	class ElaineCoreExport ForwardRenderPipeline : public RenderPipeline
	{
	public:
		ForwardRenderPipeline();
		virtual ~ForwardRenderPipeline();
		virtual void Initialize() override;
		virtual void Render(RenderView* InRenderView) override;

	private:
		// 确保持久纹理已创建（在首次渲染的 Execute 阶段调用）
		void EnsurePersistentTexturesCreated(uint32 ViewportWidth, uint32 ViewportHeight);

		// Virtual Texture per-frame CPU-side update (feedback analyze + streaming + upload)
		void UpdateVirtualTextures(RHICommandList* CmdList, uint32 FrameNumber);

		PostProcessChain* mPostProcessChain = nullptr;

		// 持久渲染目标（由资源池管理，首次使用时创建）
		RHITexture* mShadowMapTexture = nullptr;
		RHITexture* mSceneColorTexture = nullptr;
		RHITexture* mSceneDepthTexture = nullptr;

		// Virtual Texture resources
		RHITexture* mVTFeedbackTexture = nullptr;          // R32_UINT feedback RT
		RHITexture* mVTFeedbackDepthTexture = nullptr;     // Depth for feedback pass
		bool mVTEnabled = false;

		// 上次创建时的尺寸，用于检测是否需要重建
		uint32 mLastViewportWidth = 0;
		uint32 mLastViewportHeight = 0;
		uint32 mShadowMapSize = 2048;
	};
}
