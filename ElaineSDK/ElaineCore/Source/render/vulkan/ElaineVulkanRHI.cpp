#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "render/ElaineWindowSystem.h"

namespace Elaine
{
    VulkanRHI::~VulkanRHI()
    {
        // TODO
    }

    void VulkanRHI::initialize( RHIInitInfo* init_info)
    {
        m_window = WindowSystem::instance()->createMainWindow(1920, 1080, init_info->windowname);
        if (m_window == nullptr)
        {
            LOG_INFO("vulkan init failed, window not create!");
            return;
        }
        Vector2 window_size = WindowSystem::instance()->getWindowSize(init_info->windowname);
        m_viewport = { 0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f };
        m_scissor = { {0, 0}, {(uint32_t)window_size[0], (uint32_t)window_size[1]} };

#ifndef NDEBUG
        m_enable_validation_Layers = true;
        m_enable_debug_utils_label = true;
#else
        m_enable_validation_Layers = false;
        m_enable_debug_utils_label = false;
#endif
        createInstance();
        initializeDebugMessenger();
        createWindowSurface();
        initializePhysicalDevice();
        createLogicalDevice();
        createCommandPool();
        createCommandBuffers();
        createDescriptorPool();
        createSyncPrimitives();
        createSwapchain();
        createSwapchainImageViews();
        createFramebufferImageAndView();
        createAssetAllocator();
    }

    void VulkanRHI::prepareContext()
    {
        m_vk_current_command_buffer = m_vk_command_buffers[m_current_frame_index];
        ((VulkanCommandBuffer*)m_current_command_buffer)->setResource(m_vk_current_command_buffer);
    }

    RHITYPE VulkanRHI::getRHIType()
    {
        return Vulkan;
    }

