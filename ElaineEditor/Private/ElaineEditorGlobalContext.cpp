#include "ElaineEditorGlobalContext.h"
#include "ElaineEngine.h"
#include "ElaineViewport.h"
#include "ElaineOffscreenRenderTarget.h"
#include "ElaineWorld.h"
#include "ElaineGameObject.h"
#include "ElaineCameraComponent.h"
#include "ElaineSkyComponent.h"
#include "ElaineMeshComponent.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineRenderContext.h"
#include "ElaineRenderSystem.h"
#include "ElaineLogSystem.h"
#include "d3d11_1.h"

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
	}

	void EditorGlobalContext::Destroy()
	{
		// DX11 共享内存路径
		if (mSharedSRV)
		{
			mSharedSRV->Release();
			mSharedSRV = nullptr;
		}
		if (mSharedTexture)
		{
			mSharedTexture->Release();
			mSharedTexture = nullptr;
		}

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

		if (mSceneViewport && mEngine)
		{
			mEngine->UnregisterViewport(mSceneViewport);
			mEngine->DestroyViewport(mSceneViewport);
			mSceneViewport = nullptr;
		}

		delete mOffscreenRT;
		mOffscreenRT = nullptr;

		mOffscreenColorTextures[0] = nullptr;
		mOffscreenColorTextures[1] = nullptr;
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
		Elaine::GameObject* CameraObj = mDefaultWorld->CreateGameObject();
		mDefaultCamera = CameraObj->AddComponentType<Elaine::CameraComponent>("CameraComponent");

		// Sky
		Elaine::GameObject* SkyObj = mDefaultWorld->CreateGameObject();
		SkyObj->AddComponentType<Elaine::SkyComponent>("SkyComponent");

		// Test Mesh
		Elaine::GameObject* MeshObj = mDefaultWorld->CreateGameObject();
		Elaine::StaticMeshComponent* MeshComp = MeshObj->AddComponentType<Elaine::StaticMeshComponent>("StaticMeshComponent");
		MeshComp->ChangeMesh("");

		// --- 创建离屏渲染目标 ---
		mOffscreenRT = new Elaine::OffscreenRenderTarget(mViewportWidth, mViewportHeight);

		// 在渲染线程上创建 2 个带 External 标志的纹理（双缓冲，类似 Swapchain）
		Elaine::OffscreenRenderTarget* OffscreenRT = mOffscreenRT;
		Elaine::RHITexture** ColorTexPtrs = mOffscreenColorTextures;
		uint32_t W = mViewportWidth;
		uint32_t H = mViewportHeight;

		ENQUEUE_RENDER_COMMAND(CreateOffscreenColorTargets)([OffscreenRT, ColorTexPtrs, W, H](Elaine::RenderContext& InContext)
		{
			Elaine::TextureCreateFlags Flags =
				Elaine::TextureCreateFlags::External |
				Elaine::TextureCreateFlags::RenderTargetable |
				Elaine::TextureCreateFlags::ShaderResource;

			for (uint32_t i = 0; i < NUM_OFFSCREEN_BUFFERS; ++i)
			{
				Elaine::RHITexture* ColorTex = Elaine::RenderSystem::instance()->GetRHICommandContext()->RHICreateTexture2D(
					W, H,
					(uint8)Elaine::PF_R8G8B8A8,
					1, 1,
					Flags,
					Elaine::ERHIAccess::RTV,
					nullptr);

				ColorTexPtrs[i] = ColorTex;
			}

			// 初始使用第 0 个纹理作为渲染目标
			OffscreenRT->SetColorTarget(0, ColorTexPtrs[0]);
		});

		// --- 创建场景视口 ---
		Elaine::ViewportDesc Desc;
		Desc.ViewName = "EditorSceneViewport";
		mSceneViewport = mEngine->CreateViewport(Desc);
		mSceneViewport->SetTarget(mOffscreenRT);
		mSceneViewport->SetWorld(mDefaultWorld);
		mSceneViewport->SetCamera(mDefaultCamera);
		mEngine->RegisterViewport(mSceneViewport);
	}

	void EditorGlobalContext::SetupDX11SharedTexture(ID3D11Device* InDevice, void* InSharedHandle, uint32_t InWidth, uint32_t InHeight)
	{
		if (!InDevice || !InSharedHandle)
			return;

		// 清理旧资源
		if (mSharedSRV)
		{
			mSharedSRV->Release();
			mSharedSRV = nullptr;
		}
		if (mSharedTexture)
		{
			mSharedTexture->Release();
			mSharedTexture = nullptr;
		}

		// 通过 ID3D11Device1 打开 Vulkan 导出的共享内存
		ID3D11Device1* Device1 = nullptr;
		HRESULT hr = InDevice->QueryInterface(__uuidof(ID3D11Device1), (void**)&Device1);
		if (SUCCEEDED(hr) && Device1)
		{
			hr = Device1->OpenSharedResource1((HANDLE)InSharedHandle, __uuidof(ID3D11Texture2D), (void**)&mSharedTexture);
			if (FAILED(hr))
			{
				LOG_WARN("EditorGlobalContext: OpenSharedResource1 failed with HRESULT: 0x{:08x}, trying OpenSharedResource...", (unsigned)hr);
			}
			Device1->Release();
		}

		if (FAILED(hr) || !mSharedTexture)
		{
			hr = InDevice->OpenSharedResource((HANDLE)InSharedHandle, __uuidof(ID3D11Texture2D), (void**)&mSharedTexture);
			if (FAILED(hr))
			{
				LOG_WARN("EditorGlobalContext: OpenSharedResource also failed with HRESULT: 0x{:08x}", (unsigned)hr);
			}
		}

		if (SUCCEEDED(hr) && mSharedTexture)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
			SrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			SrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			SrvDesc.Texture2D.MipLevels = 1;
			SrvDesc.Texture2D.MostDetailedMip = 0;
			InDevice->CreateShaderResourceView(mSharedTexture, &SrvDesc, &mSharedSRV);
		}
	}

	bool EditorGlobalContext::TrySetupSharedMemory(ID3D11Device* InDevice)
	{
		mbSharedMemoryAttempted = true;

		// 使用第一个纹理尝试共享内存
		Elaine::RHITexture* ColorTex = mOffscreenColorTextures[0];
		if (!ColorTex)
			return false;

		void* SharedHandle = ColorTex->GetSharedMemoryHandle();
		if (!SharedHandle)
		{
			LOG_WARN("EditorGlobalContext: GetSharedMemoryHandle() returned null, falling back to staging buffer.");
			mbSharedMemoryFailed = true;
			return false;
		}

		SetupDX11SharedTexture(InDevice, SharedHandle, mViewportWidth, mViewportHeight);
		if (!mSharedSRV)
		{
			LOG_WARN("EditorGlobalContext: SetupDX11SharedTexture failed, falling back to staging buffer.");
			mbSharedMemoryFailed = true;
			return false;
		}

		LOG_INFO("EditorGlobalContext: Shared memory path established successfully.");
		return true;
	}

	void EditorGlobalContext::InitStagingFallback(ID3D11Device* InDevice)
	{
		if (!InDevice)
			return;

		LOG_INFO("EditorGlobalContext: Initializing staging buffer fallback (double-buffered readback).");

		// 在渲染线程初始化两个纹理的 readback 资源
		Elaine::RHITexture** ColorTexPtrs = mOffscreenColorTextures;

		ENQUEUE_RENDER_COMMAND(InitReadbackResources)([ColorTexPtrs](Elaine::RenderContext& InContext)
		{
			for (uint32_t i = 0; i < NUM_OFFSCREEN_BUFFERS; ++i)
			{
				if (ColorTexPtrs[i])
				{
					ColorTexPtrs[i]->InitReadbackResources();
				}
			}
		});

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
		Elaine::RHITexture* WriteTexture = mOffscreenColorTextures[mWriteIndex];
		if (!WriteTexture)
			return;

		ENQUEUE_RENDER_COMMAND(CopyToReadbackBuffer)([WriteTexture](Elaine::RenderContext& InContext)
		{
			WriteTexture->CopyToReadbackBuffer();
		});
	}

	void EditorGlobalContext::UpdateDX11TextureFromReadback(ID3D11DeviceContext* InCtx)
	{
		if (!InCtx || !mStagingD3D11Texture)
			return;

		// 第一帧 readIndex 的 buffer 还没有有效数据
		if (mbFirstStagingFrame)
		{
			mbFirstStagingFrame = false;
			return;
		}

		// 读取上一帧写入的纹理（readIdx = 1 - writeIdx）
		uint32_t ReadIndex = 1 - mWriteIndex;
		Elaine::RHITexture* ReadTexture = mOffscreenColorTextures[ReadIndex];
		if (!ReadTexture)
			return;

		uint32_t RowPitch = 0;
		if (ReadTexture->ReadbackPixels(mPixelBuffer.data(), RowPitch))
		{
			// 更新 D3D11 纹理
			InCtx->UpdateSubresource(mStagingD3D11Texture, 0, nullptr,
				mPixelBuffer.data(), RowPitch, 0);
		}
	}

	void EditorGlobalContext::SwapOffscreenBuffers()
	{
		mWriteIndex = 1 - mWriteIndex;

		// 切换 OffscreenRT 使用的 color target
		if (mOffscreenRT && mOffscreenColorTextures[mWriteIndex])
		{
			mOffscreenRT->SetColorTarget(0, mOffscreenColorTextures[mWriteIndex]);
		}
	}

	ID3D11ShaderResourceView* EditorGlobalContext::GetViewportSRV() const
	{
		// 优先使用共享内存路径
		if (mSharedSRV)
			return mSharedSRV;

		// 其次使用 staging fallback 路径
		if (mStagingSRV)
			return mStagingSRV;

		return nullptr;
	}
}