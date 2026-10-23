#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanDX11Interop.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanQueue.h"

namespace VulkanRHI
{
    namespace
    {
        constexpr VkImageUsageFlags SharedUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        void Require(VkResult Result, const char* Operation)
        {
            if (Result != VK_SUCCESS) throw std::runtime_error(std::string(Operation) + ": " + std::to_string(Result));
        }
    }

    bool GetDX11AdapterId(DX11AdapterId& Id, std::string& Error)
    {
        auto* RHI = GetVulkanDynamicRHI();
        if (!RHI || !RHI->GetPhyDevice()) { Error = "Vulkan device is not initialized"; return false; }
        VkPhysicalDeviceIDProperties Identity{};
        Identity.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
        VkPhysicalDeviceProperties2 Properties{};
        Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2; Properties.pNext = &Identity;
        vkGetPhysicalDeviceProperties2(RHI->GetPhyDevice()->GetPhysicalDeviceHandle(), &Properties);
        if (!Identity.deviceLUIDValid) { Error = "Vulkan device does not expose a valid Windows adapter LUID"; return false; }
        static_assert(sizeof(Id) == VK_LUID_SIZE, "Adapter LUID size");
        memcpy(&Id, Identity.deviceLUID, VK_LUID_SIZE);
        return true;
    }

    bool SupportsDX11TextureImport(uint32 Width, uint32 Height, std::string& Error)
    {
        auto* RHI = GetVulkanDynamicRHI();
        if (!RHI || !RHI->GetDevice() || !RHI->GetDevice()->SupportsDX11KeyedMutex())
        { Error = "VK_KHR_win32_keyed_mutex is not enabled on the selected Vulkan device"; return false; }
        VkPhysicalDeviceExternalImageFormatInfo External{};
        External.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
        External.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
        VkPhysicalDeviceImageFormatInfo2 Info{};
        Info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2; Info.pNext = &External;
        Info.format = VK_FORMAT_R8G8B8A8_UNORM; Info.type = VK_IMAGE_TYPE_2D;
        Info.tiling = VK_IMAGE_TILING_OPTIMAL; Info.usage = SharedUsage;
        VkExternalImageFormatProperties ExternalProperties{};
        ExternalProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;
        VkImageFormatProperties2 Properties{};
        Properties.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2; Properties.pNext = &ExternalProperties;
        const auto Result = vkGetPhysicalDeviceImageFormatProperties2(RHI->GetPhyDevice()->GetPhysicalDeviceHandle(), &Info, &Properties);
        const auto& Memory = ExternalProperties.externalMemoryProperties;
        if (Result != VK_SUCCESS || !(Memory.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) ||
            !(Memory.compatibleHandleTypes & VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT))
        { Error = "RGBA8 render target cannot import a D3D11 NT texture on the selected Vulkan device (" + std::to_string(Result) + ")"; return false; }
        if (!Width || !Height || Width > Properties.imageFormatProperties.maxExtent.width || Height > Properties.imageFormatProperties.maxExtent.height)
        { Error = "Shared texture extent is unsupported"; return false; }
        return true;
    }

    Elaine::RHITextureHandle ImportDX11Texture(const Elaine::RHITextureDesc& Desc, void* SharedHandle, std::string& Error)
    {
        Elaine::CheckRHIContextThread();
        if (!SharedHandle || !SupportsDX11TextureImport(static_cast<uint32>(Desc.mExtent.x), static_cast<uint32>(Desc.mExtent.y), Error)) return {};
        if (Desc.mFormat != PF_R8G8B8A8 || Desc.mDimension != TextureDimension::Texture2D ||
            Desc.mNumMips != 1 || Desc.mNumSamples != 1 || Desc.mArraySize != 1 || EnumHasAnyFlags(Desc.mFlags, TextureCreateFlags::SRGB))
        { Error = "D3D11 import requires RGBA8 UNORM 2D, one layer/mip/sample"; return {}; }
        try { return Elaine::RHITextureHandle(new VulkanTexture(GetVulkanDynamicRHI()->GetDevice(), Desc, SharedHandle)); }
        catch (const std::exception& Failure) { Error = Failure.what(); return {}; }
    }

    void SetDX11TextureSubmission(const Elaine::RHITextureHandle& Texture, const std::shared_ptr<DX11InteropSubmission>& Submission)
    {
        Elaine::CheckRHIContextThread();
        auto* Native = static_cast<VulkanTexture*>(Texture.get());
        if (!Native || !Native->IsDX11Imported()) throw std::logic_error("Not a D3D11 imported texture");
        Native->mDX11Submission = Submission;
    }

