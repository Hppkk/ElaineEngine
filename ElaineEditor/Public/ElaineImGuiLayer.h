#pragma once
#include <d3d11.h>
#include <Windows.h>
#include <string>

namespace Editor
{
	// ============================================================
	// ImGuiLayer — DX11 ImGui lifecycle manager (uses external HWND)
	// ============================================================
	class ImGuiLayer
	{
	public:
		ImGuiLayer() = default;
		~ImGuiLayer();

		// Initialize with an existing HWND (window created externally)
		bool Initialize(HWND hwnd, int width, int height);
		void Shutdown();

		// Call this when the window resizes
		void OnResize(int width, int height);

		// Frame lifecycle
		void BeginFrame();
		void EndFrame();     // Render + Present

		// Accessors
		HWND GetHWND() const { return mHwnd; }
		ID3D11Device* GetDevice() const { return mDevice; }
		ID3D11DeviceContext* GetDeviceContext() const { return mDeviceContext; }
		int GetWidth() const { return mWidth; }
		int GetHeight() const { return mHeight; }

		// Create a DX11 texture from raw RGBA pixel data (for viewport display)
		ID3D11ShaderResourceView* CreateTextureFromPixels(
			const void* pixels, int width, int height);
		void UpdateTextureFromPixels(
			ID3D11ShaderResourceView* srv, const void* pixels, int width, int height);

	private:
		bool CreateDeviceD3D(HWND hwnd);
		void CleanupDeviceD3D();
		void CreateRenderTarget();
		void CleanupRenderTarget();
		void SetupImGuiStyle();

		HWND                    mHwnd = nullptr;
		ID3D11Device*           mDevice = nullptr;
		ID3D11DeviceContext*    mDeviceContext = nullptr;
		IDXGISwapChain*         mSwapChain = nullptr;
		ID3D11RenderTargetView* mMainRenderTargetView = nullptr;
		int                     mWidth = 1600;
		int                     mHeight = 900;
		bool                    mInitialized = false;
	};
}
