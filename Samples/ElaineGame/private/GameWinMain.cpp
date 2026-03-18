#include "ElaineEngine.h"
#include "ElaineWorld.h"
#include "ElaineGameObject.h"
#include "ElaineMeshComponent.h"
#include "ElaineCameraComponent.h"
#include "ElaineSkyComponent.h"
#include "ElainePlatformWindow.h"


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
		NewWindow->createSwapchain();
		NewEngine->PostInitialize();

		//TEST
		Elaine::World* Test = NewEngine->CreateWorld();
		Elaine::GameObject* Scene = Test->CreateGameObject();
		Elaine::CameraComponent* NewCamera = Scene->AddComponentType<Elaine::CameraComponent>("CameraComponent");
		Elaine::SkyComponent* NewSky = Test->CreateGameObject()->AddComponentType<Elaine::SkyComponent>("SkyComponent");
		//NewSky->SetMaterial("material_instance/SkyBox.mi");
		Elaine::StaticMeshComponent* NewCom = Test->CreateGameObject()->AddComponentType<Elaine::StaticMeshComponent>("StaticMeshComponent");
		NewCom->ChangeMesh("");

		Elaine::ViewportDesc Desc;
		Elaine::Viewport* MainView = NewEngine->CreateViewport(Desc);
		MainView->SetTarget(NewWindow);
		MainView->SetWorld(Test);
		MainView->SetCamera(NewCamera);
		NewEngine->RegisterViewport(MainView);

		while (!NewWindow->shouldClose())
		{
			NewWindow->pollEvents();
			//Tick();
		}

		//NewEngine->UnregisterViewport(MainView);
		//NewEngine->DestroyViewport(MainView);
		delete NewEngine;
		return 0;
	}

#ifdef __cplusplus
}
#endif // __cplusplus
