#include "ElaineEditorGlobalContext.h"
#include "ElaineEngine.h"
#include "ElaineViewport.h"
#include "ElaineOffscreenRenderTarget.h"
#include "ElaineWorld.h"
#include "ElaineActor.h"
#include "ElaineCameraComponent.h"
#include "ElaineSkyComponent.h"
#include "render/ElaineRenderDocCapture.h"
#include "ElaineMeshComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineRenderContext.h"
#include "ElaineRenderSystem.h"
#include "ElaineLogSystem.h"
#include "d3d11_1.h"
#include "TaskGraph/ElaineTask.h"

namespace Editor
{
	EditorGlobalContext::EditorGlobalContext()
	{
	}

	EditorGlobalContext::~EditorGlobalContext()
	{
		Destroy();
	}

	void EditorGlobalContext::Initialize(Elaine::ElaineEngine* InEngine)
	{
		mEngine = InEngine;
		mGizmoManager.Initialize();
	}

	void EditorGlobalContext::Destroy()
	{
		mGizmoManager.Shutdown();
		mGridManager.Shutdown();
		mSelectedActor = nullptr;
        // Stop producing frames before retiring either API's texture references.
        if (mSceneViewport && mEngine)
        {
            mEngine->UnregisterViewport(mSceneViewport);
            mEngine->DestroyViewport(mSceneViewport);
            mSceneViewport = nullptr;
        }
        if (mInterop) { mInterop->Shutdown(); mInterop.reset(); }
		// Staging fallback 路径
		if (mStagingSRV)
		{
			mStagingSRV->Release();
			mStagingSRV = nullptr;
		}
		if (mStagingD3D11Texture)
		{
			mStagingD3D11Texture->Release();
			mStagingD3D11Texture = nullptr;
		}
		mPixelBuffer.clear();
		mbStagingReady = false;

		delete mOffscreenRT;
		mOffscreenRT = nullptr;

        for (auto& Owner : mOffscreenOwners) Owner.reset();
		mDefaultCamera = nullptr;
		mDefaultWorld = nullptr;
	}

	void EditorGlobalContext::CreateDefaultScene()
	{
		if (!mEngine)
			return;

		// --- 创建默认世界 ---
		mDefaultWorld = mEngine->CreateWorld();
		SetActiveWorld(mDefaultWorld);

		// Camera
		Elaine::Actor* CameraObj = mDefaultWorld->CreateActor();
		mDefaultCamera = CameraObj->AddComponentType<Elaine::CameraComponent>("CameraComponent");
        // Euler vector APIs use radians; use an explicit, upright starting view.
        mDefaultCamera->SetPosition(Elaine::Vector3(0, 3, -8));
        mDefaultCamera->LookAt(Elaine::Vector3::ZERO);


		// Sky
		Elaine::Actor* SkyObj = mDefaultWorld->CreateActor();
		SkyObj->AddComponentType<Elaine::SkyComponent>("SkyComponent");

		// Editor Grid (infinite ground plane at Y=0)
		mGridManager.Initialize(mDefaultWorld);

		// Test Mesh
		Elaine::Actor* MeshObj = mDefaultWorld->CreateActor();
		Elaine::StaticMeshComponent* MeshComp = MeshObj->AddComponentType<Elaine::StaticMeshComponent>("StaticMeshComponent");
		MeshComp->ChangeMesh("");

		// --- 创建离屏渲染目标 ---
		mOffscreenRT = new Elaine::OffscreenRenderTarget(mViewportWidth, mViewportHeight);

		// --- 创建场景视口 ---
		Elaine::ViewportDesc Desc;
		Desc.ViewName = "EditorSceneViewport";
		mSceneViewport = mEngine->CreateViewport(Desc);
		mSceneViewport->SetTarget(mOffscreenRT);
		mSceneViewport->SetWorld(mDefaultWorld);
		mSceneViewport->SetCamera(mDefaultCamera);
        auto* View = mSceneViewport->GetRenderView();
        ENQUEUE_RENDER_COMMAND(PauseUninitializedViewport)([View](Elaine::RenderContext&) { View->mbActive = false; });
		mEngine->RegisterViewport(mSceneViewport);
	}

