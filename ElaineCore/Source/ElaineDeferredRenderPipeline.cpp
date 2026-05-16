#include "ElainePrecompiledHeader.h"
#include "ElaineDeferredRenderPipeline.h"
#include "PostProcess/ElainePostProcessManager.h"
#include "RenderGraph/ElaineRenderGraph.h"
#include "RenderGraph/ElaineRenderGraphBuilder.h"
#include "RenderGraph/ElaineRenderGraphResourcePool.h"
#include "render/ElaineRenderSystem.h"
#include "render/common/ElaineRHI.h"
#include "ElaineSceneManager.h"
#include "ElaineRenderQueue.h"
#include "render/common/ElaineRHICommandList.h"
#include "ElaineRenderFrameData.h"
#include "ElaineRenderView.h"
#include "RenderGraph/ElaineRenderGraphResource.h"
#include "ElaineRenderTarget.h"
#include "ElaineSwapchainRenderTarget.h"

namespace Elaine
{
	DeferredRenderPipeline::DeferredRenderPipeline()
	{
		mRPType = RP_Defferred;
	}

	DeferredRenderPipeline::~DeferredRenderPipeline()
	{
		if (mPostProcessChain)
		{
			delete mPostProcessChain;
			mPostProcessChain = nullptr;
		}
	}

	void DeferredRenderPipeline::Initialize()
	{
		mPostProcessChain = new PostProcessChain();
		mPostProcessChain->Initialize("render/config/deferred_render_pipeline.json");

	}

