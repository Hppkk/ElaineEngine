#pragma once
#include "ElaineCorePrerequirements.h"
#include <cstdint>
#include <string>

namespace Elaine
{
    enum class RenderDocCapturePhase { Unavailable, Idle, Pending, Capturing, Succeeded, Failed, Cancelled };

    struct RenderDocCaptureStatus
    {
        bool Connected = false;
        bool AutomaticCapture = false;
        bool VulkanReady = false;
        RenderDocCapturePhase Phase = RenderDocCapturePhase::Unavailable;
        uint64_t Frame = 0;
        std::string Message;
        std::string CapturePath;
    };

    // Optional development tool. Initialize before either graphics API is initialized.
    class ElaineCoreExport RenderDocCapture
    {
    public:
        static bool InitializeFromCommandLine(int Argc, char** Argv, std::string& Error);
        static RenderDocCaptureStatus GetStatus();
        static bool RequestCapture();
        static bool OpenLastCapture();
        static void Shutdown(); // After graphics shutdown; never unloads another injector's DLL.
    };
}
