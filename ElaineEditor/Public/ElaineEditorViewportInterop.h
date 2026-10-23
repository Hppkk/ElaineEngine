#pragma once
#include "render/vulkan/ElaineVulkanDX11Interop.h"
#include "ElaineViewportFrame.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <array>
#include <vector>

namespace TaskGraph { class TaskCompletionEvent; }

namespace Editor
{
    enum class ViewportInteropMode { Auto, Shared, Readback };
    enum class ViewportInteropFault { None, Adapter, Capability, Import, MutexTimeout, MutexAbandoned };

    // GameThread owns slot selection and all DX11 immediate-context calls.
    // Only the texture and submission ticket cross to RenderThread/RHIThread.
    class EditorViewportInterop
    {
    public:
        struct Frame
        {
            Elaine::RHITextureHandle Texture;
            std::shared_ptr<VulkanRHI::DX11InteropSubmission> Submission;
            std::shared_ptr<TaskGraph::TaskCompletionEvent> Completion;
            uint64_t FrameNumber = 0;
            std::shared_ptr<ViewportFrameData> ViewData;
            explicit operator bool() const { return !!Texture; }
        };
        struct Statistics { uint64_t Submitted = 0, Displayed = 0, Dropped = 0, Skipped = 0; };
        EditorViewportInterop() = default;
        ~EditorViewportInterop();
        bool Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context, uint32_t Width, uint32_t Height,
            std::string& Error, ViewportInteropFault Fault = ViewportInteropFault::None);
        bool Recreate(uint32_t Width, uint32_t Height, std::string& Error);
        void BeginFrame();
        Frame ReserveFrame();
        void EndUIFrame();
        void Shutdown();
        ID3D11ShaderResourceView* GetDisplaySRV() const;
        const Statistics& GetStatistics() const { return mStatistics; }
        ViewportFrameData GetDisplayViewData() const;
        uint64_t GetDisplayFrame() const { return mDisplayFrame; }
        size_t GetRetiredGenerationCount() const { return mRetired.size(); }
    private:
        template<class T> using Com = Microsoft::WRL::ComPtr<T>;
        enum class SlotState { Free, Reserved, Displaying, Retiring };
        struct Slot
        {
            Com<ID3D11Texture2D> Texture;
            Com<ID3D11ShaderResourceView> SRV;
            Com<IDXGIKeyedMutex> Mutex;
            Elaine::RHITextureHandle Imported;
            std::shared_ptr<VulkanRHI::DX11InteropSubmission> Submission;
            std::shared_ptr<TaskGraph::TaskCompletionEvent> Completion;
            SlotState State = SlotState::Free;
            uint64_t FrameNumber = 0;
            std::shared_ptr<ViewportFrameData> ViewData;
        };
        struct Generation
        {
            std::array<Slot, 3> Slots;
            std::vector<std::shared_ptr<VulkanRHI::DX11InteropSubmission>> Outstanding;
            Com<ID3D11Query> LastDX11Use;
        };
        bool CreateGeneration(uint32_t Width, uint32_t Height, Generation& Result, std::string& Error);
        void ReleaseSlot(Slot& Slot);
        void RetireActive();
        void CollectRetired();
        Com<ID3D11Device> mDevice;
        Com<ID3D11DeviceContext> mContext;
        Generation mActive;
        std::vector<Generation> mRetired;
        ViewportInteropFault mFault = ViewportInteropFault::None;
        uint64_t mNextFrame = 1, mDisplayFrame = 0;
        int mDisplaySlot = -1;
        bool mInitialized = false, mPaused = true;
        Statistics mStatistics;
    };
}