	void DeferredRenderPipeline::Render(RenderView* InRenderView)
	{
		if (!InRenderView || !InRenderView->IsValid()) return;

		SceneManager* SceneMgr = InRenderView->mSceneManager;
		Camera* CameraIns = InRenderView->mCamera;
		RenderTarget* Target = InRenderView->mRenderTarget;

		SceneMgr->PrepareRenderData(CameraIns);

		RenderFrameData& FrameData = SceneMgr->GetRenderFrameData();
		RenderQueueSet* QueueSet = FrameData.mRenderQueueSet;

		if (!QueueSet || QueueSet->IsEmpty())
			return;

		RHICommandContext* GraphicsContext = GetDynamicRHI()->GetDefaultCommandContext();
		RHICommandList* CmdList = GraphicsContext->GetRHICommandListMgr()->CreateCommandList();

		CmdList->BeginFrame();

		// Acquire swapchain
		RHISwapchain* Swapchain = nullptr;
		RHITexture* AcquiredBackBuffer = nullptr;
		if (Target && Target->IsSwapchainTarget())
		{
			Swapchain = Target->GetSwapchain();
			if (Swapchain)
			{
				CmdList->AcquireSwapchainImage(Swapchain);
				AcquiredBackBuffer = Swapchain->AcquireNextTexture();
			}
		}
		else if (Target)
		{
			AcquiredBackBuffer = Target->GetTarget();
		}

		auto& RG = *RenderGraph::RenderDependencyGraph::instance();
		RG.BeginFrame();
		RenderGraph::RenderGraphBuilder& Builder = RG.GetBuilder();

		uint32 Width = 1920;
		uint32 Height = 1080;
		InRenderView->GetViewportSize(Width, Height);

		CmdList->UpdateCommonUniformBuffer(SceneMgr->GetCommonUniformBufferRHI(),
			sizeof(CommonUniformBufferCPU), &FrameData.mCommonUniformBuffer);

		// Detect viewport resize
		bool bNeedRebuild = (mLastViewportWidth != Width || mLastViewportHeight != Height);
		if (bNeedRebuild)
		{
			mGBufferA = nullptr;
			mGBufferB = nullptr;
			mGBufferC = nullptr;
			mSceneDepthTexture = nullptr;
			mSceneColorTexture = nullptr;
			mLastViewportWidth = Width;
			mLastViewportHeight = Height;
		}


		//=========================================================================
		// Resource Descriptors
		//=========================================================================
		auto& ResourcePool = RG.GetResourcePool();

		// Shadow Map
		RenderGraph::RGTextureDesc ShadowMapDesc = RenderGraph::RGTextureDesc::Create2D(
			mShadowMapSize, mShadowMapSize, PF_DepthStencil,
			TextureCreateFlags::DepthStencilTargetable | TextureCreateFlags::ShaderResource);
		ShadowMapDesc.ClearColor = LinearColor(1.0f, 0.0f, 0.0f, 0.0f);

		// GBuffer A: BaseColor(RGB) + Metallic(A)
		RenderGraph::RGTextureDesc GBufferADesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_R8G8B8A8,
			TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource);
		GBufferADesc.ClearColor = LinearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// GBuffer B: WorldNormal(RGB) + Roughness(A) - use 16F for normal precision
		RenderGraph::RGTextureDesc GBufferBDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_FloatRGBA,
			TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource);
		GBufferBDesc.ClearColor = LinearColor(0.5f, 0.5f, 1.0f, 0.5f); // default normal = up, roughness = 0.5

		// GBuffer C: Emissive(RGB) + AO(A)
		RenderGraph::RGTextureDesc GBufferCDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_R8G8B8A8,
			TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource);
		GBufferCDesc.ClearColor = LinearColor(0.0f, 0.0f, 0.0f, 1.0f); // AO = 1.0 default

		// Scene Depth
		RenderGraph::RGTextureDesc DepthDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_DepthStencil,
			TextureCreateFlags::DepthStencilTargetable | TextureCreateFlags::ShaderResource);
		DepthDesc.ClearColor = LinearColor(1.0f, 0.0f, 0.0f, 0.0f);

		// Scene Color (output of lighting pass)
		RenderGraph::RGTextureDesc SceneColorDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_R8G8B8A8,
			TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource);
		SceneColorDesc.ClearColor = LinearColor(0.0f, 0.0f, 0.0f, 1.0f);

		// Acquire persistent textures
		if (!mShadowMapTexture)
			mShadowMapTexture = ResourcePool.AcquirePersistentTexture("ShadowMap", ShadowMapDesc, GraphicsContext);
		if (!mGBufferA)
			mGBufferA = ResourcePool.AcquirePersistentTexture("GBufferA", GBufferADesc, GraphicsContext);
		if (!mGBufferB)
			mGBufferB = ResourcePool.AcquirePersistentTexture("GBufferB", GBufferBDesc, GraphicsContext);
		if (!mGBufferC)
			mGBufferC = ResourcePool.AcquirePersistentTexture("GBufferC", GBufferCDesc, GraphicsContext);
		if (!mSceneDepthTexture)
			mSceneDepthTexture = ResourcePool.AcquirePersistentTexture("SceneDepth", DepthDesc, GraphicsContext);
		if (!mSceneColorTexture)
			mSceneColorTexture = ResourcePool.AcquirePersistentTexture("SceneColor", SceneColorDesc, GraphicsContext);

		// Import to RenderGraph
		auto ShadowMap = Builder.ImportTexture("ShadowMap", mShadowMapTexture, ShadowMapDesc);
		auto GBufferA = Builder.ImportTexture("GBufferA", mGBufferA, GBufferADesc);
		auto GBufferB = Builder.ImportTexture("GBufferB", mGBufferB, GBufferBDesc);
		auto GBufferC = Builder.ImportTexture("GBufferC", mGBufferC, GBufferCDesc);
		auto SceneDepth = Builder.ImportTexture("SceneDepth", mSceneDepthTexture, DepthDesc);
		auto SceneColor = Builder.ImportTexture("SceneColor", mSceneColorTexture, SceneColorDesc);

		//=========================================================================
		// Pass 1: VT Feedback Pass (optional)
		//=========================================================================


		//=========================================================================
		// Pass 2: Shadow Pass
		//=========================================================================
		struct ShadowPassData { RenderGraph::RGTextureHandle ShadowMap; };
		Builder.AddRasterPass<ShadowPassData>("ShadowPass",
			[&](RenderGraph::RenderGraphBuilder& B, ShadowPassData& D) {
				RenderGraph::RGDepthStencilDesc DSD;
				DSD.LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
				DSD.ClearDepth = 1.0f;
				B.SetDepthStencil(ShadowMap, DSD);
				D.ShadowMap = ShadowMap;
			},
			[=](RHICommandList* InCmdList, const ShadowPassData& D) {
				if (auto* Q = QueueSet->GetRenderQueue(RenderQueue_Shadow)) {
					if (!Q->IsEmpty()) {
						CmdList->SetViewport(0, 0, 0, (float)mShadowMapSize, (float)mShadowMapSize, 1.0f);
						CmdList->SetScissorRect(true, 0, 0, mShadowMapSize, mShadowMapSize);
						Q->Render(CmdList);
					}
				}
			}
		);

		//=========================================================================
		// Pass 3: GBuffer Pass
		//=========================================================================
		struct GBufferPassData {
			RenderGraph::RGTextureHandle GBufferA, GBufferB, GBufferC, Depth;
		};
		Builder.AddRasterPass<GBufferPassData>("GBufferPass",
			[&](RenderGraph::RenderGraphBuilder& B, GBufferPassData& D) {
				RenderGraph::RGRenderTargetDesc RTD;
				RTD.LoadStoreOp = ERenderTargetActions::Clear_Store;

				// Set 3 MRT outputs
				RTD.ClearColor = GBufferADesc.ClearColor;
				B.SetRenderTarget(0, GBufferA, RTD);

				RTD.ClearColor = GBufferBDesc.ClearColor;
				B.SetRenderTarget(1, GBufferB, RTD);

				RTD.ClearColor = GBufferCDesc.ClearColor;
				B.SetRenderTarget(2, GBufferC, RTD);

				RenderGraph::RGDepthStencilDesc DSD;
				DSD.LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
				DSD.ClearDepth = 1.0f;
				B.SetDepthStencil(SceneDepth, DSD);

				D.GBufferA = GBufferA;
				D.GBufferB = GBufferB;
				D.GBufferC = GBufferC;
				D.Depth = SceneDepth;
			},
			[=](RHICommandList* InCmdList, const GBufferPassData& D) {
				CmdList->SetViewport(0, 0, 0, (float)Width, (float)Height, 1.0f);
				CmdList->SetScissorRect(true, 0, 0, Width, Height);

				// Render all opaque objects to GBuffer.
				// If VT is enabled, the render queue handles material-level switching:
				//   - VT materials: use DeferredGBufferVT.ps pipeline + VT descriptor bindings
				//   - Traditional materials: use DeferredGBuffer.ps pipeline + texture bindings
				//
				// The VT resource binding is handled per-drawcall via the RenderQueue's
				// material iteration. When a renderable has a VT-tagged material:
				//   1. RenderQueue detects VT flag via VTMaterialBindingManager
				//   2. Switches to VT pipeline variant (DeferredGBufferVT pass)
				//   3. Calls VTDescriptorSetBinder::BindVTResources() to bind set=2
				//
				// For non-VT materials, the standard per-material textures are bound normally.


				if (auto* Q = QueueSet->GetRenderQueue(RenderQueue_Normal))
				{
					if (mVTEnabled)
					{

					}
					else
					{
						// Standard rendering path (no VT)
						Q->Render(CmdList);
					}
				}
			}
		);

		//=========================================================================
		// Pass 4: Deferred Lighting Pass (Full-screen)
		//=========================================================================
		struct LightingPassData {
			RenderGraph::RGTextureHandle GBufferA, GBufferB, GBufferC, Depth, ShadowMap;
			RenderGraph::RGTextureHandle SceneColor;
		};
		Builder.AddRasterPass<LightingPassData>("DeferredLightingPass",
			[&](RenderGraph::RenderGraphBuilder& B, LightingPassData& D) {
				// Read GBuffer and depth as shader resources
				D.GBufferA = B.ReadTexture(GBufferA, ERHIAccess::SRVGraphics);
				D.GBufferB = B.ReadTexture(GBufferB, ERHIAccess::SRVGraphics);
				D.GBufferC = B.ReadTexture(GBufferC, ERHIAccess::SRVGraphics);
				D.Depth = B.ReadTexture(SceneDepth, ERHIAccess::SRVGraphics);
				D.ShadowMap = B.ReadTexture(ShadowMap, ERHIAccess::SRVGraphics);

				// Write to scene color
				RenderGraph::RGRenderTargetDesc RTD;
				RTD.LoadStoreOp = ERenderTargetActions::Clear_Store;
				RTD.ClearColor = SceneColorDesc.ClearColor;
				B.SetRenderTarget(0, SceneColor, RTD);
				D.SceneColor = SceneColor;
			},
			[=](RHICommandList* InCmdList, const LightingPassData& D) {
				CmdList->SetViewport(0, 0, 0, (float)Width, (float)Height, 1.0f);
				CmdList->SetScissorRect(true, 0, 0, Width, Height);

				// Draw full-screen triangle (3 vertices, no vertex buffer)
				// The DeferredLighting pipeline should be bound via the render queue
				// or directly through the command list's pipeline binding
				CmdList->DrawPrimitive(0, 1, 0); // 3 vertices, 1 instance
			}
		);

		//=========================================================================
		// Pass 5: Transparent Forward Pass (rendered on top of deferred result)
		//=========================================================================
		struct TransparentPassData {
			RenderGraph::RGTextureHandle SceneColor, Depth;
		};
		Builder.AddRasterPass<TransparentPassData>("TransparentForwardPass",
			[&](RenderGraph::RenderGraphBuilder& B, TransparentPassData& D) {
				RenderGraph::RGRenderTargetDesc RTD;
				RTD.LoadStoreOp = ERenderTargetActions::Load_Store; // Don't clear - keep lighting result
				B.SetRenderTarget(0, SceneColor, RTD);

				RenderGraph::RGDepthStencilDesc DSD;
				DSD.LoadStoreOp = EDepthStencilTargetActions::LoadDepthStencil_StoreDepthStencil;
				B.SetDepthStencil(SceneDepth, DSD);

				D.SceneColor = SceneColor;
				D.Depth = SceneDepth;
			},
			[=](RHICommandList* InCmdList, const TransparentPassData& D) {
				CmdList->SetViewport(0, 0, 0, (float)Width, (float)Height, 1.0f);
				CmdList->SetScissorRect(true, 0, 0, Width, Height);

				// Render sky
				if (auto* Q = QueueSet->GetRenderQueue(RenderQueue_Sky))
					Q->Render(CmdList);

				// Render transparent objects with forward shading
				if (auto* Q = QueueSet->GetRenderQueue(RenderQueue_Transparent))
					Q->Render(CmdList);
			}
		);

		//=========================================================================
		// Pass 6: Copy to BackBuffer
		//=========================================================================
		if (AcquiredBackBuffer)
		{
			RenderGraph::RGTextureDesc BackBufferDesc = RenderGraph::RGTextureDesc::Create2D(
				Width, Height, PF_R8G8B8A8, TextureCreateFlags::RenderTargetable);
			auto BackBuffer = Builder.ImportTexture("BackBuffer", AcquiredBackBuffer, BackBufferDesc);

			struct CopyPassData {
				RenderGraph::RGTextureHandle Source, Dest;
			};
			Builder.AddCopyPass<CopyPassData>("CopyToBackBuffer",
				[&](RenderGraph::RenderGraphBuilder& B, CopyPassData& D) {
					D.Source = B.ReadTexture(SceneColor, ERHIAccess::CopySrc);
					D.Dest = B.WriteTexture(BackBuffer, ERHIAccess::CopyDest);
					B.MarkForPresent(BackBuffer);
				},
				[&](RHICommandList* InCmdList, const CopyPassData& D) {
					RHITexture* Src = RG.GetRHITexture(D.Source);
					RHITexture* Dst = RG.GetRHITexture(D.Dest);
					if (Src && Dst) {
						RHICopyTextureInfo CopyInfo;
						CopyInfo.Size.x = Width;
						CopyInfo.Size.y = Height;
						CopyInfo.Size.z = 1;
						CmdList->CopyTexture(Src, Dst, CopyInfo);
					}
				}
			);
		}

		//=========================================================================
		// Execute RenderGraph
		//=========================================================================
		RG.Execute(GraphicsContext);
		RG.EndFrame();

		CmdList->EndFrame();

		if (Swapchain)
		{
			CmdList->PresentSwapchain(Swapchain);
		}
	}

	//=========================================================================
	// Virtual Texture CPU-side Update
	//=========================================================================

}