    void EditorGlobalContext::InitializeViewportTransport(ID3D11Device* Device, ID3D11DeviceContext* Context,
        ViewportInteropMode Mode, ViewportInteropFault Fault)
    {
        if (Elaine::RenderDocCapture::GetStatus().VulkanReady)
        {
            if (Mode == ViewportInteropMode::Auto)
            {
                Mode = ViewportInteropMode::Readback;
                LOG_INFO("RenderDoc enabled: auto viewport transport uses readback for self-contained Vulkan replay");
            }
            else if (Mode == ViewportInteropMode::Shared)
                LOG_WARN("RenderDoc with explicit shared viewport: external memory/keyed mutex capture and replay require separate validation");
        }
        mDX11Device = Device; mDX11Context = Context; mInteropMode = Mode;
        if (Mode != ViewportInteropMode::Readback && Elaine::Root::instance()->GetRHIType() == Elaine::Vulkan)
        {
            auto Interop = std::make_unique<EditorViewportInterop>();
            std::string Error;
            if (Interop->Initialize(Device, Context, mViewportWidth, mViewportHeight, Error, Fault))
            { mInterop = std::move(Interop); return; }
            LOG_WARN("Viewport shared texture unavailable: {}", Error);
            if (Mode == ViewportInteropMode::Shared) throw std::runtime_error("Forced shared viewport failed: " + Error);
        }
        else if (Mode == ViewportInteropMode::Shared) throw std::runtime_error("Shared viewport requires Vulkan");
        InitStagingFallback(Device);
        if (!mbStagingReady) throw std::runtime_error("Viewport readback initialization failed");
    }

    void EditorGlobalContext::PrepareViewportFrame()
    {
        if (!mInterop) return;
        mInterop->BeginFrame();
        auto Frame = mInterop->ReserveFrame();
        auto* Target = mOffscreenRT;
        auto* View = mSceneViewport->GetRenderView();
        ENQUEUE_RENDER_COMMAND(SelectSharedViewportFrame)([Target, View, Frame](Elaine::RenderContext&) {
            if (View->mSubmissionEvent) View->mSubmissionEvent->Cancel();
            View->mSubmissionEvent = Frame.Completion; View->mbActive = static_cast<bool>(Frame);
            Target->SetColorTarget(0, Frame.Texture.get());
            View->mOnFrameCaptured = Frame ? std::function<void(const Elaine::CameraState&)>([Data = Frame.ViewData](const Elaine::CameraState& Camera) {
                Data->Camera = Camera; Data->Matrices = Elaine::BuildCameraMatrices(Camera); Data->Valid = true;
            }) : std::function<void(const Elaine::CameraState&)>();
            if (Frame) VulkanRHI::SetDX11TextureSubmission(Frame.Texture, Frame.Submission);
        });
    }

    void EditorGlobalContext::EndViewportUIFrame()
    { if (mInterop) mInterop->EndUIFrame(); }

    bool EditorGlobalContext::RecreateViewportTransport(uint32_t Width, uint32_t Height)
    {
        auto* View = mSceneViewport->GetRenderView(); auto* Target = mOffscreenRT;
        ENQUEUE_RENDER_COMMAND(PauseViewportForResize)([View, Target](Elaine::RenderContext&) {
            View->mbActive = false;
            if (View->mSubmissionEvent) { View->mSubmissionEvent->Cancel(); View->mSubmissionEvent.reset(); }
            Target->SetColorTarget(0, nullptr);
        });
        Elaine::Root::instance()->FlushRenderCommands();
        mViewportWidth = Width; mViewportHeight = Height;
        if (mInterop)
        {
            std::string Error;
            if (mInterop->Recreate(Width, Height, Error))
            { Target->SetSize(Width, Height); if (Width && Height) mSceneViewport->OnTargetResized(Width, Height); return true; }
            LOG_ERROR("Shared viewport recreation failed: {}", Error);
            if (mInteropMode == ViewportInteropMode::Shared) throw std::runtime_error(Error);
            mInterop->Shutdown(); mInterop.reset();
        }
        if (!Width || !Height) return true;
        if (mStagingSRV) { mStagingSRV->Release(); mStagingSRV = nullptr; }
        if (mStagingD3D11Texture) { mStagingD3D11Texture->Release(); mStagingD3D11Texture = nullptr; }
        for (auto& Texture : mOffscreenOwners) Texture.reset();
        Target->SetSize(Width, Height); mSceneViewport->OnTargetResized(Width, Height);
        InitStagingFallback(mDX11Device); return mbStagingReady;
    }