    VulkanTexture::VulkanTexture(VulkanDevice* Device, const RHITextureDesc& Desc, void* DX11Handle)
        : RHITexture(Desc), mDevice(Device), mViewFormat(VK_FORMAT_R8G8B8A8_UNORM), mStorageFormat(VK_FORMAT_R8G8B8A8_UNORM),
          mMemProps(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT), mTiling(VK_IMAGE_TILING_OPTIMAL),
          mFullAspectMask(VK_IMAGE_ASPECT_COLOR_BIT), mPartialAspectMask(VK_IMAGE_ASPECT_COLOR_BIT), mImageLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    {
        try
        {
            VkExternalMemoryImageCreateInfo External{};
            External.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
            External.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;
            VkImageCreateInfo Image{};
            Image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; Image.pNext = &External;
            Image.imageType = VK_IMAGE_TYPE_2D; Image.format = mStorageFormat;
            Image.extent = {static_cast<uint32>(Desc.mExtent.x), static_cast<uint32>(Desc.mExtent.y), 1};
            Image.mipLevels = 1; Image.arrayLayers = 1; Image.samples = VK_SAMPLE_COUNT_1_BIT;
            Image.tiling = mTiling; Image.usage = SharedUsage; Image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            Image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            Require(vkCreateImage(Device->GetDevice(), &Image, VULKAN_CPU_ALLOCATOR, &mHandle), "Create imported image");
            vkGetImageMemoryRequirements(Device->GetDevice(), mHandle, &mMemRequirements);
            VkMemoryWin32HandlePropertiesKHR HandleProperties{};
            HandleProperties.sType = VK_STRUCTURE_TYPE_MEMORY_WIN32_HANDLE_PROPERTIES_KHR;
            Require(vkGetMemoryWin32HandlePropertiesKHR(Device->GetDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
                static_cast<HANDLE>(DX11Handle), &HandleProperties), "Query D3D11 memory types");
            uint32 Type = 0;
            Require(Device->GetDeviceMemoryManager()->GetMemoryTypeFromProperties(mMemRequirements.memoryTypeBits & HandleProperties.memoryTypeBits,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &Type), "Find imported memory type");
            VkMemoryDedicatedAllocateInfo Dedicated{};
            Dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO; Dedicated.image = mHandle;
            VkImportMemoryWin32HandleInfoKHR Import{};
            Import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
            Import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT; Import.handle = static_cast<HANDLE>(DX11Handle);
            mImportedAllocation = Device->GetDeviceMemoryManager()->Alloc(true, mMemRequirements.size, Type, &Dedicated,
                1.0f, false, __FILE__, __LINE__, static_cast<VkExternalMemoryHandleTypeFlagBits>(0), &Import);
            if (!mImportedAllocation) throw std::runtime_error("Cannot allocate imported D3D11 texture memory");
            Require(vkBindImageMemory(Device->GetDevice(), mHandle, mImportedAllocation->GetHandle(), 0), "Bind imported image");
            mDefaultView.Create(*Device, mHandle, VK_IMAGE_VIEW_TYPE_2D, mFullAspectMask, Desc.mFormat, mViewFormat, 0, 1, 0, 1, true);
            if (!mDefaultView.mView) throw std::runtime_error("Create imported image view failed");
            mPartialView = &mDefaultView; mAccess = ERHIAccess::Unknown;
        }
        catch (...)
        {
            if (mDefaultView.mView) mDefaultView.Destroy(*Device);
            if (mHandle) vkDestroyImage(Device->GetDevice(), mHandle, VULKAN_CPU_ALLOCATOR);
            mHandle = VK_NULL_HANDLE;
            if (mImportedAllocation) Device->GetDeviceMemoryManager()->Free(mImportedAllocation);
            throw;
        }
    }

    void VulkanCommandBuffer::AcquireDX11Texture(VulkanTexture* Texture)
    {
        if (!Texture->IsDX11Imported()) return;
        for (const auto& Access : mDX11Accesses) if (Access.Texture.get() == Texture) return;
        if (mDX11Released) throw std::logic_error("Shared texture used after external release");
        auto Submission = Texture->mDX11Submission;
        auto Expected = DX11SubmissionState::Reserved;
        if (!Submission || !Submission->State.compare_exchange_strong(Expected, DX11SubmissionState::Recording))
            throw std::logic_error("Shared texture was not reserved for this submission");
        mDX11Accesses.push_back({Elaine::RHITextureHandle(Texture), Submission});
        VkImageMemoryBarrier Barrier{};
        Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        Barrier.oldLayout = Texture->GetImageLayout(); Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        Barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL; Barrier.dstQueueFamilyIndex = mDevice->GetGraphicQueue()->GetFamilyIndex();
        Barrier.image = Texture->getHandle(); Barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        Barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        vkCmdPipelineBarrier(mHandle, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
        Texture->SetImageLayout(VK_IMAGE_LAYOUT_GENERAL); Texture->SetAccess(ERHIAccess::Unknown);
    }

    void VulkanCommandBuffer::ReleaseDX11Textures()
    {
        if (mDX11Released) return;
        for (const auto& Access : mDX11Accesses)
        {
            auto* Texture = static_cast<VulkanTexture*>(Access.Texture.get());
            VkImageMemoryBarrier Barrier{};
            Barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            Barrier.oldLayout = Texture->GetImageLayout(); Barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
            Barrier.srcQueueFamilyIndex = mDevice->GetGraphicQueue()->GetFamilyIndex(); Barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_EXTERNAL;
            Barrier.image = Texture->getHandle(); Barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
            Barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            vkCmdPipelineBarrier(mHandle, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &Barrier);
            Texture->SetImageLayout(VK_IMAGE_LAYOUT_GENERAL); Texture->SetAccess(ERHIAccess::Unknown);
        }
        mDX11Released = !mDX11Accesses.empty();
    }

    void VulkanCommandBuffer::NotifyDX11Submission(bool Succeeded)
    {
        for (const auto& Access : mDX11Accesses)
            Access.Submission->State.store(Succeeded ? DX11SubmissionState::Submitted : DX11SubmissionState::Failed);
    }
}
