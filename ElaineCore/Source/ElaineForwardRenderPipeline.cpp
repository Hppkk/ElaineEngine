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
			mLastViewportWidth = Width;
			mLastViewportHeight = Height;
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

		CmdList->EndFrame();

		if (Swapchain)
		{
			CmdList->PresentSwapchain(Swapchain);
		}
	}
}