	void EditorGlobalContext::InitStagingFallback(ID3D11Device* InDevice)
	{
		if (!InDevice)
			return;

        mbStagingReady = false;
        mReadback = std::make_shared<ReadbackPacket>();
        mReadbackHistory = std::make_shared<ReadbackFrameHistory>();
        mDisplayedReadbackFrame = {};
		LOG_INFO("EditorGlobalContext: Initializing staging buffer fallback (double-buffered readback).");
        auto Textures = std::make_shared<std::array<Elaine::RHITextureHandle, NUM_OFFSCREEN_BUFFERS>>();
        const auto Width = mViewportWidth, Height = mViewportHeight;
        auto* Target = mOffscreenRT; auto* View = mSceneViewport->GetRenderView();
        ENQUEUE_RENDER_COMMAND(CreateReadbackTargets)([Textures, Width, Height, Target, View, History = mReadbackHistory](Elaine::RenderContext&) {
            for (auto& Texture : *Textures)
            {
                Elaine::RHITextureDesc Desc;
                Desc.mExtent.x = static_cast<float>(Width); Desc.mExtent.y = static_cast<float>(Height);
                Desc.mFormat = Elaine::PF_R8G8B8A8; Desc.mNumMips = 1; Desc.mNumSamples = 1;
                Desc.mDimension = Elaine::TextureDimension::Texture2D; Desc.mArraySize = 1;
                Desc.mFlags = Elaine::TextureCreateFlags::RenderTargetable | Elaine::TextureCreateFlags::ShaderResource;
                Texture = Elaine::GetDynamicRHI()->GetDefaultCommandContext()->RHICreateTexture(Desc, nullptr);
            }
            Target->SetColorTarget(0, (*Textures)[0].get()); View->mbActive = true;
            View->mOnFrameCaptured = [History](const Elaine::CameraState& Camera) {
                auto& Frame = History->Rendered[0]; Frame.Camera = Camera;
                Frame.Matrices = Elaine::BuildCameraMatrices(Camera); Frame.FrameNumber = History->NextFrame++; Frame.Valid = true;
            };
        });
        Elaine::Root::instance()->FlushRenderCommands();
        mWriteIndex = 0;
        for (uint32_t Index = 0; Index < NUM_OFFSCREEN_BUFFERS; ++Index) mOffscreenOwners[Index] = (*Textures)[Index];

        for (auto Texture : mOffscreenOwners)
            ENQUEUE_RENDER_COMMAND(InitViewportReadback)([Texture](Elaine::RenderContext&) { Texture->InitReadbackResources(); });

		// 创建 D3D11 USAGE_DEFAULT 纹理用于 ImGui 显示
		D3D11_TEXTURE2D_DESC TexDesc = {};
		TexDesc.Width = mViewportWidth;
		TexDesc.Height = mViewportHeight;
		TexDesc.MipLevels = 1;
		TexDesc.ArraySize = 1;
		TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TexDesc.SampleDesc.Count = 1;
		TexDesc.Usage = D3D11_USAGE_DEFAULT;
		TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = InDevice->CreateTexture2D(&TexDesc, nullptr, &mStagingD3D11Texture);
		if (FAILED(hr))
		{
			LOG_ERROR("EditorGlobalContext: Failed to create D3D11 staging texture, HRESULT: 0x{:08x}", (unsigned)hr);
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
		SrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Texture2D.MipLevels = 1;
		SrvDesc.Texture2D.MostDetailedMip = 0;
		hr = InDevice->CreateShaderResourceView(mStagingD3D11Texture, &SrvDesc, &mStagingSRV);
		if (FAILED(hr))
		{
			LOG_ERROR("EditorGlobalContext: Failed to create D3D11 staging SRV, HRESULT: 0x{:08x}", (unsigned)hr);
			return;
		}

		// 分配 CPU 像素缓冲区
		mPixelBuffer.resize(mViewportWidth * mViewportHeight * 4, 0);
		mbStagingReady = true;
		mbFirstStagingFrame = true;
	}

    void EditorGlobalContext::EnqueueCopyToReadbackBuffer()
    {
        ++mReadbackCount;
        auto Texture = mOffscreenOwners[mWriteIndex]; auto Packet = mReadback;
        if (!Texture) return;
        { std::lock_guard<std::mutex> Lock(Packet->Mutex); if (Packet->Pending) return; Packet->Pending = true; }
        // Runs after the previous RHI batch in the RenderThread synchronization window.
        ENQUEUE_RENDER_COMMAND(ReadbackViewport)([Texture, Packet, Index = mWriteIndex, History = mReadbackHistory](Elaine::RenderContext&) {
                std::vector<uint8_t> Pixels(Texture->GetWidth() * Texture->GetHeight() * 4); uint32_t Pitch = 0;
                const bool Ready = Texture->ReadbackPixels(Pixels.data(), Pitch);
                const auto CompletedFrame = History->Copied[Index];
                if ((!History->Copied[Index].Valid || Ready) && History->Rendered[Index].Valid)
                {
                    Texture->CopyToReadbackBuffer();
                    History->Copied[Index] = History->Rendered[Index];
                }
                std::lock_guard<std::mutex> Lock(Packet->Mutex);
                if (Ready && CompletedFrame.Valid && CompletedFrame.FrameNumber > Packet->Frame.FrameNumber)
                { Packet->Pixels = std::move(Pixels); Packet->Pitch = Pitch; Packet->Frame = CompletedFrame; Packet->Ready = true; }
                Packet->Pending = false;
        });
    }
    void EditorGlobalContext::UpdateDX11TextureFromReadback(ID3D11DeviceContext* Context)
    {
        if (!Context || !mStagingD3D11Texture) return;
        std::lock_guard<std::mutex> Lock(mReadback->Mutex);
        if (!mReadback->Ready) return;
        Context->UpdateSubresource(mStagingD3D11Texture, 0, nullptr, mReadback->Pixels.data(), mReadback->Pitch, 0);
        mDisplayedReadbackFrame = mReadback->Frame;
        mReadback->Ready = false;
    }

	void EditorGlobalContext::SwapOffscreenBuffers()
	{
		mWriteIndex = 1 - mWriteIndex;

		// 切换 OffscreenRT 使用的 color target
		if (mOffscreenRT && mOffscreenOwners[mWriteIndex].get())
		{
            auto Texture = mOffscreenOwners[mWriteIndex]; auto* Target = mOffscreenRT;
            ENQUEUE_RENDER_COMMAND(SwapReadbackTarget)([Target, Texture, View = mSceneViewport->GetRenderView(), Index = mWriteIndex, History = mReadbackHistory](Elaine::RenderContext&) {
                Target->SetColorTarget(0, Texture.get());
                View->mOnFrameCaptured = [History, Index](const Elaine::CameraState& Camera) {
                    auto& Frame = History->Rendered[Index]; Frame.Camera = Camera;
                    Frame.Matrices = Elaine::BuildCameraMatrices(Camera); Frame.FrameNumber = History->NextFrame++; Frame.Valid = true;
                };
            });
		}
	}

    ViewportFrameData EditorGlobalContext::GetDisplayedViewportFrame() const
    { return mInterop ? mInterop->GetDisplayViewData() : mDisplayedReadbackFrame; }

	ID3D11ShaderResourceView* EditorGlobalContext::GetViewportSRV() const
	{
		// 优先使用共享内存路径
        if (mInterop) return mInterop->GetDisplaySRV();

		// 其次使用 staging fallback 路径
		if (mStagingSRV)
			return mStagingSRV;

		return nullptr;
	}
}
