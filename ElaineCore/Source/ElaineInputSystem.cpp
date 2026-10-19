#include "ElainePrecompiledHeader.h"
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
