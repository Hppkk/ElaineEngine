#pragma once
#include "render/ElaineRenderDocCapture.h"
#include <memory>
#include <mutex>

namespace Elaine::RenderDocInternal
{
    // Private adapter seam: tests exercise the same state machine without loading a GPU API.
    class Driver
    {
    public:
        virtual ~Driver() = default;
        virtual bool IsCapturing() = 0;
        virtual bool Start(void* Device) = 0;
        virtual bool End(void* Device) = 0;
        virtual void Discard(void* Device) = 0;
        virtual uint32_t CaptureCount() = 0;
        virtual std::string CapturePath(uint32_t Index) = 0;
        virtual void SetTitle(const std::string& Title) = 0;
        virtual bool Open(const std::string& Path) = 0;
    };

    class CaptureState
    {
    public:
        void Connect(std::shared_ptr<Driver> InDriver, uint64_t InAutoFrame)
        {
            std::lock_guard Lock(Mutex);
            Backend = std::move(InDriver); AutoFrame = InAutoFrame; FrameCount = 0;
            Status = {}; Status.Connected = static_cast<bool>(Backend); Status.AutomaticCapture = InAutoFrame != 0;
            Status.Message = Backend ? "RenderDoc connected; waiting for Vulkan capture layer" : "RenderDoc is not enabled";
        }
        void Ready()
        {
            std::lock_guard Lock(Mutex);
            if (!Backend) return;
            Status.VulkanReady = true; Status.Phase = RenderDocCapturePhase::Idle;
            Status.Message = "Vulkan capture ready";
        }
        void Fail(const std::string& Message)
        {
            std::lock_guard Lock(Mutex);
            AbortLocked(Message); Status.VulkanReady = false;
            Status.Phase = RenderDocCapturePhase::Failed; Status.Message = Message;
        }
        RenderDocCaptureStatus Snapshot() const
        { std::lock_guard Lock(Mutex); return Status; }
        bool Request()
        {
            std::lock_guard Lock(Mutex);
            if (!Backend || !Status.VulkanReady || ActiveDevice) return false;
            Status.Phase = RenderDocCapturePhase::Pending;
            Status.Message = "Waiting for the next Vulkan scene frame"; return true;
        }
        void Begin(void* Device)
        {
            std::lock_guard Lock(Mutex);
            if (!Backend || !Status.VulkanReady) return;
            ++FrameCount;
            if (AutoFrame && FrameCount >= AutoFrame)
            { AutoFrame = 0; Status.Phase = RenderDocCapturePhase::Pending; }
            if (Status.Phase != RenderDocCapturePhase::Pending || ActiveDevice) return;
            if (Backend->IsCapturing())
            { Status.Message = "Waiting for the external RenderDoc capture to finish"; return; }
            BeforeCount = Backend->CaptureCount();
            if (!Backend->Start(Device))
            { Status.Phase = RenderDocCapturePhase::Failed; Status.Message = "RenderDoc did not start a Vulkan capture"; return; }
            ActiveDevice = Device; Status.Frame = FrameCount;
            Status.Phase = RenderDocCapturePhase::Capturing; Status.Message = "Capturing Vulkan scene frame";
            Backend->SetTitle("Elaine Vulkan scene " + std::to_string(FrameCount));
        }
        void End()
        {
            std::lock_guard Lock(Mutex);
            if (!ActiveDevice || !Backend) return;
            void* Device = ActiveDevice; ActiveDevice = nullptr;
            const bool Finished = Backend->End(Device);
            if (!Finished && Backend->IsCapturing()) Backend->Discard(Device);
            if (!Finished || Backend->CaptureCount() <= BeforeCount)
            { Status.Phase = RenderDocCapturePhase::Failed; Status.Message = "RenderDoc failed to save the Vulkan capture"; return; }
            auto Path = Backend->CapturePath(Backend->CaptureCount() - 1);
            if (Path.empty())
            { Status.Phase = RenderDocCapturePhase::Failed; Status.Message = "RenderDoc returned no capture file"; return; }
            Status.CapturePath = std::move(Path); Status.Phase = RenderDocCapturePhase::Succeeded;
            Status.Message = "Vulkan capture saved";
        }
        void Abort(const std::string& Reason)
        { std::lock_guard Lock(Mutex); AbortLocked(Reason); }
        void Detach()
        {
            std::lock_guard Lock(Mutex);
            const bool WasActive = ActiveDevice != nullptr;
            AbortLocked("Vulkan device is shutting down");
            if (WasActive || Status.Phase == RenderDocCapturePhase::Pending || AutoFrame)
            { Status.Phase = RenderDocCapturePhase::Cancelled; Status.Message = "Capture cancelled at shutdown"; }
            AutoFrame = 0; Status.VulkanReady = false;
        }
        void Disconnect()
        {
            Detach(); std::lock_guard Lock(Mutex);
            Backend.reset(); Status.Connected = false;
        }
        bool Open()
        {
            std::lock_guard Lock(Mutex);
            return Backend && !Status.CapturePath.empty() && Backend->Open(Status.CapturePath);
        }
    private:
        void AbortLocked(const std::string& Reason)
        {
            if (!ActiveDevice || !Backend) return;
            Backend->Discard(ActiveDevice); ActiveDevice = nullptr;
            Status.Phase = RenderDocCapturePhase::Failed; Status.Message = Reason;
        }
        mutable std::mutex Mutex;
        std::shared_ptr<Driver> Backend;
        RenderDocCaptureStatus Status;
        void* ActiveDevice = nullptr;
        uint64_t FrameCount = 0, AutoFrame = 0;
        uint32_t BeforeCount = 0;
    };
}
