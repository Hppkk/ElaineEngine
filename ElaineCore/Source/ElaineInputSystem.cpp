#ifndef ELAINE_INPUT_STANDALONE
#include "ElainePrecompiledHeader.h"
#else
#include <Windows.h>
#endif
#include "ElaineInputSystem.h"

namespace Elaine
{
    InputSystem::InputSystem()
    {
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
        SetPointerCaptureCallback([](InputWindowId id, bool capture) {
            auto window = reinterpret_cast<HWND>(id);
            if (capture) SetCapture(window);
            else if (GetCapture() == window) ReleaseCapture();
        });
#endif
    }
}
