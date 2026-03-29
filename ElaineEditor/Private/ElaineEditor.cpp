#include "ElaineEditor.h"
#include "ElaineImGuiLayer.h"
#include "ElaineRoot.h"
#include "ElaineSceneHierarchyPanel.h"
#include "ElaineInspectorPanel.h"
#include "ElaineConsolePanel.h"
#include "ElaineViewportPanel.h"
#include "ElaineContentBrowserPanel.h"
#include "ElaineEngine.h"
#include "ElaineWorld.h"
#include "ElaineEditorUI.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineInputSystem.h"
#include "imgui.h"

namespace Editor
{
	ElaineEditor::ElaineEditor(Elaine::ElaineEngine* InEngine)
		: mEngineImpl(InEngine)
	{
	}

	ElaineEditor::~ElaineEditor()
	{
		Destroy();
	}

	bool ElaineEditor::Initialize(Elaine::PlatformWindow* window)
	{
		mMainWindow = window;

		// Get native HWND from PlatformWindow
		HWND hwnd = (HWND)window->getNativeHandle();
		int width = window->getWidth();
		int height = window->getHeight();

		// --- Create ImGui Layer (DX11 on PlatformWindow's HWND) ---
		mImGuiLayer = new ImGuiLayer();
		if (!mImGuiLayer->Initialize(hwnd, width, height))
			return false;

		// --- Initialize EditorGlobalContext ---
		new EditorGlobalContext();
		EditorGlobalContext::instance()->Initialize(mEngineImpl);
		EditorGlobalContext::instance()->CreateDefaultScene();

		// --- Create EditorUI ---
		mEditorUI = new EditorUI();
		mEditorUI->Initialize(mEngineImpl);

		// --- Create panels ---
		mHierarchyPanel = new SceneHierarchyPanel(mEditorUI);
		mInspectorPanel = new InspectorPanel(mEditorUI);
		mConsolePanel   = new ConsolePanel();
		mViewportPanel  = new ViewportPanel();
		mContentBrowserPanel = new ContentBrowserPanel();

		mEditorUI->AddPanel(mHierarchyPanel);
		mEditorUI->AddPanel(mViewportPanel);
		mEditorUI->AddPanel(mInspectorPanel);
		mEditorUI->AddPanel(mConsolePanel);
		mEditorUI->AddPanel(mContentBrowserPanel);



		// Welcome log
		mConsolePanel->AddLog("info", "Elaine Engine Editor initialized.");
		mConsolePanel->AddLog("info", "DX11 ImGui backend ready.");
		mConsolePanel->AddLog("info", "Scene viewport created with offscreen rendering.");

		return true;
	}

	void ElaineEditor::Destroy()
	{
		EditorGlobalContext::instance()->Destroy();

		delete mHierarchyPanel;  mHierarchyPanel = nullptr;
		delete mInspectorPanel;  mInspectorPanel = nullptr;
		delete mConsolePanel;    mConsolePanel = nullptr;
		delete mViewportPanel;   mViewportPanel = nullptr;
		delete mContentBrowserPanel; mContentBrowserPanel = nullptr;
		delete mEditorUI;        mEditorUI = nullptr;
		delete mImGuiLayer;      mImGuiLayer = nullptr;
	}

	void ElaineEditor::Tick()
	{
		if (!mImGuiLayer) return;

		Elaine::RenderToLogic_Barrier->Wait();
		auto* Ctx = EditorGlobalContext::instance();

		// ============================================================
		// 1. 延迟初始化传输路径（共享内存优先，失败后 fallback 到 staging）
		// ============================================================
		if (Ctx->GetOffscreenColorTexture() && !Ctx->mbSharedMemoryAttempted)
		{
			if (!Ctx->TrySetupSharedMemory(mImGuiLayer->GetDevice()))
			{
				Ctx->InitStagingFallback(mImGuiLayer->GetDevice());
			}
		}

		// ============================================================
		// 2. Staging 路径：读取上一帧 readIndex 纹理的 readback buffer
		// ============================================================
		if (Ctx->IsStagingReady())
		{
			Ctx->UpdateDX11TextureFromReadback(mImGuiLayer->GetDeviceContext());
		}

		// ============================================================
		// 3. 提交渲染命令（渲染到 writeIndex 纹理）
		// ============================================================
		mEngineImpl->RenderOneFrame();

		// ============================================================
		// 4. Staging 路径：在渲染命令中加入 image→buffer copy，然后交换
		// ============================================================
		if (Ctx->IsStagingReady())
		{
			Ctx->EnqueueCopyToReadbackBuffer();
			Ctx->SwapOffscreenBuffers();
		}

		Elaine::LogicToRender_Barrier->Signal();

		// ============================================================
		// 5. 更新 ViewportPanel
		// ============================================================
		ID3D11ShaderResourceView* srv = Ctx->GetViewportSRV();
		if (srv)
		{
			mViewportPanel->SetViewportTexture(srv,
				Ctx->GetViewportWidth(),
				Ctx->GetViewportHeight());
		}

		mImGuiLayer->BeginFrame();

		// 同步 ImGui 的输入捕获标志到引擎 InputSystem
		{
			ImGuiIO& io = ImGui::GetIO();
			Elaine::InputSystem::instance()->SetUIWantsKeyboard(io.WantCaptureKeyboard);
			Elaine::InputSystem::instance()->SetUIWantsMouse(io.WantCaptureMouse);
		}

		mEditorUI->Draw();
		mImGuiLayer->EndFrame();
	}

	void ElaineEditor::Run()
	{
		while (!mMainWindow->shouldClose())
		{
			mMainWindow->pollEvents();
			Tick();
		}
	}
}