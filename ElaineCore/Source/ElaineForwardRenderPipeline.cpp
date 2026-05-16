#include "ElainePrecompiledHeader.h"
#include "ElaineForwardRenderPipeline.h"
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
// Virtual Texture includes
#include "VirtualTexture/ElaineVirtualTextureTypes.h"
#include "VirtualTexture/ElaineVTFeedbackAnalyzer.h"
#include "VirtualTexture/ElaineVTStreamingManager.h"
#include "VirtualTexture/ElaineVTIndirectionTexture.h"
#include "VirtualTexture/ElaineVirtualTextureSpace.h"
#include "VirtualTexture/ElainePhysicalTilePool.h"

namespace Elaine
{
	ForwardRenderPipeline::ForwardRenderPipeline()
	{
		mRPType = RP_Forward;
	}

	ForwardRenderPipeline::~ForwardRenderPipeline()
	{

	}

	void ForwardRenderPipeline::Initialize()
	{
		mPostProcessChain = new PostProcessChain();
		mPostProcessChain->Initialize("render/config/forward_render_pipeline.json");

		// Check if VirtualTextureSystem is available and initialized
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (VTSystem && VTSystem->IsInitialized())
		{
			mVTEnabled = true;
		}
	}

	void ForwardRenderPipeline::Render(RenderView* InRenderView)
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

		Elaine::RHICommandContext* GraphicsContext = GetDynamicRHI()->GetDefaultCommandContext();
		RHICommandList* CmdList = GraphicsContext->GetRHICommandListMgr()->CreateCommandList();

		CmdList->BeginFrame();

		// @TODO
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

		CmdList->UpdateCommonUniformBuffer(SceneMgr->GetCommonUniformBufferRHI(), sizeof(CommonUniformBufferCPU), &FrameData.mCommonUniformBuffer);

		bool bNeedRebuildViewportTextures = (mLastViewportWidth != Width || mLastViewportHeight != Height);
		if (bNeedRebuildViewportTextures)
		{
			mSceneColorTexture = nullptr;
			mSceneDepthTexture = nullptr;
			mVTFeedbackTexture = nullptr;
			mVTFeedbackDepthTexture = nullptr;
			mLastViewportWidth = Width;
			mLastViewportHeight = Height;
		}

		//=========================================================================
		// Virtual Texture: CPU-side update (analyze previous frame feedback,
		// dispatch streaming, upload completed tiles, update indirection textures)
		//=========================================================================
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (mVTEnabled && VTSystem && VTSystem->IsInitialized())
		{
			static uint32 sFrameCounter = 0;
			UpdateVirtualTextures(CmdList, sFrameCounter++);
		}

		RenderGraph::RGTextureDesc ShadowMapDesc = RenderGraph::RGTextureDesc::Create2D(
			mShadowMapSize, mShadowMapSize, PF_DepthStencil, TextureCreateFlags::DepthStencilTargetable | TextureCreateFlags::ShaderResource);
		ShadowMapDesc.ClearColor = LinearColor(1.0f, 0.0f, 0.0f, 0.0f);

