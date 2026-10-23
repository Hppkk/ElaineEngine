#include "ElaineEditor.h"
#include "ElaineResourceImportService.h"
#include "ElaineEditorInput.h"
#include "TaskGraph/ElaineTaskGraph.h"
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

	bool ElaineEditor::Initialize(Elaine::PlatformWindow* window, ViewportInteropMode Mode, ViewportInteropFault Fault)
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
        EditorGlobalContext::instance()->InitializeViewportTransport(mImGuiLayer->GetDevice(), mImGuiLayer->GetDeviceContext(), Mode, Fault);

		// --- Create EditorUI ---
		mEditorUI = new EditorUI();
		mEditorUI->Initialize(mEngineImpl);

		// --- Create panels ---
		mHierarchyPanel = new SceneHierarchyPanel(mEditorUI);
		mInspectorPanel = new InspectorPanel(mEditorUI);
		mConsolePanel   = new ConsolePanel();
		mViewportPanel  = new ViewportPanel();
        mViewportPanel->InitializeInput();
        Elaine::InputLayerDesc uiInput;
        uiInput.Name = "EditorUI";
        uiInput.Priority = 1000;
        uiInput.Enabled = false;
        mUIInputLayer = Elaine::InputSystem::instance()->RegisterLayer(uiInput);
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
        Elaine::ResourceImportService::Get().Shutdown();
        mUIInputLayer.Reset();
		if (EditorGlobalContext::instance()) EditorGlobalContext::instance()->Destroy();

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
        TaskGraph::TaskGraph::instance()->BeginGameThreadFrame();
		auto* Ctx = EditorGlobalContext::instance();

        Ctx->PrepareViewportFrame();

		if (Ctx->IsStagingReady())
		{
			Ctx->UpdateDX11TextureFromReadback(mImGuiLayer->GetDeviceContext());
		}

		ID3D11ShaderResourceView* srv = Ctx->GetViewportSRV();
        mViewportPanel->SetViewportTexture(srv, Ctx->GetViewportWidth(), Ctx->GetViewportHeight());

        auto* input = Elaine::InputSystem::instance();
        input->BeginFrame();
        mViewportPanel->BeginInputFrame();
        const bool popupBeforeFrame = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
        mImGuiLayer->BeginFrame();
        mEditorUI->Draw();
        UpdateEditorInputRouting(*mViewportPanel, mUIInputLayer, popupBeforeFrame);
        input->DispatchEvents();
        mViewportPanel->ProcessInput();

        // Input and editor operations must precede world ticking and render submission.
        mEngineImpl->RenderOneFrame();
        if (Ctx->IsStagingReady())
        {
            Ctx->EnqueueCopyToReadbackBuffer();
            Ctx->SwapOffscreenBuffers();
        }
        Elaine::LogicToRender_Barrier->Signal();
		mImGuiLayer->EndFrame();
        Ctx->EndViewportUIFrame();
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
