#include "ElainePrecompiledHeader.h"
#include "ElaineEditorViewportInterop.h"
#include "TaskGraph/ElaineTask.h"
#include "ElaineRoot.h"
#include "ElaineRenderCommandQueue.h"
#include "ElaineRenderContext.h"
#include "ElaineLogSystem.h"
#include <algorithm>
#include <chrono>
#include <thread>

namespace Editor
{
    namespace
    {
        void RequireDX(HRESULT Result, const char* Operation)
        {
            if (Result != S_OK) throw std::runtime_error(std::string(Operation) + ": HRESULT " + std::to_string(static_cast<unsigned>(Result)));
        }
        // This is a shutdown-only wait, executed in the original RenderThread synchronization window.
        void DrainVulkan()
        {
            ENQUEUE_RENDER_COMMAND(DrainSharedViewport)([](Elaine::RenderContext&) {
                auto* RHI = Elaine::GetDynamicRHI();
                RHI->GetDefaultCommandContext()->RHIWaitIdle();
                RHI->ProcessGPUCompletions();
            });
            Elaine::Root::instance()->FlushRenderCommands();
        }
    }

    EditorViewportInterop::~EditorViewportInterop()
    {
        // Explicit Shutdown runs before either API device is destroyed.
        try { Shutdown(); } catch (const std::exception& Error) { LOG_ERROR("Viewport interop shutdown: {}", Error.what()); }
    }

    bool EditorViewportInterop::Initialize(ID3D11Device* Device, ID3D11DeviceContext* Context,
        uint32_t Width, uint32_t Height, std::string& Error, ViewportInteropFault Fault)
    {
        mDevice = Device; mContext = Context; mFault = Fault;
        if (!Device || !Context) { Error = "DX11 device/context is missing"; return false; }
        VulkanRHI::DX11AdapterId Id;
        if (!VulkanRHI::GetDX11AdapterId(Id, Error)) return false;
        Com<IDXGIDevice> DXDevice; Com<IDXGIAdapter> Adapter;
        DXGI_ADAPTER_DESC Desc{};
        if (Device->QueryInterface(IID_PPV_ARGS(DXDevice.GetAddressOf())) != S_OK || DXDevice->GetAdapter(&Adapter) != S_OK || Adapter->GetDesc(&Desc) != S_OK)
        { Error = "Cannot query DX11 adapter identity"; return false; }
        if (Fault == ViewportInteropFault::Adapter || Desc.AdapterLuid.LowPart != Id.Low || Desc.AdapterLuid.HighPart != Id.High)
        { Error = "Vulkan and DX11 adapter LUIDs differ"; return false; }
        if (Fault == ViewportInteropFault::Capability) { Error = "Injected unsupported D3D11 texture import capability"; return false; }
        if (!VulkanRHI::SupportsDX11TextureImport(Width, Height, Error)) return false;
        if (!CreateGeneration(Width, Height, mActive, Error)) return false;
        mInitialized = true; mPaused = false;
        LOG_INFO("Viewport interop: DX11 -> Vulkan shared textures initialized, three slots, adapter {:08x}:{:08x}", static_cast<uint32_t>(Id.High), Id.Low);
        return true;
    }