		RenderGraph::RGTextureDesc SceneColorDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_R8G8B8A8, TextureCreateFlags::RenderTargetable | TextureCreateFlags::ShaderResource);
		SceneColorDesc.ClearColor = LinearColor(0.0f, 0.2f, 0.4f, 1.0f);

		RenderGraph::RGTextureDesc DepthDesc = RenderGraph::RGTextureDesc::Create2D(
			Width, Height, PF_DepthStencil, TextureCreateFlags::DepthStencilTargetable);
		DepthDesc.ClearColor = LinearColor(1.0f, 0.0f, 0.0f, 0.0f);

		auto& ResourcePool = RG.GetResourcePool();
		if (!mShadowMapTexture)
		{
			mShadowMapTexture = ResourcePool.AcquirePersistentTexture("ShadowMap", ShadowMapDesc, GraphicsContext);
		}
		if (!mSceneColorTexture)
		{
			mSceneColorTexture = ResourcePool.AcquirePersistentTexture("SceneColor", SceneColorDesc, GraphicsContext);
		}
		if (!mSceneDepthTexture)
		{
			mSceneDepthTexture = ResourcePool.AcquirePersistentTexture("SceneDepth", DepthDesc, GraphicsContext);
		}

		RenderGraph::RGTextureHandle ShadowMap = Builder.ImportTexture("ShadowMap", mShadowMapTexture, ShadowMapDesc);
		RenderGraph::RGTextureHandle SceneColor = Builder.ImportTexture("SceneColor", mSceneColorTexture, SceneColorDesc);
		RenderGraph::RGTextureHandle SceneDepth = Builder.ImportTexture("SceneDepth", mSceneDepthTexture, DepthDesc);

		//=========================================================================
		// VT Feedback Pass - renders at reduced resolution to capture tile requests
		//=========================================================================
		RenderGraph::RGTextureHandle VTFeedbackRT;
		RenderGraph::RGTextureHandle VTFeedbackDepth;

		if (mVTEnabled && VTSystem && VTSystem->IsInitialized())
		{
			uint32 FBWidth = Width / VTConstants::FeedbackDownscaleFactor;
			uint32 FBHeight = Height / VTConstants::FeedbackDownscaleFactor;
			if (FBWidth < 1) FBWidth = 1;
			if (FBHeight < 1) FBHeight = 1;

			RenderGraph::RGTextureDesc VTFeedbackDesc = RenderGraph::RGTextureDesc::Create2D(
				FBWidth, FBHeight, PF_R32_UINT,
				TextureCreateFlags::RenderTargetable | TextureCreateFlags::CPUReadback);
			VTFeedbackDesc.ClearColor = LinearColor(0.0f, 0.0f, 0.0f, 0.0f);

			RenderGraph::RGTextureDesc VTFeedbackDepthDesc = RenderGraph::RGTextureDesc::Create2D(
				FBWidth, FBHeight, PF_DepthStencil,
				TextureCreateFlags::DepthStencilTargetable);
			VTFeedbackDepthDesc.ClearColor = LinearColor(1.0f, 0.0f, 0.0f, 0.0f);

			auto& ResourcePool = RG.GetResourcePool();
			if (!mVTFeedbackTexture)
			{
				mVTFeedbackTexture = ResourcePool.AcquirePersistentTexture("VTFeedback", VTFeedbackDesc, GraphicsContext);
			}
			if (!mVTFeedbackDepthTexture)
			{
				mVTFeedbackDepthTexture = ResourcePool.AcquirePersistentTexture("VTFeedbackDepth", VTFeedbackDepthDesc, GraphicsContext);
			}

			VTFeedbackRT = Builder.ImportTexture("VTFeedback", mVTFeedbackTexture, VTFeedbackDesc);
			VTFeedbackDepth = Builder.ImportTexture("VTFeedbackDepth", mVTFeedbackDepthTexture, VTFeedbackDepthDesc);

			struct VTFeedbackPassData
			{
				RenderGraph::RGTextureHandle FeedbackRT;
				RenderGraph::RGTextureHandle FeedbackDepth;
			};

			Builder.AddRasterPass<VTFeedbackPassData>("VTFeedbackPass",
				[&](RenderGraph::RenderGraphBuilder& Builder, VTFeedbackPassData& Data)
				{
					RenderGraph::RGRenderTargetDesc RTDesc;
					RTDesc.LoadStoreOp = ERenderTargetActions::Clear_Store;
					RTDesc.ClearColor = VTFeedbackDesc.ClearColor;

					RenderGraph::RGDepthStencilDesc DSDesc;
					DSDesc.LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
					DSDesc.ClearDepth = 1.0f;

					Builder.SetRenderTarget(0, VTFeedbackRT, RTDesc);
					Builder.SetDepthStencil(VTFeedbackDepth, DSDesc);
					Data.FeedbackRT = VTFeedbackRT;
					Data.FeedbackDepth = VTFeedbackDepth;
				},
				[=](Elaine::RHICommandList* InCmdList, const VTFeedbackPassData& Data)
				{
					uint32 FBW = Width / VTConstants::FeedbackDownscaleFactor;
					uint32 FBH = Height / VTConstants::FeedbackDownscaleFactor;
					if (FBW < 1) FBW = 1;
					if (FBH < 1) FBH = 1;

					CmdList->SetViewport(0, 0, 0, (float)FBW, (float)FBH, 1.0f);
					CmdList->SetScissorRect(true, 0, 0, FBW, FBH);

					// Render all opaque objects with VT feedback shader
					// This uses VTFeedback.vs/ps to output packed tile coords
					if (auto* Queue = QueueSet->GetRenderQueue(RenderQueue_Normal))
					{
						Queue->RenderWithOverrideMaterial(CmdList, "VTFeedback");
					}
				}
			);
		}

		//=========================================================================
		// Shadow Pass
		//=========================================================================
		struct ShadowPassData
		{
			RenderGraph::RGTextureHandle ShadowMap;
		};

		Builder.AddRasterPass<ShadowPassData>("ShadowPass",
			[&](RenderGraph::RenderGraphBuilder& Builder, ShadowPassData& Data)
			{
				RenderGraph::RGDepthStencilDesc DSDesc;
				DSDesc.LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
				DSDesc.ClearDepth = 1.0f;
				
				Builder.SetDepthStencil(ShadowMap, DSDesc);
				Data.ShadowMap = ShadowMap;
			},
			[=](Elaine::RHICommandList* InCmdList, const ShadowPassData& Data)
			{
				if (auto* ShadowQueue = QueueSet->GetRenderQueue(RenderQueue_Shadow))
				{
					if (!ShadowQueue->IsEmpty())
					{
						CmdList->SetViewport(0, 0, 0, (float)mShadowMapSize, (float)mShadowMapSize, 1.0f);
						CmdList->SetScissorRect(true, 0, 0, mShadowMapSize, mShadowMapSize);
						ShadowQueue->Render(CmdList);
					}
				}
			}
		);

		//=========================================================================
		// Base Pass (Opaque + Sky + Transparent +...)
		//=========================================================================
		struct BasePassData
		{
			RenderGraph::RGTextureHandle RenderTarget;
			RenderGraph::RGTextureHandle DepthStencil;
			RenderGraph::RGTextureHandle ShadowMap;
		};

		Builder.AddRasterPass<BasePassData>("BasePass",
			[&](RenderGraph::RenderGraphBuilder& Builder, BasePassData& Data)
			{
				RenderGraph::RGRenderTargetDesc RTDesc;
				RTDesc.LoadStoreOp = ERenderTargetActions::Clear_Store;
				RTDesc.ClearColor = SceneColorDesc.ClearColor;

				RenderGraph::RGDepthStencilDesc DSDesc;
				DSDesc.LoadStoreOp = EDepthStencilTargetActions::ClearDepthStencil_StoreDepthStencil;
				DSDesc.ClearDepth = 1.0f;

				Builder.SetRenderTarget(0, SceneColor, RTDesc);
				Builder.SetDepthStencil(SceneDepth, DSDesc);
				
				Data.RenderTarget = SceneColor;
				Data.DepthStencil = SceneDepth;
				Data.ShadowMap = Builder.ReadTexture(ShadowMap, ERHIAccess::SRVGraphics);
			},
			[=](Elaine::RHICommandList* InCmdList, const BasePassData& Data)
			{
				CmdList->SetViewport(0, 0, 0, (float)Width, (float)Height, 1.0f);
				CmdList->SetScissorRect(true, 0, 0, Width, Height);
					
				if (auto* Queue = QueueSet->GetRenderQueue(RenderQueue_Normal))
					Queue->Render(CmdList);

				if (auto* Queue = QueueSet->GetRenderQueue(RenderQueue_Sky))
					Queue->Render(CmdList);

				if (auto* Queue = QueueSet->GetRenderQueue(RenderQueue_Transparent))
					Queue->Render(CmdList);
			}
		);

		//=========================================================================
		// Copy Pass - [SceneColor -> Swapchain Image]
		//=========================================================================
		if (AcquiredBackBuffer)
		{
			RenderGraph::RGTextureDesc BackBufferDesc = RenderGraph::RGTextureDesc::Create2D(
				Width, Height, PF_R8G8B8A8, TextureCreateFlags::RenderTargetable);
			RenderGraph::RGTextureHandle BackBuffer = Builder.ImportTexture("BackBuffer", AcquiredBackBuffer, BackBufferDesc);

			struct CopyPassData
			{
				RenderGraph::RGTextureHandle Source;
				RenderGraph::RGTextureHandle Dest;
			};

			Builder.AddCopyPass<CopyPassData>("CopyToBackBuffer",
				[&](RenderGraph::RenderGraphBuilder& Builder, CopyPassData& Data)
				{
					Data.Source = Builder.ReadTexture(SceneColor, ERHIAccess::CopySrc);
					Data.Dest = Builder.WriteTexture(BackBuffer, ERHIAccess::CopyDest);
					Builder.MarkForPresent(BackBuffer);
				},
				[&](Elaine::RHICommandList* InCmdList, const CopyPassData& Data)
				{
					RHITexture* SourceTex = RG.GetRHITexture(Data.Source);
					RHITexture* DestTex = RG.GetRHITexture(Data.Dest);
					if (SourceTex && DestTex)
					{
						RHICopyTextureInfo CopyInfo;
						CopyInfo.Size.x = Width;
						CopyInfo.Size.y = Height;
						CopyInfo.Size.z = 1;
						CmdList->CopyTexture(SourceTex, DestTex, CopyInfo);
					}
				}
			);
		}

		//=========================================================================
		// Execute RenderGraph
		//=========================================================================
		RG.Execute(GraphicsContext);
		RG.EndFrame();

		// After RenderGraph execution, the VT feedback buffer is now filled
		// The next frame's UpdateVirtualTextures() will read it back and analyze

		CmdList->EndFrame();

		if (Swapchain)
		{
			CmdList->PresentSwapchain(Swapchain);
		}
	}

	//=========================================================================
	// Virtual Texture CPU-side Update
	//=========================================================================
	void ForwardRenderPipeline::UpdateVirtualTextures(RHICommandList* CmdList, uint32 FrameNumber)
	{
		VirtualTextureSystem* VTSystem = VirtualTextureSystem::instance();
		if (!VTSystem || !VTSystem->IsInitialized())
			return;

		// Step 1: Update the VT system (analyzes previous frame's feedback,
		//         processes tile requests, updates page tables)
		VTSystem->Update(FrameNumber);

		// Step 2: Update indirection textures for all active spaces
		// The VirtualTextureSystem::Update() internally calls:
		//   - AnalyzeFeedback()        → reads back feedback buffer, extracts tile requests
		//   - ProcessTileRequests()    → submits to streaming manager
		//   - UpdatePageTables()       → processes completed tile uploads
		//   - UpdateIndirectionTextures() → triggers VTIndirectionTexture::Update()
		//
		// The indirection texture upload commands need to be recorded into
		// the current command list. This is handled by the VT system internally
		// through the RHI command context.
		//
		// Note: The feedback buffer readback from GPU→CPU has 1 frame latency
		// by design (we read this frame what was rendered last frame).
	}
}
