#pragma once
#include "Common/ElaineCorePrerequirements.h"
#include "Common/ElaineInputTypes.h"
#include <Windows.h>

namespace Elaine
{
    // Main windows call this from WindowsWindow; ImGui floating windows use a subclass.
    // Never call both collectors for the same HWND/message.
    class ElaineCoreExport WindowsInput
    {
    public:
        static EKeyCode TranslateKey(WPARAM key, LPARAM flags);
        static void QueueMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    };
}
