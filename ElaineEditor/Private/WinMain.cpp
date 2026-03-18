#include "ElaineEditor.h"
#include "ElaineEngine.h"
#include "ElainePlatformWindow.h"

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
		INIT_DESC.UseRHIThread = true;
		INIT_DESC.WindowHandle = NewWindow->getNativeHandle();
		NewEngine->Initialize(INIT_DESC);
		NewEngine->PostInitialize();

		// --- Create editor (场景/视口/离屏渲染 由 EditorGlobalContext 管理) ---
		ElaineEditor* NewEditor = new ElaineEditor(NewEngine);
		NewEditor->Initialize(NewWindow);
		NewEditor->Run();

		delete NewEditor;
		delete NewEngine;
		return 0;
	}

#ifdef __cplusplus
}
#endif // __cplusplus

}