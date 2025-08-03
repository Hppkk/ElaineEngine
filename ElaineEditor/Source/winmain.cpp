#include "ElaineRoot.h"
#include "ElaineLogSystem.h"
#include "ElaineEditor.h"
#include "ElaineEngine.h"
#include "render/ElaineWindowSystem.h"

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
#include <Windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#endif // ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS


namespace ElaineEditor
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

	bool InitMainWindow()
	{
		wchar_t ExePath[MAX_PATH];
		GetModuleFileNameW(NULL, ExePath, MAX_PATH);
		PathRemoveFileSpecW(ExePath);
		std::wstring IconPath = std::wstring(ExePath) + L"\\..\\..\\icons\\ElaineEngine.ico";

		HICON hIcon = (HICON)LoadImage(
			NULL,
			IconPath.c_str(),
			IMAGE_ICON,
			32,
			32,
			LR_LOADFROMFILE | LR_DEFAULTSIZE
		);

		WNDCLASSEXW wc { };
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = GetModuleHandleW(0);
		wc.hIcon = hIcon;
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

		RECT R = { 0, 0, G_WIN_WIDTH, G_WIN_HEIGHT };
		AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
		int width = R.right - R.left;
		int height = R.bottom - R.top;

		G_WIN_HANDLE = CreateWindowExW(0, wc.lpszClassName, L"ElaineEditor", WS_OVERLAPPEDWINDOW | SW_SHOWDEFAULT, CW_USEDEFAULT,
			CW_USEDEFAULT, G_WIN_WIDTH, G_WIN_HEIGHT, HWND(0), HMENU(0), wc.hInstance, NULL);
		if (!G_WIN_HANDLE)
		{
			MessageBox(0, L"CreateWindow Failed.", 0, 0);
			return false;
		}

		ShowWindow(G_WIN_HANDLE, SW_SHOW);
		UpdateWindow(G_WIN_HANDLE);



		if (hIcon != NULL)
		{
			SendMessage(G_WIN_HANDLE, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		}

		return true;
	}

	int main(int argc, char** argv)
	{
		InitMainWindow();
		
		Elaine::ElaineEngine* NewEngine = new Elaine::ElaineEngine();
		Elaine::RHI_PARAM_DESC INIT_DESC;
		INIT_DESC.Height = G_WIN_HEIGHT;
		INIT_DESC.Width = G_WIN_WIDTH;
		INIT_DESC.RHIType = Elaine::Vulkan;
		INIT_DESC.UseRHIThread = true;
		INIT_DESC.WindowHandle = G_WIN_HANDLE;
		NewEngine->Initilize(INIT_DESC);

		ElaineEditor* NewEditor = new ElaineEditor(NewEngine);
		NewEditor->Initilize();
		
		MSG msg = { 0 };
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				NewEditor->Tick();
			}
			else
			{ 
				
			}
		}

		delete NewEditor;
		delete NewEngine;
		return 0;
	}
#ifdef __cplusplus
}
#endif // __cplusplus

}