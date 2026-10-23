#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include <atomic>
#include <memory>
#include <string>

// Windows/Vulkan backend interface. No native API headers are required by the editor.
namespace VulkanRHI
{
    struct DX11AdapterId { uint32 Low = 0; int32 High = 0; };
    enum class DX11SubmissionState { Reserved, Recording, Submitted, Abandoned, Failed };
    struct DX11InteropSubmission
    {
        std::atomic<DX11SubmissionState> State{DX11SubmissionState::Reserved};
        std::atomic_bool GPUComplete{false};
        std::atomic_bool Discard{false};
    };
    ElaineCoreExport bool GetDX11AdapterId(DX11AdapterId& Id, std::string& Error);
    ElaineCoreExport bool SupportsDX11TextureImport(uint32 Width, uint32 Height, std::string& Error);
    // Borrows the NT handle for the duration of the call. The caller closes it.
    ElaineCoreExport Elaine::RHITextureHandle ImportDX11Texture(
        const Elaine::RHITextureDesc& Desc, void* SharedHandle, std::string& Error);
    // RenderThread synchronization window only, before recording this frame.
    ElaineCoreExport void SetDX11TextureSubmission(const Elaine::RHITextureHandle& Texture,
        const std::shared_ptr<DX11InteropSubmission>& Submission);
}
