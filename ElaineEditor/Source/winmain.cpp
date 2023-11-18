#include "ElaineRoot.h"
#include "ElaineLogSystem.h"
#include "ElaineEditor.h"
#include "ElaineEngine.h"
#include "render/ElaineWindowSystem.h"

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
#include <Windows.h>
#endif // ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS


namespace Elaine
{
#ifdef __cplusplus
extern "C" {
#endif // _cplusplus

	HWND		g_win_hwnd;
	int		g_height = 1080;
	int		g_width = 1920;

	LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
        switch (msg)
        {
        case WM_ACTIVATE:
            return 0;
        case WM_SIZE:
            // Save the new client area dimensions.
			g_width = LOWORD(lParam);
            g_height = HIWORD(lParam);
            return 0;

        case WM_ENTERSIZEMOVE:
            return 0;

            // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
            // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
            return 0;

            // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

            // The WM_MENUCHAR message is sent when a menu is active and the user presses 
            // a key that does not correspond to any mnemonic or accelerator key. 
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

            // Catch this message so to prevent the window from becoming too small.
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

	bool initMainWindow()
	{
		WNDCLASSEXW wc { };
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandleW(0);
		wc.hIcon = LoadIcon(0, IDI_APPLICATION);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
		wc.lpszMenuName = 0;
		wc.lpszClassName = L"ElaineEditor";
		wc.cbSize = sizeof(WNDCLASSEXW);
		
		if (!RegisterClassExW(&wc))
		{
			MessageBox(0, L"RegisterClass Failed.", 0, 0);
			return false;
		}

		RECT R = { 0, 0, g_width, g_height };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		g_win_hwnd = CreateWindowExW(0, wc.lpszClassName, L"ElaineEditor", WS_OVERLAPPEDWINDOW | SW_SHOWDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, g_width, g_height, HWND(0), HMENU(0), wc.hInstance, NULL);
		if (!g_win_hwnd)
		{
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		ShowWindow(g_win_hwnd, SW_SHOW);
		UpdateWindow(g_win_hwnd);

		return true;
	}

	int main(int argc, char** argv)
	{
		//initMainWindow();

		new Root(em_Editor);
		Root::instance()->Init();
		
		ElaineEngine* engine = new ElaineEngine();
		EngineInitDesc desc{};
		engine->initilize(desc);
		ElaineEditor* editor = new ElaineEditor();
		editor->initialize();
		WindowSystem::instance()->tick();
		//MSG msg = { 0 };
		//while (msg.message != WM_QUIT)
		//{
		//	if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		//	{
		//		TranslateMessage(&msg);
		//		DispatchMessage(&msg);
		//	}
		//	else
		//	{
		//		
		//	}
		//}
		delete editor;
		delete engine;
		delete Root::instance();
		return 0;
	}
#ifdef __cplusplus
}
#endif // __cplusplus

}