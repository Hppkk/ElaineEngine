#include "ElaineEditor.h"
#include "ElaineEngine.h"
#include "render/ElaineRenderDocCapture.h"
#include <iostream>
#include "ElainePlatformWindow.h"
#include "ElaineBarrier.h"
#include "ElaineEditorGlobalContext.h"
#include "ElaineLogSystem.h"
#include "imgui.h"

namespace Editor
{
#ifdef __cplusplus
extern "C" {
#endif // _cplusplus

	HWND	G_WIN_HANDLE;
	int		G_WIN_HEIGHT = 800;
	int		G_WIN_WIDTH = 1440;

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
        switch (msg)
        {
        case WM_ACTIVATE:
            return 0;
        case WM_SIZE:
            // Save the new client area dimensions.
			G_WIN_WIDTH = LOWORD(lParam);
            G_WIN_HEIGHT = HIWORD(lParam);
            return 0;

        case WM_ENTERSIZEMOVE:
            return 0;

        case WM_EXITSIZEMOVE:
            return 0;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_MENUCHAR:
            return MAKELRESULT(0, MNC_CLOSE);

        case WM_GETMINMAXINFO:
            ((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
            return 0;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            return 0;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            return 0;
        case WM_MOUSEMOVE:
            return 0;
        }

        return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	int main(int argc, char** argv)
	{
        std::string CaptureError;
        if (!Elaine::RenderDocCapture::InitializeFromCommandLine(argc, argv, CaptureError))
        { std::cerr << "RenderDoc initialization failed: " << CaptureError << std::endl; return 2; }
        struct CaptureLifetime { ~CaptureLifetime() { Elaine::RenderDocCapture::Shutdown(); } } CaptureGuard;
        try
        {
        bool Smoke = false, DedicatedRHI = true, InteropStress = false;
        ViewportInteropMode Mode = ViewportInteropMode::Auto;
        ViewportInteropFault Fault = ViewportInteropFault::None;
        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--resource-smoke") Smoke = true;
            if (std::string(argv[i]) == "--viewport-interop-stress") { Smoke = true; InteropStress = true; }
            if (std::string(argv[i]) == "--rhi-inline") DedicatedRHI = false;
            const std::string Arg(argv[i]);
            if (Arg == "--viewport-interop=shared") Mode = ViewportInteropMode::Shared;
            else if (Arg == "--viewport-interop=readback") Mode = ViewportInteropMode::Readback;
            else if (Arg == "--viewport-interop=auto") Mode = ViewportInteropMode::Auto;
            else if (Arg == "--viewport-interop-fault=adapter") Fault = ViewportInteropFault::Adapter;
            else if (Arg == "--viewport-interop-fault=capability") Fault = ViewportInteropFault::Capability;
            else if (Arg == "--viewport-interop-fault=import") Fault = ViewportInteropFault::Import;
            else if (Arg == "--viewport-interop-fault=mutex") Fault = ViewportInteropFault::MutexAbandoned;
            else if (Arg == "--viewport-interop-fault=timeout") Fault = ViewportInteropFault::MutexTimeout;
            else if (Arg.find("--viewport-interop=") == 0 || Arg.find("--viewport-interop-fault=") == 0) return 2;
        }
		// --- Create window using PlatformWindow ---
		Elaine::WindowDesc CreateDesc{};
		CreateDesc.title = "ElaineEditor";
		CreateDesc.width = 1600;
		CreateDesc.height = 900;
		Elaine::PlatformWindow* NewWindow = new Elaine::PlatformWindow();
		NewWindow->create(CreateDesc);

		// --- Create engine ---
		Elaine::ElaineEngine* NewEngine = new Elaine::ElaineEngine();
		Elaine::RHI_PARAM_DESC INIT_DESC;
		INIT_DESC.Height = G_WIN_HEIGHT;
		INIT_DESC.Width = G_WIN_WIDTH;
		INIT_DESC.RHIType = Elaine::Vulkan;
		INIT_DESC.UseRHIThread = DedicatedRHI;
		INIT_DESC.NativeHandle = NewWindow->getNativeHandle();
		NewEngine->Initialize(INIT_DESC);
		NewEngine->PostInitialize();

		// --- Create editor (场景/视口/离屏渲染 由 EditorGlobalContext 管理) ---
		ElaineEditor* NewEditor = new ElaineEditor(NewEngine);
        int ExitCode = 0;
        try
        {
        if (!NewEditor->Initialize(NewWindow, Mode, Fault)) throw std::runtime_error("Editor initialization failed");
        if (Smoke)
        {
            ImGui::GetIO().IniFilename = nullptr; // Automated runs must not overwrite the user's layout.
            for (int Frame = 0; Frame < 180 && !NewWindow->shouldClose(); ++Frame)
            {
                if (InteropStress)
                {
                    auto* Context = EditorGlobalContext::instance();
                    if (Frame == 30) Context->RecreateViewportTransport(640,480);
                    if (Frame == 60) Context->RecreateViewportTransport(0,0);
                    if (Frame == 70) Context->RecreateViewportTransport(1280,720);
                    if (Frame == 100) Context->RecreateViewportTransport(800,600);
                    if (Frame == 120) ShowWindow(static_cast<HWND>(NewWindow->getNativeHandle()),SW_MINIMIZE);
                    if (Frame == 130) ShowWindow(static_cast<HWND>(NewWindow->getNativeHandle()),SW_RESTORE);
                    if (Frame == 145) Context->RecreateViewportTransport(1280,720);
                }
                NewWindow->pollEvents(); NewEditor->Tick();
            }
        }
        else NewEditor->Run();
        auto* Context = EditorGlobalContext::instance();
        if (const auto* Stats = Context->GetInteropStatistics())
        {
            LOG_INFO("Viewport interop smoke: submitted={} displayed={} dropped={} skipped={} readbacks={}",
                Stats->Submitted, Stats->Displayed, Stats->Dropped, Stats->Skipped, Context->GetReadbackCount());
            if (Smoke && (!Stats->Displayed || Context->GetReadbackCount())) ExitCode = 1;
        }
        else if (Mode == ViewportInteropMode::Shared) ExitCode = 1;
        }
        catch (const std::exception& Error) { LOG_ERROR("Editor failed: {}", Error.what()); ExitCode = 1; }

		delete NewEditor;
		delete NewEngine;
        delete NewWindow;
        const auto CaptureResult = Elaine::RenderDocCapture::GetStatus();
        if (CaptureResult.AutomaticCapture && CaptureResult.Phase != Elaine::RenderDocCapturePhase::Succeeded)
        { std::cerr << "Requested Vulkan capture did not complete: " << CaptureResult.Message << std::endl; return 1; }
        return ExitCode ? ExitCode : (Smoke && Elaine::GetRHIValidationErrorCount() != 0 ? 1 : 0);
        }
        catch (const std::exception& Error) { std::cerr << "Editor startup failed: " << Error.what() << std::endl; return 1; }
	}

#ifdef __cplusplus
}
#endif // __cplusplus

}