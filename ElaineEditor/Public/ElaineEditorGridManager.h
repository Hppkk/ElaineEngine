#pragma once
#include <atomic>
#include <memory>

namespace Elaine
{
	class World;
	class GridRenderProxy;
	class MaterialInstanceDynamic;
}

namespace Editor
{
	// ============================================================
	// EditorGridManager
	// 
	// Creates and owns the editor's infinite ground-plane grid.
	// The grid is rendered via a GridRenderProxy on the render thread,
	// using the grid.material / Grid.mi assets.
	//
	// MaterialInstanceDynamic 由逻辑线程创建和持有。
	// 通过 CreateSnapshot() 生成参数快照，经 ENQUEUE_RENDER_COMMAND
	// 传递给渲染线程的 GridRenderProxy::RenderMaterialProxy。
	//
	// Lifetime: create once during editor scene setup, destroy on teardown.
	// ============================================================
	class EditorGridManager
	{
	public:
		EditorGridManager() = default;
		~EditorGridManager();

		/// Create the grid proxy + material on the render thread.
		/// @param InWorld  – the world whose SceneManager will own the proxy
		void Initialize(Elaine::World* InWorld);

		/// Destroy the grid proxy on the render thread.
		void Shutdown();
		void SetVisible(bool Visible);
		bool IsVisible() const { return mVisible; }

	private:
		struct ProxyState
		{
			std::atomic<Elaine::GridRenderProxy*> Proxy{nullptr};
			std::atomic<bool> Active{true};
			std::atomic<bool> Visible{true};
		};
		Elaine::MaterialInstanceDynamic* mMaterial = nullptr;  // 逻辑线程持有，不传递给渲染线程
		bool mInitialized = false;
		bool mVisible = true;
		Elaine::World* mWorld = nullptr;
		std::shared_ptr<ProxyState> mState;
	};
}
