#include "ElaineImGuiLayer.h"
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx11.h"
#include "imgui/ImGuizmo/ImGuizmo.h"
#include <d3d11.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32.lib")

// Forward declare Win32 message handler from imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Editor
{
	// Subclass ID for our ImGui hook
	static const UINT_PTR IMGUI_SUBCLASS_ID = 1001;

	// ============================================================
	// SubclassProc — intercepts messages before PlatformWindow's WndProc
	// ============================================================
	static LRESULT CALLBACK ImGuiSubclassProc(
		HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
		UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
	{
		// Let ImGui handle the message first
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
			return true;

		// Handle resize for DX11 swapchain
		ImGuiLayer* layer = reinterpret_cast<ImGuiLayer*>(dwRefData);
		if (msg == WM_SIZE && wParam != SIZE_MINIMIZED && layer)
		{
			layer->OnResize((int)LOWORD(lParam), (int)HIWORD(lParam));
		}

		// Pass to original WndProc
		return DefSubclassProc(hWnd, msg, wParam, lParam);
	}

	ImGuiLayer::~ImGuiLayer()
	{
		Shutdown();
	}

	// ============================================================
	// CreateDeviceD3D
	// ============================================================
	bool ImGuiLayer::CreateDeviceD3D(HWND hwnd)
	{
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hwnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0,
		};

		HRESULT hr = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			createDeviceFlags, featureLevelArray, 2,
			D3D11_SDK_VERSION, &sd,
			&mSwapChain, &mDevice, &featureLevel, &mDeviceContext);

		if (FAILED(hr))
			return false;

		CreateRenderTarget();
		return true;
	}

	void ImGuiLayer::CreateRenderTarget()
	{
		ID3D11Texture2D* backBuffer = nullptr;
		mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
		if (backBuffer)
		{
			mDevice->CreateRenderTargetView(backBuffer, nullptr, &mMainRenderTargetView);
			backBuffer->Release();
		}
	}

	void ImGuiLayer::CleanupRenderTarget()
	{
		if (mMainRenderTargetView) { mMainRenderTargetView->Release(); mMainRenderTargetView = nullptr; }
	}

	void ImGuiLayer::CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if (mSwapChain)      { mSwapChain->Release();      mSwapChain = nullptr; }
		if (mDeviceContext)  { mDeviceContext->Release();   mDeviceContext = nullptr; }
		if (mDevice)         { mDevice->Release();          mDevice = nullptr; }
	}

	// ============================================================
	// SetupImGuiStyle
	// ============================================================
	void ImGuiLayer::SetupImGuiStyle()
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* colors = style.Colors;

		ImGui::StyleColorsDark();

		colors[ImGuiCol_WindowBg]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ChildBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_PopupBg]            = ImVec4(0.12f, 0.12f, 0.12f, 0.94f);
		colors[ImGuiCol_Border]             = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
		colors[ImGuiCol_FrameBg]            = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBg]            = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_MenuBarBg]          = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_Header]             = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_HeaderHovered]      = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_HeaderActive]       = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
		colors[ImGuiCol_Tab]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
		colors[ImGuiCol_TabHovered]         = ImVec4(0.28f, 0.56f, 0.90f, 0.80f);
		colors[ImGuiCol_TabActive]          = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
		colors[ImGuiCol_DockingPreview]     = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
		colors[ImGuiCol_DockingEmptyBg]     = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
		colors[ImGuiCol_Button]             = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
		colors[ImGuiCol_ButtonHovered]      = ImVec4(0.28f, 0.56f, 0.90f, 1.00f);
		colors[ImGuiCol_ButtonActive]       = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
		colors[ImGuiCol_Separator]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

		style.WindowRounding    = 4.0f;
		style.FrameRounding     = 3.0f;
		style.GrabRounding      = 3.0f;
		style.TabRounding       = 4.0f;
		style.ScrollbarRounding = 6.0f;
		style.WindowBorderSize  = 1.0f;
		style.FrameBorderSize   = 0.0f;
		style.PopupBorderSize   = 1.0f;
		style.WindowPadding     = ImVec2(8.0f, 8.0f);
		style.FramePadding      = ImVec2(6.0f, 4.0f);
		style.ItemSpacing       = ImVec2(8.0f, 4.0f);
		style.IndentSpacing     = 20.0f;
	}

	// ============================================================
	// Initialize
	// ============================================================
	bool ImGuiLayer::Initialize(HWND hwnd, int width, int height)
	{
		mHwnd = hwnd;
		mWidth = width;
		mHeight = height;

		if (!CreateDeviceD3D(hwnd))
			return false;

		// Install subclass to intercept Win32 messages for ImGui
		SetWindowSubclass(hwnd, ImGuiSubclassProc, IMGUI_SUBCLASS_ID, reinterpret_cast<DWORD_PTR>(this));

		// Setup ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		SetupImGuiStyle();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX11_Init(mDevice, mDeviceContext);

		mInitialized = true;
		return true;
	}

	void ImGuiLayer::Shutdown()
	{
		if (!mInitialized) return;
		mInitialized = false;

		// Remove subclass
		if (mHwnd)
			RemoveWindowSubclass(mHwnd, ImGuiSubclassProc, IMGUI_SUBCLASS_ID);

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		CleanupDeviceD3D();
	}

	// ============================================================
	// OnResize
	// ============================================================
	void ImGuiLayer::OnResize(int width, int height)
	{
		if (width <= 0 || height <= 0) return;
		mWidth = width;
		mHeight = height;
		CleanupRenderTarget();
		if (mSwapChain)
			mSwapChain->ResizeBuffers(0, (UINT)width, (UINT)height, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget();
	}

	// ============================================================
	// BeginFrame
	// ============================================================
	void ImGuiLayer::BeginFrame()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();
	}

	// ============================================================
	// EndFrame
	// ============================================================
	void ImGuiLayer::EndFrame()
	{
		ImGui::Render();

		const float clearColor[4] = { 0.06f, 0.06f, 0.06f, 1.00f };
		mDeviceContext->OMSetRenderTargets(1, &mMainRenderTargetView, nullptr);
		mDeviceContext->ClearRenderTargetView(mMainRenderTargetView, clearColor);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		ImGuiIO& io = ImGui::GetIO();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		mSwapChain->Present(1, 0);
	}

	// ============================================================
	// Texture helpers
	// ============================================================
	ID3D11ShaderResourceView* ImGuiLayer::CreateTextureFromPixels(
		const void* pixels, int width, int height)
	{
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = width;
		texDesc.Height = height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = pixels;
		initData.SysMemPitch = width * 4;

		ID3D11Texture2D* tex = nullptr;
		mDevice->CreateTexture2D(&texDesc, &initData, &tex);
		if (!tex) return nullptr;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		ID3D11ShaderResourceView* srv = nullptr;
		mDevice->CreateShaderResourceView(tex, &srvDesc, &srv);
		tex->Release();

		return srv;
	}

	void ImGuiLayer::UpdateTextureFromPixels(
		ID3D11ShaderResourceView* srv, const void* pixels, int width, int height)
	{
		if (!srv) return;
		ID3D11Resource* resource = nullptr;
		srv->GetResource(&resource);
		if (resource)
		{
			mDeviceContext->UpdateSubresource(resource, 0, nullptr, pixels, width * 4, 0);
			resource->Release();
		}
	}
}
