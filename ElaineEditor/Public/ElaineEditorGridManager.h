#pragma once

namespace Elaine
{
	class World;
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

	private:
		Elaine::MaterialInstanceDynamic* mMaterial = nullptr;
		bool mInitialized = false;
	};
}
