#pragma once
#include "render/common/ElaineRHI.h"



namespace Elaine
{
	class ElaineCoreExport Dx12RHI final :public RHI, public Singleton<Dx12RHI>
	{
	public:
        ~Dx12RHI();

        // initialize
        virtual void initialize(RHIInitInfo* init_info) override;
        virtual void prepareContext() override;
        virtual RHITYPE getRHIType() override;

        virtual bool isPointLightShadowEnabled() override;
        // allocate and create
        bool allocateCommandBuffers(const RHICommandBufferAllocateInfo* pAllocateInfo, RHICommandBuffer*& pCommandBuffers) override;
        bool allocateDescriptorSets(const RHIDescriptorSetAllocateInfo* pAllocateInfo, RHIDescriptorSet*& pDescriptorSets) override;
        void createSwapchain() override;
        void recreateSwapchain() override;
        void createSwapchainImageViews() override;
        void createFramebufferImageAndView() override;
        RHISampler* getOrCreateDefaultSampler(RHIDefaultSamplerType type) override;
        RHISampler* getOrCreateMipmapSampler(uint32_t width, uint32_t height) override;
        RHIShader* createShaderModule(const std::vector<unsigned char>& shader_code) override;
        void createBuffer(RHIDeviceSize size, RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory) override;
        void createBufferAndInitialize(RHIBufferUsageFlags usage, RHIMemoryPropertyFlags properties, RHIBuffer*& buffer, RHIDeviceMemory*& buffer_memory, RHIDeviceSize size, void* data = nullptr, int datasize = 0) override;
        
