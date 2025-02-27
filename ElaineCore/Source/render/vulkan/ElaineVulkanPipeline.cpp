#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanPipeline.h"
#include "vulkan/ElaineVulkanShader.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanViewport.h"
#include "vulkan/ElaineVulkanRenderPass.h"

namespace VulkanRHI
{
    VulkanLayout::VulkanLayout(VulkanDevice* InDevice)
        :mDevice(InDevice)
    {

    }

    VulkanLayout::~VulkanLayout()
    {

    }

    VulkanPipeline::VulkanPipeline(VulkanDevice* InDevice)
    {
        mLayout = new VulkanLayout(InDevice);
    }

    VulkanPipeline::~VulkanPipeline()
    {
        SAFE_DELETE(mLayout);
    }

    VulkanComputePipeline::VulkanComputePipeline(VulkanDevice* InDevice)
        :VulkanPipeline(InDevice)
    {
        mType = Compute;
    }

    VulkanComputePipeline::~VulkanComputePipeline()
    {

    }

    VulkanGfxPipeline::VulkanGfxPipeline(VulkanDevice* InDevice)
        :VulkanPipeline(InDevice)
    {
        mType = Graphic;

    }

    VulkanGfxPipeline::~VulkanGfxPipeline()
    {

    }

    VulkanPiplineManager::VulkanPiplineManager(VulkanDevice* InDevice)
        :mDevice(InDevice)
    {

    }

    VulkanPiplineManager::~VulkanPiplineManager()
    {

    }

    VulkanComputePipeline* VulkanPiplineManager::GetOrCreateComputePipeline(VulkanShader* InShader)
    {
        //find hash is exist current shader pipeline
        auto Iter = mComputePipelinePool.find(InShader->GetHash());
        if (Iter != mComputePipelinePool.end())
            return Iter->second;


        return CreateComputePipeline(InShader);
    }
    VulkanComputePipeline* VulkanPiplineManager::CreateComputePipeline(VulkanShader* InShader)
    {
        VulkanComputePipeline* NewPipeline = new VulkanComputePipeline(mDevice);
        NewPipeline->mComputeShader = InShader;

        //todo cache pipline layout

        VkPipelineLayoutCreateInfo LayoutCreateInfo;
        Elaine::Memory::MemoryZero(LayoutCreateInfo);
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        
        vkCreatePipelineLayout(mDevice->GetDevice(), &LayoutCreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mLayout->mPipelineLayout);
        VkComputePipelineCreateInfo CreateInfo;
        Elaine::Memory::MemoryZero(CreateInfo);
        CreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        CreateInfo.stage = InShader->GetShaderStageCreateInfo();
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;
        vkCreateComputePipelines(mDevice->GetDevice(), mPipelineCache.GetHandle(), 1, &CreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mPipeline);
        mComputePipelinePool.emplace(InShader->GetHash(), NewPipeline);
        return NewPipeline;
    }

