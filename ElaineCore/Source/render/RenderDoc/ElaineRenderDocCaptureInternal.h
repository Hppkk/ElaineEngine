#pragma once
#include <string>

namespace Elaine::RenderDocInternal
{
    bool IsConnected();
    void VulkanReady();
    void VulkanUnavailable(const std::string& Reason);
    void BeginFrame(void* Device);
    void EndFrame();
    void AbortFrame(const char* Reason);
    void DetachVulkan();
}