    bool VulkanRHI::allocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer*& pCommandBuffers)
    {
        return false;
    }
    bool VulkanRHI::allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet*& pDescriptorSets)
    {
        return false;
    }
    void VulkanRHI::createSwapchain()
    {

    }
    void VulkanRHI::recreateSwapchain()
    {

    }
    void VulkanRHI::createSwapchainImageViews()
    {

    }
    void VulkanRHI::createFramebufferImageAndView()
    {

    }
    RHISampler* VulkanRHI::getOrCreateDefaultSampler(RHIDefaultSamplerType type)
    {
        return nullptr;
    }
    RHISampler* VulkanRHI::getOrCreateMipmapSampler(uint32_t width, uint32_t height)
    {
        return nullptr;
    }
    RHIShader* VulkanRHI::createShaderModule(const std::vector<unsigned char>& shader_code)
    {
        return nullptr;
    }
    void VulkanRHI::createBuffer(RHIDeviceSize size, RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory)
    {

    }
    void VulkanRHI::createBufferAndInitialize(RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory, RHIDeviceSize size, void* data /*= nullptr*/, int datasize/* = 0*/)
    {

    }

    void VulkanRHI::copyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, RHIDeviceSize srcOffset, RHIDeviceSize dstOffset, RHIDeviceSize size)
    {

    }
    void VulkanRHI::createImage(uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
        RHIImage*& image, RHIDeviceMemory*& memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels)
    {

    }
    void VulkanRHI::createImageView(RHIImage* image, RHIFormat format, RHIImageAspectFlags image_aspect_flags, RHIImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
        RHIImageView*& image_view)
    {

    }
    //void createGlobalImage(RHIImage*& image, RHIImageView*& image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, void* texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels = 0) override;
    //void createCubeMap(RHIImage*& image, RHIImageView*& image_view, VmaAllocation& image_allocation, uint32_t texture_image_width, uint32_t texture_image_height, std::array<void*, 6> texture_image_pixels, RHIFormat texture_image_format, uint32_t miplevels) override;
    bool VulkanRHI::createCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool*& pCommandPool)
    {
        return false;
    }
    bool VulkanRHI::createDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool*& pDescriptorPool)
    {
        return false;
    }
    bool VulkanRHI::createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* pCreateInfo, RHIDescriptorSetLayout*& pSetLayout)
    {
        return false;
    }
    bool VulkanRHI::createFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence*& pFence)
    {
        return false;
    }
    bool VulkanRHI::createFramebuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer)
    {
        return false;
    }
    bool VulkanRHI::createGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines)
    {
        return false;
    }
    bool VulkanRHI::createComputePipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIComputePipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines)
    {
        return false;
    }
    bool VulkanRHI::createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout)
    {
        return false;
    }
    bool VulkanRHI::createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass)
    {
        return false;
    }
    bool VulkanRHI::createSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler*& pSampler)
    {
        return false;
    }
    bool VulkanRHI::createSemaphore(const RHISemaphoreCreateInfo* pCreateInfo, RHISemaphore*& pSemaphore)
    {
        return false;
    }
    // command and command write
    bool VulkanRHI::waitForFencesPFN(uint32_t fenceCount, RHIFence* const* pFence, RHIBool32 waitAll, uint64_t timeout)
    {
        return false;
    }
    bool VulkanRHI::resetFencesPFN(uint32_t fenceCount, RHIFence* const* pFences)
    {
        return false;
    }
    bool VulkanRHI::resetCommandPoolPFN(RHICommandPool* commandPool, RHICommandPoolResetFlags flags)
    {
        return false;
    }
    bool VulkanRHI::beginCommandBufferPFN(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        return false;
    }
    bool VulkanRHI::endCommandBufferPFN(RHICommandBuffer* commandBuffer)
    {
        return false;
    }
    void VulkanRHI::cmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents)
    {

    }
    void VulkanRHI::cmdNextSubpassPFN(RHICommandBuffer* commandBuffer, RHISubpassContents contents)
    {

    }
    void VulkanRHI::cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer)
    {

    }
    void VulkanRHI::cmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline)
    {

    }
    void VulkanRHI::cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports)
    {

    }
    void VulkanRHI::cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors)
    {

    }
    void VulkanRHI::cmdBindVertexBuffersPFN(
        RHICommandBuffer* commandBuffer,
        uint32_t firstBinding,
        uint32_t bindingCount,
        RHIBuffer* const* pBuffers,
        const RHIDeviceSize* pOffsets)
    {

    }
    void VulkanRHI::cmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType)
    {

    }
    void VulkanRHI::cmdBindDescriptorSetsPFN(
        RHICommandBuffer* commandBuffer,
        RHIPipelineBindPoint pipelineBindPoint,
        RHIPipelineLayout* layout,
        uint32_t firstSet,
        uint32_t descriptorSetCount,
        const RHIDescriptorSet* const* pDescriptorSets,
        uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets)
    {

    }
    void VulkanRHI::cmdDrawIndexedPFN(RHICommandBuffer* commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {

    }
    void VulkanRHI::cmdClearAttachmentsPFN(RHICommandBuffer* commandBuffer, uint32_t attachmentCount, const RHIClearAttachment* pAttachments, uint32_t rectCount, const RHIClearRect* pRects)
    {

    }

    bool VulkanRHI::beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        return false;
    }
    void VulkanRHI::cmdCopyImageToBuffer(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageLayout srcImageLayout, RHIBuffer* dstBuffer, uint32_t regionCount, const RHIBufferImageCopy* pRegions)
    {

    }
    void VulkanRHI::cmdCopyImageToImage(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageAspectFlagBits srcFlag, RHIImage* dstImage, RHIImageAspectFlagBits dstFlag, uint32_t width, uint32_t height)
    {

    }
    void VulkanRHI::cmdCopyBuffer(RHICommandBuffer* commandBuffer, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32_t regionCount, RHIBufferCopy* pRegions)
    {

    }
    void VulkanRHI::cmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {

    }
    void VulkanRHI::cmdDispatch(RHICommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {

    }
    void VulkanRHI::cmdDispatchIndirect(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset)
    {

    }
    void VulkanRHI::cmdPipelineBarrier(RHICommandBuffer* commandBuffer, RHIPipelineStageFlags srcStageMask, RHIPipelineStageFlags dstStageMask, RHIDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const RHIMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const RHIBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const RHIImageMemoryBarrier* pImageMemoryBarriers)
    {

    }
    bool VulkanRHI::endCommandBuffer(RHICommandBuffer* commandBuffer)
    {
        return false;
    }
    void VulkanRHI::updateDescriptorSets(uint32_t descriptorWriteCount, const RHIWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const RHICopyDescriptorSet* pDescriptorCopies)
    {

    }
    bool VulkanRHI::queueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence)
    {
        return false;
    }
    bool VulkanRHI::queueWaitIdle(RHIQueue* queue)
    {
        return false;
    }

    void VulkanRHI::clear()
    {
        if (m_enable_validation_Layers)
        {
            destroyDebugUtilsMessengerEXT(m_instance, m_debug_messenger, nullptr);
        }

    }

    void VulkanRHI::waitForFences()
    {
        VkResult res_wait_for_fences =
            _vkWaitForFences(m_device, 1, &m_is_frame_in_flight_fences[m_current_frame_index], VK_TRUE, UINT64_MAX);
        if (VK_SUCCESS != res_wait_for_fences)
        {
            LOG_ERROR("failed to synchronize!");
        }
    }

    bool VulkanRHI::waitForFences(uint32_t fenceCount, const RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout)
    {
        //fence
        int fence_size = fenceCount;
        std::vector<VkFence> vk_fence_list(fence_size);
        for (int i = 0; i < fence_size; ++i)
        {
            const auto& rhi_fence_element = pFences[i];
            auto& vk_fence_element = vk_fence_list[i];

            vk_fence_element = ((VulkanFence*)rhi_fence_element)->getResource();
        };

        VkResult result = vkWaitForFences(m_device, fenceCount, vk_fence_list.data(), waitAll, timeout);

        if (result == VK_SUCCESS)
        {
            return RHI_SUCCESS;
        }
        else
        {
            LOG_ERROR("waitForFences failed");
            return false;
        }
    }

    void VulkanRHI::getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties)
    {
        VkPhysicalDeviceProperties vk_physical_device_properties;
        vkGetPhysicalDeviceProperties(m_physical_device, &vk_physical_device_properties);

        pProperties->apiVersion = vk_physical_device_properties.apiVersion;
        pProperties->driverVersion = vk_physical_device_properties.driverVersion;
        pProperties->vendorID = vk_physical_device_properties.vendorID;
        pProperties->deviceID = vk_physical_device_properties.deviceID;
        pProperties->deviceType = (RHIPhysicalDeviceType)vk_physical_device_properties.deviceType;
        for (uint32_t i = 0; i < RHI_MAX_PHYSICAL_DEVICE_NAME_SIZE; i++)
        {
            pProperties->deviceName[i] = vk_physical_device_properties.deviceName[i];
        }
        for (uint32_t i = 0; i < RHI_UUID_SIZE; i++)
        {
            pProperties->pipelineCacheUUID[i] = vk_physical_device_properties.pipelineCacheUUID[i];
        }
        pProperties->sparseProperties.residencyStandard2DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DBlockShape;
        pProperties->sparseProperties.residencyStandard2DMultisampleBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard2DMultisampleBlockShape;
        pProperties->sparseProperties.residencyStandard3DBlockShape = (VkBool32)vk_physical_device_properties.sparseProperties.residencyStandard3DBlockShape;
        pProperties->sparseProperties.residencyAlignedMipSize = (VkBool32)vk_physical_device_properties.sparseProperties.residencyAlignedMipSize;
        pProperties->sparseProperties.residencyNonResidentStrict = (VkBool32)vk_physical_device_properties.sparseProperties.residencyNonResidentStrict;

        pProperties->limits.maxImageDimension1D = vk_physical_device_properties.limits.maxImageDimension1D;
        pProperties->limits.maxImageDimension2D = vk_physical_device_properties.limits.maxImageDimension2D;
        pProperties->limits.maxImageDimension3D = vk_physical_device_properties.limits.maxImageDimension3D;
        pProperties->limits.maxImageDimensionCube = vk_physical_device_properties.limits.maxImageDimensionCube;
        pProperties->limits.maxImageArrayLayers = vk_physical_device_properties.limits.maxImageArrayLayers;
        pProperties->limits.maxTexelBufferElements = vk_physical_device_properties.limits.maxTexelBufferElements;
        pProperties->limits.maxUniformBufferRange = vk_physical_device_properties.limits.maxUniformBufferRange;
        pProperties->limits.maxStorageBufferRange = vk_physical_device_properties.limits.maxStorageBufferRange;
        pProperties->limits.maxPushConstantsSize = vk_physical_device_properties.limits.maxPushConstantsSize;
        pProperties->limits.maxMemoryAllocationCount = vk_physical_device_properties.limits.maxMemoryAllocationCount;
        pProperties->limits.maxSamplerAllocationCount = vk_physical_device_properties.limits.maxSamplerAllocationCount;
        pProperties->limits.bufferImageGranularity = (VkDeviceSize)vk_physical_device_properties.limits.bufferImageGranularity;
        pProperties->limits.sparseAddressSpaceSize = (VkDeviceSize)vk_physical_device_properties.limits.sparseAddressSpaceSize;
        pProperties->limits.maxBoundDescriptorSets = vk_physical_device_properties.limits.maxBoundDescriptorSets;
        pProperties->limits.maxPerStageDescriptorSamplers = vk_physical_device_properties.limits.maxPerStageDescriptorSamplers;
        pProperties->limits.maxPerStageDescriptorUniformBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorUniformBuffers;
        pProperties->limits.maxPerStageDescriptorStorageBuffers = vk_physical_device_properties.limits.maxPerStageDescriptorStorageBuffers;
        pProperties->limits.maxPerStageDescriptorSampledImages = vk_physical_device_properties.limits.maxPerStageDescriptorSampledImages;
        pProperties->limits.maxPerStageDescriptorStorageImages = vk_physical_device_properties.limits.maxPerStageDescriptorStorageImages;
        pProperties->limits.maxPerStageDescriptorInputAttachments = vk_physical_device_properties.limits.maxPerStageDescriptorInputAttachments;
        pProperties->limits.maxPerStageResources = vk_physical_device_properties.limits.maxPerStageResources;
        pProperties->limits.maxDescriptorSetSamplers = vk_physical_device_properties.limits.maxDescriptorSetSamplers;
        pProperties->limits.maxDescriptorSetUniformBuffers = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffers;
        pProperties->limits.maxDescriptorSetUniformBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetUniformBuffersDynamic;
        pProperties->limits.maxDescriptorSetStorageBuffers = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffers;
        pProperties->limits.maxDescriptorSetStorageBuffersDynamic = vk_physical_device_properties.limits.maxDescriptorSetStorageBuffersDynamic;
        pProperties->limits.maxDescriptorSetSampledImages = vk_physical_device_properties.limits.maxDescriptorSetSampledImages;
        pProperties->limits.maxDescriptorSetStorageImages = vk_physical_device_properties.limits.maxDescriptorSetStorageImages;
        pProperties->limits.maxDescriptorSetInputAttachments = vk_physical_device_properties.limits.maxDescriptorSetInputAttachments;
        pProperties->limits.maxVertexInputAttributes = vk_physical_device_properties.limits.maxVertexInputAttributes;
        pProperties->limits.maxVertexInputBindings = vk_physical_device_properties.limits.maxVertexInputBindings;
        pProperties->limits.maxVertexInputAttributeOffset = vk_physical_device_properties.limits.maxVertexInputAttributeOffset;
        pProperties->limits.maxVertexInputBindingStride = vk_physical_device_properties.limits.maxVertexInputBindingStride;
        pProperties->limits.maxVertexOutputComponents = vk_physical_device_properties.limits.maxVertexOutputComponents;
        pProperties->limits.maxTessellationGenerationLevel = vk_physical_device_properties.limits.maxTessellationGenerationLevel;
        pProperties->limits.maxTessellationPatchSize = vk_physical_device_properties.limits.maxTessellationPatchSize;
        pProperties->limits.maxTessellationControlPerVertexInputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexInputComponents;
        pProperties->limits.maxTessellationControlPerVertexOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerVertexOutputComponents;
        pProperties->limits.maxTessellationControlPerPatchOutputComponents = vk_physical_device_properties.limits.maxTessellationControlPerPatchOutputComponents;
        pProperties->limits.maxTessellationControlTotalOutputComponents = vk_physical_device_properties.limits.maxTessellationControlTotalOutputComponents;
        pProperties->limits.maxTessellationEvaluationInputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationInputComponents;
        pProperties->limits.maxTessellationEvaluationOutputComponents = vk_physical_device_properties.limits.maxTessellationEvaluationOutputComponents;
        pProperties->limits.maxGeometryShaderInvocations = vk_physical_device_properties.limits.maxGeometryShaderInvocations;
        pProperties->limits.maxGeometryInputComponents = vk_physical_device_properties.limits.maxGeometryInputComponents;
        pProperties->limits.maxGeometryOutputComponents = vk_physical_device_properties.limits.maxGeometryOutputComponents;
        pProperties->limits.maxGeometryOutputVertices = vk_physical_device_properties.limits.maxGeometryOutputVertices;
        pProperties->limits.maxGeometryTotalOutputComponents = vk_physical_device_properties.limits.maxGeometryTotalOutputComponents;
        pProperties->limits.maxFragmentInputComponents = vk_physical_device_properties.limits.maxFragmentInputComponents;
        pProperties->limits.maxFragmentOutputAttachments = vk_physical_device_properties.limits.maxFragmentOutputAttachments;
        pProperties->limits.maxFragmentDualSrcAttachments = vk_physical_device_properties.limits.maxFragmentDualSrcAttachments;
        pProperties->limits.maxFragmentCombinedOutputResources = vk_physical_device_properties.limits.maxFragmentCombinedOutputResources;
        pProperties->limits.maxComputeSharedMemorySize = vk_physical_device_properties.limits.maxComputeSharedMemorySize;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupCount[i] = vk_physical_device_properties.limits.maxComputeWorkGroupCount[i];
        }
        pProperties->limits.maxComputeWorkGroupInvocations = vk_physical_device_properties.limits.maxComputeWorkGroupInvocations;
        for (uint32_t i = 0; i < 3; i++)
        {
            pProperties->limits.maxComputeWorkGroupSize[i] = vk_physical_device_properties.limits.maxComputeWorkGroupSize[i];
        }
        pProperties->limits.subPixelPrecisionBits = vk_physical_device_properties.limits.subPixelPrecisionBits;
        pProperties->limits.subTexelPrecisionBits = vk_physical_device_properties.limits.subTexelPrecisionBits;
        pProperties->limits.mipmapPrecisionBits = vk_physical_device_properties.limits.mipmapPrecisionBits;
        pProperties->limits.maxDrawIndexedIndexValue = vk_physical_device_properties.limits.maxDrawIndexedIndexValue;
        pProperties->limits.maxDrawIndirectCount = vk_physical_device_properties.limits.maxDrawIndirectCount;
        pProperties->limits.maxSamplerLodBias = vk_physical_device_properties.limits.maxSamplerLodBias;
        pProperties->limits.maxSamplerAnisotropy = vk_physical_device_properties.limits.maxSamplerAnisotropy;
        pProperties->limits.maxViewports = vk_physical_device_properties.limits.maxViewports;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.maxViewportDimensions[i] = vk_physical_device_properties.limits.maxViewportDimensions[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.viewportBoundsRange[i] = vk_physical_device_properties.limits.viewportBoundsRange[i];
        }
        pProperties->limits.viewportSubPixelBits = vk_physical_device_properties.limits.viewportSubPixelBits;
        pProperties->limits.minMemoryMapAlignment = vk_physical_device_properties.limits.minMemoryMapAlignment;
        pProperties->limits.minTexelBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minTexelBufferOffsetAlignment;
        pProperties->limits.minUniformBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minUniformBufferOffsetAlignment;
        pProperties->limits.minStorageBufferOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.minStorageBufferOffsetAlignment;
        pProperties->limits.minTexelOffset = vk_physical_device_properties.limits.minTexelOffset;
        pProperties->limits.maxTexelOffset = vk_physical_device_properties.limits.maxTexelOffset;
        pProperties->limits.minTexelGatherOffset = vk_physical_device_properties.limits.minTexelGatherOffset;
        pProperties->limits.maxTexelGatherOffset = vk_physical_device_properties.limits.maxTexelGatherOffset;
        pProperties->limits.minInterpolationOffset = vk_physical_device_properties.limits.minInterpolationOffset;
        pProperties->limits.maxInterpolationOffset = vk_physical_device_properties.limits.maxInterpolationOffset;
        pProperties->limits.subPixelInterpolationOffsetBits = vk_physical_device_properties.limits.subPixelInterpolationOffsetBits;
        pProperties->limits.maxFramebufferWidth = vk_physical_device_properties.limits.maxFramebufferWidth;
        pProperties->limits.maxFramebufferHeight = vk_physical_device_properties.limits.maxFramebufferHeight;
        pProperties->limits.maxFramebufferLayers = vk_physical_device_properties.limits.maxFramebufferLayers;
        pProperties->limits.framebufferColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferColorSampleCounts;
        pProperties->limits.framebufferDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferDepthSampleCounts;
        pProperties->limits.framebufferStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferStencilSampleCounts;
        pProperties->limits.framebufferNoAttachmentsSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.framebufferNoAttachmentsSampleCounts;
        pProperties->limits.maxColorAttachments = vk_physical_device_properties.limits.maxColorAttachments;
        pProperties->limits.sampledImageColorSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageColorSampleCounts;
        pProperties->limits.sampledImageIntegerSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageIntegerSampleCounts;
        pProperties->limits.sampledImageDepthSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageDepthSampleCounts;
        pProperties->limits.sampledImageStencilSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.sampledImageStencilSampleCounts;
        pProperties->limits.storageImageSampleCounts = (VkSampleCountFlags)vk_physical_device_properties.limits.storageImageSampleCounts;
        pProperties->limits.maxSampleMaskWords = vk_physical_device_properties.limits.maxSampleMaskWords;
        pProperties->limits.timestampComputeAndGraphics = (VkBool32)vk_physical_device_properties.limits.timestampComputeAndGraphics;
        pProperties->limits.timestampPeriod = vk_physical_device_properties.limits.timestampPeriod;
        pProperties->limits.maxClipDistances = vk_physical_device_properties.limits.maxClipDistances;
        pProperties->limits.maxCullDistances = vk_physical_device_properties.limits.maxCullDistances;
        pProperties->limits.maxCombinedClipAndCullDistances = vk_physical_device_properties.limits.maxCombinedClipAndCullDistances;
        pProperties->limits.discreteQueuePriorities = vk_physical_device_properties.limits.discreteQueuePriorities;
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.pointSizeRange[i] = vk_physical_device_properties.limits.pointSizeRange[i];
        }
        for (uint32_t i = 0; i < 2; i++)
        {
            pProperties->limits.lineWidthRange[i] = vk_physical_device_properties.limits.lineWidthRange[i];
        }
        pProperties->limits.pointSizeGranularity = vk_physical_device_properties.limits.pointSizeGranularity;
        pProperties->limits.lineWidthGranularity = vk_physical_device_properties.limits.lineWidthGranularity;
        pProperties->limits.strictLines = (VkBool32)vk_physical_device_properties.limits.strictLines;
        pProperties->limits.standardSampleLocations = (VkBool32)vk_physical_device_properties.limits.standardSampleLocations;
        pProperties->limits.optimalBufferCopyOffsetAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyOffsetAlignment;
        pProperties->limits.optimalBufferCopyRowPitchAlignment = (VkDeviceSize)vk_physical_device_properties.limits.optimalBufferCopyRowPitchAlignment;
        pProperties->limits.nonCoherentAtomSize = (VkDeviceSize)vk_physical_device_properties.limits.nonCoherentAtomSize;

    }

    RHICommandBuffer* VulkanRHI::getCurrentCommandBuffer() const
    {
        return nullptr;
    }
    RHICommandBuffer* const* VulkanRHI::getCommandBufferList() const
    {
        return nullptr;
    }
    RHICommandPool* VulkanRHI::getCommandPoor() const
    {
        return nullptr;
    }
    RHIDescriptorPool* VulkanRHI::getDescriptorPoor()const
    {
        return nullptr;
    }
    RHIFence* const* VulkanRHI::getFenceList() const
    {
        return nullptr;
    }
    QueueFamilyIndices VulkanRHI::getQueueFamilyIndices() const
    {
        return {};
    }
    RHIQueue* VulkanRHI::getGraphicsQueue() const
    {
        return nullptr;
    }
    RHIQueue* VulkanRHI::getComputeQueue() const
    {
        return nullptr;
    }
    RHISwapChainDesc VulkanRHI::getSwapchainInfo()
    {
        return {};
    }
    RHIDepthImageDesc VulkanRHI::getDepthImageInfo() const
    {
        return {};
    }
    uint8_t VulkanRHI::getMaxFramesInFlight() const
    {
        return 0;
    }
    uint8_t VulkanRHI::getCurrentFrameIndex() const
    {
        return 0;
    }
    void VulkanRHI::setCurrentFrameIndex(uint8_t index)
    {
        
    }

    // command write
    RHICommandBuffer* VulkanRHI::beginSingleTimeCommands()
    {
        return nullptr;
    }
    void VulkanRHI::endSingleTimeCommands(RHICommandBuffer* command_buffer)
    {

    }
    bool VulkanRHI::prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        return false;
    }
    void VulkanRHI::submitRendering(std::function<void()> passUpdateAfterRecreateSwapchain)
    {

    }
    void VulkanRHI::pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color)
    {

    }
    void VulkanRHI::popEvent(RHICommandBuffer* commond_buffer)
    {

    }

    void VulkanRHI::resetCommandPool()
    {
        VkResult res_reset_command_pool = _vkResetCommandPool(m_device, m_command_pools[m_current_frame_index], 0);
        if (VK_SUCCESS != res_reset_command_pool)
        {
            LOG_ERROR("failed to synchronize");
        }
    }

    void VulkanRHI::clearSwapchain()
    {

    }
    void VulkanRHI::destroyDefaultSampler(RHIDefaultSamplerType type)
    {

    }
    void VulkanRHI::destroyMipmappedSampler()
    {

    }
    void VulkanRHI::destroyShaderModule(RHIShader* shader)
    {

    }
    void VulkanRHI::destroySemaphore(RHISemaphore* semaphore)
    {

    }
    void VulkanRHI::destroySampler(RHISampler* sampler)
    {

    }
    void VulkanRHI::destroyInstance(RHIInstance* instance)
    {

    }
    void VulkanRHI::destroyImageView(RHIImageView* imageView)
    {

    }
    void VulkanRHI::destroyImage(RHIImage* image)
    {

    }
    void VulkanRHI::destroyFramebuffer(RHIFramebuffer* framebuffer)
    {

    }
    void VulkanRHI::destroyFence(RHIFence* fence)
    {

    }
    void VulkanRHI::destroyDevice()
    {

    }
    void VulkanRHI::destroyCommandPool(RHICommandPool* commandPool)
    {

    }
    void VulkanRHI::destroyBuffer(RHIBuffer*& buffer)
    {

    }
    void VulkanRHI::freeCommandBuffers(RHICommandPool* commandPool, uint32_t commandBufferCount, RHICommandBuffer* pCommandBuffers)
    {

    }

    // memory
    void VulkanRHI::freeMemory(RHIDeviceMemory*& memory)
    {

    }
    bool VulkanRHI::mapMemory(RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size, RHIMemoryMapFlags flags, void** ppData)
    {
        return false;
    }
    void VulkanRHI::unmapMemory(RHIDeviceMemory* memory)
    {

    }
    void VulkanRHI::invalidateMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {

    }
    void VulkanRHI::flushMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {

    }

    //semaphores
    RHISemaphore*& VulkanRHI::getTextureCopySemaphore(uint32_t index)
    {
        return m_image_available_for_texturescopy_semaphores[index];
    }

    void VulkanRHI::createInstance()
    {

    }
    void VulkanRHI::initializeDebugMessenger()
    {

    }
    void VulkanRHI::createWindowSurface()
    {

    }
    void VulkanRHI::initializePhysicalDevice()
    {

    }
    void VulkanRHI::createLogicalDevice()
    {

    }
    void VulkanRHI::createCommandPool()
    {

    }
    void VulkanRHI::createCommandBuffers()
    {

    }
    void VulkanRHI::createDescriptorPool()
    {

    }
    void VulkanRHI::createSyncPrimitives()
    {

    }
    void VulkanRHI::createAssetAllocator()
    {

    }
    bool VulkanRHI::isPointLightShadowEnabled()
    {
        return false;
    }
    bool VulkanRHI::checkValidationLayerSupport()
    {
        return false;
    }
    std::vector<const char*> VulkanRHI::getRequiredExtensions()
    {
        return {};
    }
    void VulkanRHI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {

    }

    VkResult VulkanRHI::createDebugUtilsMessengerEXT(VkInstance                                instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        return {};
    }
    void VulkanRHI::destroyDebugUtilsMessengerEXT(VkInstance                   instance,
        VkDebugUtilsMessengerEXT     debugMessenger,
        const VkAllocationCallbacks* pAllocator)
    {

    }

    QueueFamilyIndices VulkanRHI::findQueueFamilies(VkPhysicalDevice physical_device)
    {
        return {};
    }
    bool VulkanRHI::checkDeviceExtensionSupport(VkPhysicalDevice physical_device)
    {
        return false;
    }
    bool VulkanRHI::isDeviceSuitable(VkPhysicalDevice physical_device)
    {
        return false;
    }
    SwapChainSupportDetails VulkanRHI::querySwapChainSupport(VkPhysicalDevice physical_device)
    {
        return {};
    }

    VkFormat VulkanRHI::findDepthFormat()
    {
        return {};
    }
    VkFormat VulkanRHI::findSupportedFormat(const std::vector<VkFormat>& candidates,
        VkImageTiling                tiling,
        VkFormatFeatureFlags         features)
    {
        return {};
    }

    VkSurfaceFormatKHR
        VulkanRHI::chooseSwapchainSurfaceFormatFromDetails(const std::vector<VkSurfaceFormatKHR>& available_surface_formats)
    {
        return {};
    }
    VkPresentModeKHR
        VulkanRHI::chooseSwapchainPresentModeFromDetails(const std::vector<VkPresentModeKHR>& available_present_modes)
    {
        return {};
    }
    VkExtent2D VulkanRHI::chooseSwapchainExtentFromDetails(const VkSurfaceCapabilitiesKHR& capabilities)
    {
        return {};
    }
}