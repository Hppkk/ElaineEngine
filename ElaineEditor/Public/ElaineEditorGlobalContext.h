#pragma once
#include "ElaineSingleton.h"
#include "ElaineEditorGridManager.h"
#include "ElaineGizmoManager.h"
#include <d3d11.h>
#include "render/common/ElaineRHICommandContext.h"
#include <mutex>
#include "ElaineEditorViewportInterop.h"

namespace Elaine
{
	class ElaineEngine;
	class Viewport;
	class OffscreenRenderTarget;
	class World;
	class Actor;
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
		void SetSelectedActor(Elaine::Actor* obj) { mSelectedActor = obj; }
		Elaine::Actor* GetSelectedActor() const { return mSelectedActor; }

		void SetActiveWorld(Elaine::World* world) { mActiveWorld = world; }
		Elaine::World* GetActiveWorld() const { return mActiveWorld; }

		//=====================================================================
		// 离屏渲染视口（共享三缓冲，回读双缓冲）
		//=====================================================================
		Elaine::Viewport* GetSceneViewport() const { return mSceneViewport; }
		Elaine::OffscreenRenderTarget* GetOffscreenRT() const { return mOffscreenRT; }
        // Readback path only; shared targets are leased through PrepareViewportFrame.
		Elaine::RHITexture* GetOffscreenColorTexture() const { return mOffscreenOwners[mWriteIndex].get(); }
		EditorGizmoManager& GetGizmoManager() { return mGizmoManager; }
		EditorGridManager& GetGridManager() { return mGridManager; }

		//=====================================================================
		// DX11 共享纹理 (共享内存路径)
		//=====================================================================
        void InitializeViewportTransport(ID3D11Device* Device, ID3D11DeviceContext* Context, ViewportInteropMode Mode, ViewportInteropFault Fault);
        void PrepareViewportFrame();
        void EndViewportUIFrame();
        bool RecreateViewportTransport(uint32_t Width, uint32_t Height);
        const EditorViewportInterop::Statistics* GetInteropStatistics() const { return mInterop ? &mInterop->GetStatistics() : nullptr; }
        uint64_t GetReadbackCount() const { return mReadbackCount; }

		//=====================================================================
		// Staging Buffer Fallback (CPU readback 路径)
		//=====================================================================
		void InitStagingFallback(ID3D11Device* InDevice);
		void EnqueueCopyToReadbackBuffer();
		void UpdateDX11TextureFromReadback(ID3D11DeviceContext* InCtx);
		void SwapOffscreenBuffers();
        ID3D11ShaderResourceView* GetViewportSRV() const;
        ViewportFrameData GetDisplayedViewportFrame() const;


		bool IsUsingSharedMemory() const { return mInterop != nullptr; }
		bool IsStagingReady() const { return mbStagingReady; }

		uint32_t GetViewportWidth() const { return mViewportWidth; }
		uint32_t GetViewportHeight() const { return mViewportHeight; }


	private:
		Elaine::ElaineEngine*           mEngine = nullptr;

		// 场景
		Elaine::World*                  mActiveWorld = nullptr;
		Elaine::World*                  mDefaultWorld = nullptr;
		Elaine::CameraComponent*        mDefaultCamera = nullptr;
		Elaine::Actor*             mSelectedActor = nullptr;

		// 离屏渲染（双缓冲）
		static constexpr uint32_t       NUM_OFFSCREEN_BUFFERS = 2;
		Elaine::Viewport*               mSceneViewport = nullptr;
		Elaine::OffscreenRenderTarget*   mOffscreenRT = nullptr;
        Elaine::RHITextureHandle mOffscreenOwners[NUM_OFFSCREEN_BUFFERS];
        struct ReadbackPacket { std::mutex Mutex; std::vector<uint8_t> Pixels; uint32_t Pitch = 0; bool Ready = false; bool Pending = false; ViewportFrameData Frame; };
        struct ReadbackFrameHistory
        {
            // RenderThread owns history; publication is through ReadbackPacket::Mutex.
            std::array<ViewportFrameData, NUM_OFFSCREEN_BUFFERS> Rendered, Copied;
            uint64_t NextFrame = 1;
        };
        std::shared_ptr<ReadbackFrameHistory> mReadbackHistory = std::make_shared<ReadbackFrameHistory>();
        ViewportFrameData mDisplayedReadbackFrame;
        std::shared_ptr<ReadbackPacket> mReadback = std::make_shared<ReadbackPacket>();
		uint32_t                         mWriteIndex = 0;
		uint32_t                         mViewportWidth = 1280;
		uint32_t                         mViewportHeight = 720;

		// DX11 共享内存路径
        std::unique_ptr<EditorViewportInterop> mInterop;
        ID3D11Device* mDX11Device = nullptr;
        ID3D11DeviceContext* mDX11Context = nullptr;
        ViewportInteropMode mInteropMode = ViewportInteropMode::Auto;
        uint64_t mReadbackCount = 0;

		// Staging fallback 路径
		bool                             mbStagingReady = false;
		bool                             mbFirstStagingFrame = true;
		ID3D11Texture2D*                 mStagingD3D11Texture = nullptr;
		ID3D11ShaderResourceView*        mStagingSRV = nullptr;
		std::vector<uint8_t>             mPixelBuffer;

		// Editor Grid (infinite ground plane)
		EditorGridManager                mGridManager;
		EditorGizmoManager               mGizmoManager;
	};
}