    bool EditorViewportInterop::CreateGeneration(uint32_t Width, uint32_t Height, Generation& Result, std::string& Error)
    {
        try
        {
            if (!VulkanRHI::SupportsDX11TextureImport(Width, Height, Error)) return false;
            for (auto& Slot : Result.Slots)
            {
                D3D11_TEXTURE2D_DESC Desc{};
                Desc.Width = Width; Desc.Height = Height; Desc.MipLevels = 1; Desc.ArraySize = 1;
                Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; Desc.SampleDesc.Count = 1;
                Desc.Usage = D3D11_USAGE_DEFAULT; Desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
                Desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
                RequireDX(mDevice->CreateTexture2D(&Desc, nullptr, &Slot.Texture), "Create DX11 shared texture");
                RequireDX(mDevice->CreateShaderResourceView(Slot.Texture.Get(), nullptr, &Slot.SRV), "Create shared SRV");
                RequireDX(Slot.Texture.As(&Slot.Mutex), "Query shared keyed mutex");
                Com<IDXGIResource1> Resource; RequireDX(Slot.Texture.As(&Resource), "Query DXGIResource1");
                HANDLE Handle = nullptr;
                RequireDX(Resource->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &Handle), "Create shared NT handle");
                struct ImportResult { Elaine::RHITextureHandle Texture; std::string Error; };
                auto Imported = std::make_shared<ImportResult>();
                try
                {
                    if (mFault == ViewportInteropFault::Import) throw std::runtime_error("Injected Vulkan texture import failure");
                    ENQUEUE_RENDER_COMMAND(ImportSharedViewport)([Imported, Handle, Width, Height](Elaine::RenderContext&) {
                        Elaine::RHITextureDesc Texture;
                        Texture.mExtent.x = static_cast<float>(Width); Texture.mExtent.y = static_cast<float>(Height);
                        Texture.mFormat = Elaine::PF_R8G8B8A8; Texture.mNumMips = 1; Texture.mNumSamples = 1;
                        Texture.mArraySize = 1; Texture.mDimension = Elaine::TextureDimension::Texture2D;
                        Texture.mFlags = Elaine::TextureCreateFlags::RenderTargetable | Elaine::TextureCreateFlags::ShaderResource;
                        Imported->Texture = VulkanRHI::ImportDX11Texture(Texture, Handle, Imported->Error);
                    });
                    Elaine::Root::instance()->FlushRenderCommands();
                }
                catch (...) { CloseHandle(Handle); throw; }
                CloseHandle(Handle);
                if (!Imported->Texture) throw std::runtime_error(Imported->Error);
                Slot.Imported = std::move(Imported->Texture);
            }
            return true;
        }
        catch (const std::exception& Failure)
        {
            Error = Failure.what(); Result = {}; return false;
        }
    }

    EditorViewportInterop::Frame EditorViewportInterop::ReserveFrame()
    {
        if (!mInitialized || mPaused) return {};
        for (auto& Slot : mActive.Slots)
        {
            if (Slot.State != SlotState::Free) continue;
            Slot.State = SlotState::Reserved; Slot.FrameNumber = mNextFrame++;
            Slot.ViewData = std::make_shared<ViewportFrameData>();
            Slot.ViewData->FrameNumber = Slot.FrameNumber;
            auto Ticket = std::make_shared<VulkanRHI::DX11InteropSubmission>();
            auto Event = std::make_shared<TaskGraph::TaskCompletionEvent>();
            Event->Subscribe([Ticket](TaskGraph::TaskState State) {
                if (State != TaskGraph::TaskState::Succeeded) Ticket->Discard = true;
                auto Expected = VulkanRHI::DX11SubmissionState::Reserved;
                if (Ticket->State.compare_exchange_strong(Expected, VulkanRHI::DX11SubmissionState::Abandoned)) Ticket->GPUComplete = true;
            });
            Slot.Submission = Ticket; Slot.Completion = Event;
            mActive.Outstanding.push_back(Ticket); ++mStatistics.Submitted;
            return {Slot.Imported, Ticket, Event, Slot.FrameNumber, Slot.ViewData};
        }
        ++mStatistics.Skipped;
        return {};
    }

    void EditorViewportInterop::BeginFrame()
    {
        using State = VulkanRHI::DX11SubmissionState;
        std::array<size_t,3> Order{0,1,2};
        std::sort(Order.begin(),Order.end(),[&](size_t A,size_t B) { return mActive.Slots[A].FrameNumber > mActive.Slots[B].FrameNumber; });
        for (size_t Index : Order)
        {
            auto& Slot = mActive.Slots[Index];
            if (Slot.State != SlotState::Reserved || !Slot.Submission) continue;
            const auto Phase = Slot.Submission->State.load();
            if (Phase == State::Abandoned) { Slot.State = SlotState::Free; continue; }
            if (Phase == State::Failed) throw std::runtime_error("Vulkan shared texture submission failed");
            if (Phase != State::Submitted) continue;
            if (!Slot.Completion->IsComplete()) continue;
            const HRESULT Result = mFault == ViewportInteropFault::MutexAbandoned ? WAIT_ABANDONED :
                (mFault == ViewportInteropFault::MutexTimeout ? WAIT_TIMEOUT : Slot.Mutex->AcquireSync(1, 0));
            if (Result == WAIT_TIMEOUT) continue;
            if (Result != S_OK) throw std::runtime_error("Shared keyed mutex acquire failed/abandoned: " + std::to_string(static_cast<unsigned>(Result)));
            if (Slot.Submission->Discard || Slot.FrameNumber <= mDisplayFrame)
            { Slot.State = SlotState::Retiring; ++mStatistics.Dropped; continue; }
            if (mDisplaySlot >= 0) mActive.Slots[mDisplaySlot].State = SlotState::Retiring;
            Slot.State = SlotState::Displaying; mDisplaySlot = static_cast<int>(Index);
            mDisplayFrame = Slot.FrameNumber; ++mStatistics.Displayed;
        }
        mActive.Outstanding.erase(std::remove_if(mActive.Outstanding.begin(), mActive.Outstanding.end(),
            [](const auto& Ticket) { return Ticket->GPUComplete.load(); }), mActive.Outstanding.end());
        CollectRetired();
    }

    void EditorViewportInterop::ReleaseSlot(Slot& Slot)
    {
        RequireDX(Slot.Mutex->ReleaseSync(0), "Return shared texture to Vulkan");
        Slot.State = SlotState::Free;
    }

    void EditorViewportInterop::EndUIFrame()
    {
        bool HasRetiring = false;
        for (const auto& Slot : mActive.Slots) HasRetiring |= Slot.State == SlotState::Retiring;
        if (!HasRetiring) return;
        // Called after main AND platform-window draws. ReleaseSync orders GPU access;
        // Flush submits the last DX11 reads without waiting for their completion.
        ID3D11ShaderResourceView* Null = nullptr;
        mContext->PSSetShaderResources(0, 1, &Null); mContext->Flush();
        for (auto& Slot : mActive.Slots) if (Slot.State == SlotState::Retiring) ReleaseSlot(Slot);
    }

    ID3D11ShaderResourceView* EditorViewportInterop::GetDisplaySRV() const
    { return mDisplaySlot >= 0 ? mActive.Slots[mDisplaySlot].SRV.Get() : nullptr; }

    ViewportFrameData EditorViewportInterop::GetDisplayViewData() const
    {
        if (mDisplaySlot < 0 || !mActive.Slots[mDisplaySlot].ViewData) return {};
        return *mActive.Slots[mDisplaySlot].ViewData;
    }

    void EditorViewportInterop::RetireActive()
    {
        if (!mActive.Slots[0].Imported) return;
        for (auto& Slot : mActive.Slots)
            if (Slot.Submission && Slot.Submission->State == VulkanRHI::DX11SubmissionState::Reserved && Slot.Completion)
                Slot.Completion->Cancel();
        if (mDisplaySlot >= 0) mActive.Slots[mDisplaySlot].State = SlotState::Retiring;
        EndUIFrame(); mDisplaySlot = -1;
        D3D11_QUERY_DESC Query{D3D11_QUERY_EVENT, 0};
        RequireDX(mDevice->CreateQuery(&Query, &mActive.LastDX11Use), "Create shared generation retirement query");
        mContext->End(mActive.LastDX11Use.Get()); mContext->Flush();
        mRetired.push_back(std::move(mActive)); mActive = {};
    }

    void EditorViewportInterop::CollectRetired()
    {
        mRetired.erase(std::remove_if(mRetired.begin(), mRetired.end(), [&](auto& Generation) {
            for (auto& Ticket : Generation.Outstanding)
                if (!Ticket->GPUComplete.load()) return false;
            BOOL Completed = FALSE;
            const auto Result = mContext->GetData(Generation.LastDX11Use.Get(), &Completed, sizeof(Completed), D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if (Result != S_OK && Result != S_FALSE) RequireDX(Result,"Poll shared viewport retirement");
            return Result == S_OK && Completed;
        }), mRetired.end());
    }

    bool EditorViewportInterop::Recreate(uint32_t Width, uint32_t Height, std::string& Error)
    {
        RetireActive(); mPaused = true;
        if (!Width || !Height) return true;
        if (!CreateGeneration(Width, Height, mActive, Error)) return false;
        mPaused = false; return true;
    }

    void EditorViewportInterop::Shutdown()
    {
        if (!mInitialized) { mActive = {}; mContext.Reset(); mDevice.Reset(); return; }
        RetireActive();
        DrainVulkan();
        for (auto& Generation : mRetired)
        {
            BOOL Complete = FALSE;
            const auto Deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
            HRESULT Result = S_FALSE;
            while ((Result = mContext->GetData(Generation.LastDX11Use.Get(), &Complete, sizeof(Complete), D3D11_ASYNC_GETDATA_DONOTFLUSH)) == S_FALSE)
            {
                if (std::chrono::steady_clock::now() > Deadline) throw std::runtime_error("DX11 shared viewport retirement timed out");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            RequireDX(Result, "Wait for DX11 shared viewport retirement");
        }
        mRetired.clear(); mInitialized = false; mPaused = true;
        mContext.Reset(); mDevice.Reset();
    }
}