    VulkanGfxPipeline* VulkanPiplineManager::GetOrCreateGfxPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState)
    {
        
        //todo cache

        return CreateGraphicPipeline(InState);
    }

    VulkanGfxPipeline* VulkanPiplineManager::CreateGraphicPipeline(const GRAPHICS_PIPELINE_STATE_DESC& InState)
    {
        VulkanGfxPipeline* NewPipeline = new VulkanGfxPipeline(mDevice);
        
        VkGraphicsPipelineCreateInfo CreateInfo;
        Elaine::Memory::MemoryZero(CreateInfo);
        CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;

        
        std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
        std::vector<VkDescriptorSetLayout> ShaderLayouts;
        if (!InState.mVSShaderCode.empty())
        {
            VulkanShader* NewShader = mDevice->GetShaderManager()->CreateShader(InState.mVSShaderCode, Elaine::EShaderStage::VertexShader);
            ShaderStages.push_back(NewShader->GetShaderStageCreateInfo());
            NewPipeline->mVsShader = NewShader;
            NewShader->GetDescriptorSetLayouts(ShaderLayouts);
        }
        if (!InState.mPSShaderCode.empty())
        {
            VulkanShader* NewShader = mDevice->GetShaderManager()->CreateShader(InState.mPSShaderCode, Elaine::EShaderStage::FragmentShader);
            ShaderStages.push_back(NewShader->GetShaderStageCreateInfo());
            NewPipeline->mPsShader = NewShader;
            NewShader->GetDescriptorSetLayouts(ShaderLayouts);
        }
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.stageCount = ShaderStages.size();

        VkPipelineLayoutCreateInfo LayoutCreateInfo;
        Elaine::Memory::MemoryZero(LayoutCreateInfo);
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.pSetLayouts = ShaderLayouts.data();
        LayoutCreateInfo.setLayoutCount = ShaderLayouts.size();
        //todo 
        //LayoutCreateInfo.pPushConstantRanges

        vkCreatePipelineLayout(mDevice->GetDevice(), &LayoutCreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mLayout->mPipelineLayout);
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
        Elaine::Memory::MemoryZero(VertexInputStateCreateInfo);
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        //todo
        std::vector<VkVertexInputAttributeDescription> VKVertexAttributeDescs;
        
        for (int Index = 0; Index < InState.mVertexAttribute.mAttributeSize;++Index)
        {
            VkVertexInputAttributeDescription TempAttributeDesc;
            TempAttributeDesc.binding = 0;
            TempAttributeDesc.format = EngineToVkVertexFormat(InState.mVertexAttribute.mFormat[Index]);
            TempAttributeDesc.location = Index;
            TempAttributeDesc.offset = InState.mVertexAttribute.mOffset[Index];
            VKVertexAttributeDescs.push_back(TempAttributeDesc);
        }
        
        VertexInputStateCreateInfo.pVertexAttributeDescriptions = VKVertexAttributeDescs.data();
        VertexInputStateCreateInfo.vertexAttributeDescriptionCount = VKVertexAttributeDescs.size();
        std::vector<VkVertexInputBindingDescription> VKVertexInputBindingDescs;
        VkVertexInputBindingDescription TempInputBindingDesc;
        TempInputBindingDesc.binding = 0;
        TempInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        TempInputBindingDesc.stride = InState.mVertexAttribute.mStride;
        VKVertexInputBindingDescs.push_back(TempInputBindingDesc);
        VertexInputStateCreateInfo.pVertexBindingDescriptions = VKVertexInputBindingDescs.data();
        VertexInputStateCreateInfo.vertexBindingDescriptionCount = VKVertexInputBindingDescs.size();
        CreateInfo.pVertexInputState = &VertexInputStateCreateInfo;

        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo;
        Elaine::Memory::MemoryZero(InputAssemblyStateCreateInfo);
        InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyStateCreateInfo.topology = TransRHIPrimitiveToVk(InState.mPrimitiveType);
        InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
        CreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;

        if (InState.mbTessellation)
        {
            VkPipelineTessellationStateCreateInfo TessellationStateCreateInfo;
            Elaine::Memory::MemoryZero(TessellationStateCreateInfo);
            TessellationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            TessellationStateCreateInfo.patchControlPoints = 0;
            CreateInfo.pTessellationState = &TessellationStateCreateInfo;
        }

        VkPipelineViewportStateCreateInfo ViewportStateCreateInfo;
        Elaine::Memory::MemoryZero(ViewportStateCreateInfo);
        ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        
        if (InState.mbUseDefultViewPort)
        {
            ViewportStateCreateInfo.viewportCount = 1;
            ViewportStateCreateInfo.pViewports = &GetVulkanDynamicRHI()->GetViewport()->GetDefaultViewPort();
            ViewportStateCreateInfo.scissorCount = 1;
            ViewportStateCreateInfo.pScissors = &GetVulkanDynamicRHI()->GetViewport()->GetDefaultScissor();
        }
        else
        {
            std::vector<VkViewport> VKViewPorts;
            for (auto& RHIVP : InState.mViewBounds)
            {
                VkViewport VKvps;
                VKvps.x = RHIVP.TopLeftX;
                VKvps.y = RHIVP.TopLeftY;
                VKvps.maxDepth = RHIVP.MaxDepth;
                VKvps.minDepth = RHIVP.MinDepth;
                VKvps.width = RHIVP.Width;
                VKvps.height = RHIVP.Height;
                VKViewPorts.push_back(VKvps);
            }
            ViewportStateCreateInfo.viewportCount = InState.mViewBounds.size();
            ViewportStateCreateInfo.pViewports = VKViewPorts.data();
            std::vector<VkRect2D> VKRect2Ds;
            for (auto& RHIRc : InState.mScissors)
            {

            }
            ViewportStateCreateInfo.scissorCount = InState.mScissors.size();
            ViewportStateCreateInfo.pScissors = VKRect2Ds.data();
        }

        CreateInfo.pViewportState = &ViewportStateCreateInfo;

        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;
        Elaine::Memory::MemoryZero(RasterizationStateCreateInfo);
        RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.cullMode = TransRHICullModeToVk(InState.mCullMode);
        RasterizationStateCreateInfo.polygonMode = TransRHIPolygonModeToVk(InState.mPolygonMode);
        RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
        RasterizationStateCreateInfo.depthBiasConstantFactor = 0.01f;
        RasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        RasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
        RasterizationStateCreateInfo.lineWidth = 1.0f;
        CreateInfo.pRasterizationState = &RasterizationStateCreateInfo;

        VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo;
        Elaine::Memory::MemoryZero(MultisampleStateCreateInfo);
        MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultisampleStateCreateInfo.rasterizationSamples = TransRHIMultiSampleToVk(InState.mMultiSampleCount);
        MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        MultisampleStateCreateInfo.minSampleShading = 1;
        MultisampleStateCreateInfo.pSampleMask = nullptr;
        MultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        MultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;
        CreateInfo.pMultisampleState = &MultisampleStateCreateInfo;

        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
        Elaine::Memory::MemoryZero(DepthStencilStateCreateInfo);
        DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilStateCreateInfo.depthTestEnable = InState.mbDepthTestEnable;
        DepthStencilStateCreateInfo.depthWriteEnable = InState.mbDepthWriteEnable;
        DepthStencilStateCreateInfo.depthCompareOp = TransRHICompareOpToVk(InState.mDepthOp);
        DepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        //todo stencil test
        DepthStencilStateCreateInfo.stencilTestEnable = InState.mStencilTestEnable;
        //DepthStencilStateCreateInfo.front.;
        //DepthStencilStateCreateInfo.back.
        DepthStencilStateCreateInfo.minDepthBounds = 0.0f;
        DepthStencilStateCreateInfo.maxDepthBounds = 0.0f;
        CreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;

        //todo color blend
        VkPipelineColorBlendAttachmentState ColorBlendAttachmentState;
        Elaine::Memory::MemoryZero(ColorBlendAttachmentState);
        ColorBlendAttachmentState.blendEnable = InState.mbEnableColorBlend;
        ColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        ColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        ColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        ColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        ColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
        ColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
            | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo;
        Elaine::Memory::MemoryZero(ColorBlendStateCreateInfo);
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        ColorBlendStateCreateInfo.attachmentCount = 1;
        ColorBlendStateCreateInfo.pAttachments = &ColorBlendAttachmentState;
        CreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;

        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo;
        Elaine::Memory::MemoryZero(DynamicStateCreateInfo);
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        std::vector<VkDynamicState> mVkDynamicStates{ VK_DYNAMIC_STATE_VIEWPORT ,VK_DYNAMIC_STATE_SCISSOR };
        DynamicStateCreateInfo.dynamicStateCount = mVkDynamicStates.size();
        DynamicStateCreateInfo.pDynamicStates = mVkDynamicStates.data();

        CreateInfo.pDynamicState = &DynamicStateCreateInfo;
        CreateInfo.renderPass = GetVulkanDynamicRHI()->GetDefaultRenderPass()->GetHandle();//NewPipeline->GetRenderPass()->GetHandle();
        CreateInfo.subpass = 0;
        CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        CreateInfo.basePipelineIndex = -1;
        vkCreateGraphicsPipelines(mDevice->GetDevice(), mPipelineCache.GetHandle(), 1, &CreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mPipeline);
        mGfxPipelinePool.emplace(InState.mStateHash, NewPipeline);
        return NewPipeline;
    }
}