        void copyBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, RHIDeviceSize srcOffset, RHIDeviceSize dstOffset, RHIDeviceSize size) override;
        void createImage(uint32_t image_width, uint32_t image_height, RHIFormat format, RHIImageTiling image_tiling, RHIImageUsageFlags image_usage_flags, RHIMemoryPropertyFlags memory_property_flags,
            RHIImage*& image, RHIDeviceMemory*& memory, RHIImageCreateFlags image_create_flags, uint32_t array_layers, uint32_t miplevels) override;
        void createImageView(RHIImage* image, RHIFormat format, RHIImageAspectFlags image_aspect_flags, RHIImageViewType view_type, uint32_t layout_count, uint32_t miplevels,
            RHIImageView*& image_view) override;
        virtual void createCommandPool() override;
        bool createCommandPool(const RHICommandPoolCreateInfo* pCreateInfo, RHICommandPool*& pCommandPool) override;
        bool createDescriptorPool(const RHIDescriptorPoolCreateInfo* pCreateInfo, RHIDescriptorPool*& pDescriptorPool) override;
        bool createDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo* pCreateInfo, RHIDescriptorSetLayout*& pSetLayout) override;
        bool createFence(const RHIFenceCreateInfo* pCreateInfo, RHIFence*& pFence) override;
        bool createFramebuffer(const RHIFramebufferCreateInfo* pCreateInfo, RHIFramebuffer*& pFramebuffer) override;
        bool createGraphicsPipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIGraphicsPipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines) override;
        bool createComputePipelines(RHIPipelineCache* pipelineCache, uint32_t createInfoCount, const RHIComputePipelineCreateInfo* pCreateInfos, RHIPipeline*& pPipelines) override;
        bool createPipelineLayout(const RHIPipelineLayoutCreateInfo* pCreateInfo, RHIPipelineLayout*& pPipelineLayout) override;
        bool createRenderPass(const RHIRenderPassCreateInfo* pCreateInfo, RHIRenderPass*& pRenderPass) override;
        bool createSampler(const RHISamplerCreateInfo* pCreateInfo, RHISampler*& pSampler) override;
        bool createSemaphore(const RHISemaphoreCreateInfo* pCreateInfo, RHISemaphore*& pSemaphore) override;

        // command and command write
        bool waitForFencesPFN(uint32_t fenceCount, RHIFence* const* pFence, RHIBool32 waitAll, uint64_t timeout) override;
        bool resetFencesPFN(uint32_t fenceCount, RHIFence* const* pFences) override;
        bool resetCommandPoolPFN(RHICommandPool* commandPool, RHICommandPoolResetFlags flags) override;
        bool beginCommandBufferPFN(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;
        bool endCommandBufferPFN(RHICommandBuffer* commandBuffer) override;
        void cmdBeginRenderPassPFN(RHICommandBuffer* commandBuffer, const RHIRenderPassBeginInfo* pRenderPassBegin, RHISubpassContents contents) override;
        void cmdNextSubpassPFN(RHICommandBuffer* commandBuffer, RHISubpassContents contents) override;
        void cmdEndRenderPassPFN(RHICommandBuffer* commandBuffer) override;
        void cmdBindPipelinePFN(RHICommandBuffer* commandBuffer, RHIPipelineBindPoint pipelineBindPoint, RHIPipeline* pipeline) override;
        void cmdSetViewportPFN(RHICommandBuffer* commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const RHIViewport* pViewports) override;
        void cmdSetScissorPFN(RHICommandBuffer* commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const RHIRect2D* pScissors) override;
        void cmdBindVertexBuffersPFN(
            RHICommandBuffer* commandBuffer,
            uint32_t firstBinding,
            uint32_t bindingCount,
            RHIBuffer* const* pBuffers,
            const RHIDeviceSize* pOffsets) override;
        void cmdBindIndexBufferPFN(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset, RHIIndexType indexType) override;
        void cmdBindDescriptorSetsPFN(
            RHICommandBuffer* commandBuffer,
            RHIPipelineBindPoint pipelineBindPoint,
            RHIPipelineLayout* layout,
            uint32_t firstSet,
            uint32_t descriptorSetCount,
            const RHIDescriptorSet* const* pDescriptorSets,
            uint32_t dynamicOffsetCount,
            const uint32_t* pDynamicOffsets) override;
        void cmdDrawIndexedPFN(RHICommandBuffer* commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override;
        void cmdClearAttachmentsPFN(RHICommandBuffer* commandBuffer, uint32_t attachmentCount, const RHIClearAttachment* pAttachments, uint32_t rectCount, const RHIClearRect* pRects) override;

        bool beginCommandBuffer(RHICommandBuffer* commandBuffer, const RHICommandBufferBeginInfo* pBeginInfo) override;
        void cmdCopyImageToBuffer(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageLayout srcImageLayout, RHIBuffer* dstBuffer, uint32_t regionCount, const RHIBufferImageCopy* pRegions) override;
        void cmdCopyImageToImage(RHICommandBuffer* commandBuffer, RHIImage* srcImage, RHIImageAspectFlagBits srcFlag, RHIImage* dstImage, RHIImageAspectFlagBits dstFlag, uint32_t width, uint32_t height) override;
        void cmdCopyBuffer(RHICommandBuffer* commandBuffer, RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, uint32_t regionCount, RHIBufferCopy* pRegions) override;
        void cmdDraw(RHICommandBuffer* commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
        void cmdDispatch(RHICommandBuffer* commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
        void cmdDispatchIndirect(RHICommandBuffer* commandBuffer, RHIBuffer* buffer, RHIDeviceSize offset) override;
        void cmdPipelineBarrier(RHICommandBuffer* commandBuffer, RHIPipelineStageFlags srcStageMask, RHIPipelineStageFlags dstStageMask, RHIDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const RHIMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const RHIBufferMemoryBarrier* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const RHIImageMemoryBarrier* pImageMemoryBarriers) override;
        bool endCommandBuffer(RHICommandBuffer* commandBuffer) override;
        void updateDescriptorSets(uint32_t descriptorWriteCount, const RHIWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const RHICopyDescriptorSet* pDescriptorCopies) override;
        bool queueSubmit(RHIQueue* queue, uint32_t submitCount, const RHISubmitInfo* pSubmits, RHIFence* fence) override;
        bool queueWaitIdle(RHIQueue* queue) override;
        void resetCommandPool() override;
        void waitForFences() override;
        bool waitForFences(uint32_t fenceCount, const RHIFence* const* pFences, RHIBool32 waitAll, uint64_t timeout);

        // query
        void getPhysicalDeviceProperties(RHIPhysicalDeviceProperties* pProperties) override;
        RHICommandBuffer* getCurrentCommandBuffer() const override;
        RHICommandBuffer* const* getCommandBufferList() const override;
        RHICommandPool* getCommandPoor() const override;
        RHIDescriptorPool* getDescriptorPoor()const override;
        RHIFence* const* getFenceList() const override;
        QueueFamilyIndices getQueueFamilyIndices() const override;
        RHIQueue* getGraphicsQueue() const override;
        RHIQueue* getComputeQueue() const override;
        RHISwapChainDesc getSwapchainInfo() override;
        RHIDepthImageDesc getDepthImageInfo() const override;
        uint8_t getMaxFramesInFlight() const override;
        uint8_t getCurrentFrameIndex() const override;
        void setCurrentFrameIndex(uint8_t index) override;

        // command write
        RHICommandBuffer* beginSingleTimeCommands() override;
        void            endSingleTimeCommands(RHICommandBuffer* command_buffer) override;
        bool prepareBeforePass(std::function<void()> passUpdateAfterRecreateSwapchain) override;
        void submitRendering(std::function<void()> passUpdateAfterRecreateSwapchain) override;
        void pushEvent(RHICommandBuffer* commond_buffer, const char* name, const float* color) override;
        void popEvent(RHICommandBuffer* commond_buffer) override;

        // destory
 
        void clear() override;
        void clearSwapchain() override;
        void destroyDefaultSampler(RHIDefaultSamplerType type) override;
        void destroyMipmappedSampler() override;
        void destroyShaderModule(RHIShader* shader) override;
        void destroySemaphore(RHISemaphore* semaphore) override;
        void destroySampler(RHISampler* sampler) override;
        void destroyInstance(RHIInstance* instance) override;
        void destroyImageView(RHIImageView* imageView) override;
        void destroyImage(RHIImage* image) override;
        void destroyFramebuffer(RHIFramebuffer* framebuffer) override;
        void destroyFence(RHIFence* fence) override;
        void destroyDevice() override;
        void destroyCommandPool(RHICommandPool* commandPool) override;
        void destroyBuffer(RHIBuffer*& buffer) override;
        void freeCommandBuffers(RHICommandPool* commandPool, uint32_t commandBufferCount, RHICommandBuffer* pCommandBuffers) override;

        // memory
        void freeMemory(RHIDeviceMemory*& memory) override;
        bool mapMemory(RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size, RHIMemoryMapFlags flags, void** ppData) override;
        void unmapMemory(RHIDeviceMemory* memory) override;
        void invalidateMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size) override;
        void flushMappedMemoryRanges(void* pNext, RHIDeviceMemory* memory, RHIDeviceSize offset, RHIDeviceSize size) override;

        //semaphores
        RHISemaphore*& getTextureCopySemaphore(uint32_t index) override;
    public:
        RHISemaphore*                       m_semaohore;
	};
}