#pragma once
#include "ElaineSingleton.h"
#include "ElaineEditorGridManager.h"
#include <d3d11.h>

namespace Elaine
{
	class ElaineEngine;
	class Viewport;
	class OffscreenRenderTarget;
	class World;
	class GameObject;
	class CameraComponent;
	class RHITexture;
}

namespace Editor
{
	class EditorGlobalContext : public Elaine::Singleton<EditorGlobalContext>
	{
	public:
		EditorGlobalContext();
		~EditorGlobalContext();
		void Initialize(Elaine::ElaineEngine* InEngine);
		void Destroy();

		//=====================================================================
		// 默认场景
		//=====================================================================
		void CreateDefaultScene();

		//=====================================================================
		// 场景选择状态
		//=====================================================================
		void SetSelectedGameObject(Elaine::GameObject* obj) { mSelectedGameObject = obj; }
		Elaine::GameObject* GetSelectedGameObject() const { return mSelectedGameObject; }

		void SetActiveWorld(Elaine::World* world) { mActiveWorld = world; }
		Elaine::World* GetActiveWorld() const { return mActiveWorld; }

		//=====================================================================
		// 离屏渲染视口（双缓冲，类似 Swapchain 2 张 image）
		//=====================================================================
		Elaine::Viewport* GetSceneViewport() const { return mSceneViewport; }
		Elaine::OffscreenRenderTarget* GetOffscreenRT() const { return mOffscreenRT; }
		Elaine::RHITexture* GetOffscreenColorTexture() const { return mOffscreenColorTextures[mWriteIndex]; }

		//=====================================================================
		// DX11 共享纹理 (共享内存路径)
		//=====================================================================
		ID3D11ShaderResourceView* GetSharedSRV() const { return mSharedSRV; }
		void SetupDX11SharedTexture(ID3D11Device* InDevice, void* InSharedHandle, uint32_t InWidth, uint32_t InHeight);

		//=====================================================================
		// Staging Buffer Fallback (CPU readback 路径)
		//=====================================================================
		bool TrySetupSharedMemory(ID3D11Device* InDevice);
		void InitStagingFallback(ID3D11Device* InDevice);
		void EnqueueCopyToReadbackBuffer();
		void UpdateDX11TextureFromReadback(ID3D11DeviceContext* InCtx);
		void SwapOffscreenBuffers();
		ID3D11ShaderResourceView* GetViewportSRV() const;

		bool IsUsingSharedMemory() const { return !mbSharedMemoryFailed && mSharedSRV != nullptr; }
		bool IsStagingReady() const { return mbStagingReady; }

		uint32_t GetViewportWidth() const { return mViewportWidth; }
		uint32_t GetViewportHeight() const { return mViewportHeight; }

		// 允许 ElaineEditor::Tick() 访问
		bool mbSharedMemoryAttempted = false;

	private:
		Elaine::ElaineEngine*           mEngine = nullptr;

		// 场景
		Elaine::World*                  mActiveWorld = nullptr;
		Elaine::World*                  mDefaultWorld = nullptr;
		Elaine::CameraComponent*        mDefaultCamera = nullptr;
		Elaine::GameObject*             mSelectedGameObject = nullptr;

		// 离屏渲染（双缓冲）
		static constexpr uint32_t       NUM_OFFSCREEN_BUFFERS = 2;
		Elaine::Viewport*               mSceneViewport = nullptr;
		Elaine::OffscreenRenderTarget*   mOffscreenRT = nullptr;
		Elaine::RHITexture*              mOffscreenColorTextures[NUM_OFFSCREEN_BUFFERS] = {};
		uint32_t                         mWriteIndex = 0;
		uint32_t                         mViewportWidth = 1280;
		uint32_t                         mViewportHeight = 720;

		// DX11 共享内存路径
		ID3D11Texture2D*                 mSharedTexture = nullptr;
		ID3D11ShaderResourceView*        mSharedSRV = nullptr;

		// Staging fallback 路径
		bool                             mbSharedMemoryFailed = false;
		bool                             mbStagingReady = false;
		bool                             mbFirstStagingFrame = true;
		ID3D11Texture2D*                 mStagingD3D11Texture = nullptr;
		ID3D11ShaderResourceView*        mStagingSRV = nullptr;
		std::vector<uint8_t>             mPixelBuffer;

		// Editor Grid (infinite ground plane)
		EditorGridManager                mGridManager;
	};
}