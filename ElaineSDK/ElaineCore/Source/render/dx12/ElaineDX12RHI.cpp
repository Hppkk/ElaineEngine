#include "ElainePrecompiledHeader.h"
#include "render/dx12/ElaineDX12RHI.h"

namespace Elaine
{
    Dx12RHI::~Dx12RHI()
    {

    }

    void Dx12RHI::initialize(RHIInitInfo init_info)
    {

    }

    void Dx12RHI::prepareContext()
    {

    }

    // allocate and create
    bool Dx12RHI::allocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer*& pCommandBuffers)
    {
        return true;
    }
    bool Dx12RHI::allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet*& pDescriptorSets)
    {
        return true;
    }
    void Dx12RHI::createSwapchain()
    {

    }
    void Dx12RHI::recreateSwapchain()
    {

    }
    void Dx12RHI::createSwapchainImageViews()
    {

    }
    void Dx12RHI::createFramebufferImageAndView()
    {

    }
    RHISampler* Dx12RHI::getOrCreateDefaultSampler(RHIDefaultSamplerType type)
    {
        return nullptr;
    }
    RHISampler* Dx12RHI::getOrCreateMipmapSampler(uint32_t width, uint32_t height)
    {
        return nullptr;

    }
    RHIShader* Dx12RHI::createShaderModule(const std::vector<unsigned char>& shader_code)
    {
        return nullptr;

    }
    void Dx12RHI::createBuffer(RHIDeviceSize size, RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory)
    {

    }
    void Dx12RHI::createBufferAndInitialize(RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory, RHIDeviceSize size, void* data/* = nullptr*/, int datasize /*= 0*/)
    {

    }

    void Dx12RHI::copyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, RHIDeviceSize srcOffset, RHIDeviceSize dstOffset, RHIDeviceSize size)
    {

    }
    void Dx12RHI::createImage(uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
        RHIImage*& image, RHIDeviceMemory*& memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels)
    {

    }
    void Dx12RHI::createImageView(RHIImage* image, RHIFormat format, RHIImageAspectFlags image_aspect_flags, RHIImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
        RHIImageView*& image_view)
    {

    }

    bool Dx12RHI::createCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool*& pCommandPool)
    {
        return true;
    }
    bool Dx12RHI::createDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool*& pDescriptorPool)
    {
        return true;

    }
    bool Dx12RHI::createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* pCreateInfo, RHIDescriptorSetLayout*& pSetLayout)
    {
        return true;

    }
    bool Dx12RHI::createFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence*& pFence)
    {
        return true;

    }
    bool Dx12RHI::createFramebuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer)
    {
        return true;

    }
    bool Dx12RHI::createGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines)
    {
        return true;

    }
    bool Dx12RHI::createComputePipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIComputePipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines)
    {
        return true;

    }
    bool Dx12RHI::createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout)
    {
        return true;

    }
    bool Dx12RHI::createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass)
    {
        return true;

    }
    bool Dx12RHI::createSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler*& pSampler)
    {
        return true;

    }
    bool Dx12RHI::createSemaphore(const RHISemaphoreCreateInfo* pCreateInfo, RHISemaphore*& pSemaphore)
    {
        return true;

    }

    // command and command write
    bool Dx12RHI::waitForFencesPFN(uint32_t fenceCount, RHIFence* const* pFence, RHIBool32 waitAll, uint64_t timeout)
    {
        return true;

    }
    bool Dx12RHI::resetFencesPFN(uint32_t fenceCount, RHIFence* const* pFences)
    {
        return true;

    }
    bool Dx12RHI::resetCommandPoolPFN(RHICommandPool* commandPool, RHICommandPoolResetFlags flags)
    {
        return true;

    }
    bool Dx12RHI::beginCommandBufferPFN(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        return true;

    }
    bool Dx12RHI::endCommandBufferPFN(RHICommandBuffer* commandBuffer)
    {
        return true;

    }
    void Dx12RHI::cmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents)
    {

    }
    void Dx12RHI::cmdNextSubpassPFN(RHICommandBuffer* commandBuffer, RHISubpassContents contents)
    {

    }
    void Dx12RHI::cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer)
    {

    }
    void Dx12RHI::cmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline)
    {

    }
    void Dx12RHI::cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports)
    {

    }
    void Dx12RHI::cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors)
    {

    }
    void Dx12RHI::cmdBindVertexBuffersPFN(
        RHICommandBuffer* commandBuffer,
        uint32_t firstBinding,
        uint32_t bindingCount,
        RHIBuffer* const* pBuffers,
        const RHIDeviceSize* pOffsets)
    {

    }
    void Dx12RHI::cmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType)
    {

    }
    void Dx12RHI::cmdBindDescriptorSetsPFN(
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
    void Dx12RHI::cmdDrawIndexedPFN(RHICommandBuffer* commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {

    }
    void Dx12RHI::cmdClearAttachmentsPFN(RHICommandBuffer* commandBuffer, uint32_t attachmentCount, const RHIClearAttachment* pAttachments, uint32_t rectCount, const RHIClearRect* pRects)
    {

    }

    bool Dx12RHI::beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo)
    {
        return true;

    }
    void Dx12RHI::cmdCopyImageToBuffer(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageLayout srcImageLayout, RHIBuffer* dstBuffer, uint32_t regionCount, const RHIBufferImageCopy* pRegions)
    {

    }
    void Dx12RHI::cmdCopyImageToImage(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageAspectFlagBits srcFlag, RHIImage* dstImage, RHIImageAspectFlagBits dstFlag, uint32_t width, uint32_t height)
    {

    }
    void Dx12RHI::cmdCopyBuffer(RHICommandBuffer* commandBuffer, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32_t regionCount, RHIBufferCopy* pRegions)
    {

    }
    void Dx12RHI::cmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {

    }
    void Dx12RHI::cmdDispatch(RHICommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
    {

    }
    void Dx12RHI::cmdDispatchIndirect(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset)
    {

    }
    void Dx12RHI::cmdPipelineBarrier(RHICommandBuffer* commandBuffer, RHIPipelineStageFlags srcStageMask, RHIPipelineStageFlags dstStageMask, RHIDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const RHIMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const RHIBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const RHIImageMemoryBarrier* pImageMemoryBarriers)
    {

    }
    bool Dx12RHI::endCommandBuffer(RHICommandBuffer* commandBuffer)
    {
        return true;

    }
    void Dx12RHI::updateDescriptorSets(uint32_t descriptorWriteCount, const RHIWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const RHICopyDescriptorSet* pDescriptorCopies)
    {

    }
    bool Dx12RHI::queueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence)
    {
        return true;

    }
    bool Dx12RHI::queueWaitIdle(RHIQueue* queue)
    {
        return true;

    }
    void Dx12RHI::resetCommandPool()
    {

    }
    void Dx12RHI::waitForFences()
    {

    }
    bool Dx12RHI::waitForFences(uint32_t fenceCount, const RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout)
    {
        return true;

    }

    // query
    void Dx12RHI::getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties)
    {

    }
    RHICommandBuffer* Dx12RHI::getCurrentCommandBuffer() const
    {
        return nullptr;
    }
    RHICommandBuffer* const* Dx12RHI::getCommandBufferList() const
    {
        return nullptr;

    }
    RHICommandPool* Dx12RHI::getCommandPoor() const
    {
        return nullptr;

    }
    RHIDescriptorPool* Dx12RHI::getDescriptorPoor()const
    {
        return nullptr;

    }
    RHIFence* const* Dx12RHI::getFenceList() const
    {
        return nullptr;

    }
    QueueFamilyIndices Dx12RHI::getQueueFamilyIndices() const
    {
        return QueueFamilyIndices();
    }
    RHIQueue* Dx12RHI::getGraphicsQueue() const
    {
        return nullptr;

    }
    RHIQueue* Dx12RHI::getComputeQueue() const
    {
        return nullptr;

    }
    RHISwapChainDesc Dx12RHI::getSwapchainInfo()
    {
        return RHISwapChainDesc();
    }
    RHIDepthImageDesc Dx12RHI::getDepthImageInfo() const
    {
        return RHIDepthImageDesc();
    }
    uint8_t Dx12RHI::getMaxFramesInFlight() const
    {
        return 0;
    }
    uint8_t Dx12RHI::getCurrentFrameIndex() const
    {
        return 0;

    }
    void Dx12RHI::setCurrentFrameIndex(uint8_t index)
    {

    }

    // command write
    RHICommandBuffer* Dx12RHI::beginSingleTimeCommands()
    {
        return nullptr;
    }
    void Dx12RHI::endSingleTimeCommands(RHICommandBuffer* command_buffer)
    {

    }
    bool Dx12RHI::prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain)
    {
        return true;
    }
    void Dx12RHI::submitRendering(std::function<void()> passUpdateAfterRecreateSwapchain)
    {

    }
    void Dx12RHI::pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color)
    {

    }
    void Dx12RHI::popEvent(RHICommandBuffer* commond_buffer)
    {

    }

    // destory

    void Dx12RHI::clear()
    {

    }
    void Dx12RHI::clearSwapchain()
    {

    }
    void Dx12RHI::destroyDefaultSampler(RHIDefaultSamplerType type)
    {

    }
    void Dx12RHI::destroyMipmappedSampler()
    {

    }
    void Dx12RHI::destroyShaderModule(RHIShader* shader)
    {

    }
    void Dx12RHI::destroySemaphore(RHISemaphore* semaphore)
    {

    }
    void Dx12RHI::destroySampler(RHISampler* sampler)
    {

    }
    void Dx12RHI::destroyInstance(RHIInstance* instance)
    {

    }
    void Dx12RHI::destroyImageView(RHIImageView* imageView)
    {

    }
    void Dx12RHI::destroyImage(RHIImage* image)
    {

    }
    void Dx12RHI::destroyFramebuffer(RHIFramebuffer* framebuffer)
    {

    }
    void Dx12RHI::destroyFence(RHIFence* fence)
    {

    }
    void Dx12RHI::destroyDevice()
    {

    }
    void Dx12RHI::destroyCommandPool(RHICommandPool* commandPool)
    {

    }
    void Dx12RHI::destroyBuffer(RHIBuffer*& buffer)
    {

    }
    void Dx12RHI::freeCommandBuffers(RHICommandPool* commandPool, uint32_t commandBufferCount, RHICommandBuffer* pCommandBuffers)
    {

    }

    // memory
    void Dx12RHI::freeMemory(RHIDeviceMemory*& memory)
    {

    }
    bool Dx12RHI::mapMemory(RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size, RHIMemoryMapFlags flags, void** ppData)
    {
        return true;
    }
    void Dx12RHI::unmapMemory(RHIDeviceMemory* memory)
    {

    }
    void Dx12RHI::invalidateMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {

    }
    void Dx12RHI::flushMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size)
    {

    }

    //semaphores
    RHISemaphore*& Dx12RHI::getTextureCopySemaphore(uint32_t index)
    {
        return m_semaohore;
    }
}