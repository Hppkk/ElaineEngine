#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanPipeline.h"
#include "vulkan/ElaineVulkanShader.h"
#include "vulkan/ElaineVulkanDevice.h"
#include "vulkan/ElaineVulkanViewport.h"
#include "vulkan/ElaineVulkanRenderPass.h"
#include "vulkan/ElaineVulkanRenderPassCache.h"
#include "vulkan/ElaineVulkanCommandContext.h"

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

    VulkanDescriptorSet* VulkanPipeline::GetDescriptorSet(int32 InSlot)
    {
        return mDescriptorSetMap[InSlot];
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
        NewPipeline->SetPipelineState(InState);  // 保存 State 用于创建变体
        
        VkGraphicsPipelineCreateInfo CreateInfo;
        Elaine::Memory::MemoryZero(CreateInfo);
        CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;

        
        std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
        //std::vector<VkDescriptorSetLayout> ShaderLayouts;
        std::vector<VkVertexInputAttributeDescription> VKVertexAttributeDescs;
        VulkanShader* NewVsShader = nullptr;
        VulkanShader* NewPsShader = nullptr;
        if (!InState.mVSShaderCode.empty())
        {
            NewVsShader = mDevice->GetShaderManager()->CreateShader(InState.mVSShaderCode, Elaine::EShaderStage::VertexShader, NewPipeline, InState.mVSPath);
            NewPipeline->mVsShader = NewVsShader;
            //NewVsShader->GetDescriptorSetLayouts(ShaderLayouts);
            //VKVertexAttributeDescs = NewVsShader->GetVertexInputAttributeDescriptions();
        }

        if (!InState.mPSShaderCode.empty())
        {
            NewPsShader = mDevice->GetShaderManager()->CreateShader(InState.mPSShaderCode, Elaine::EShaderStage::FragmentShader, NewPipeline, InState.mPSPath);
            NewPipeline->mPsShader = NewPsShader;
           // NewPsShader->GetDescriptorSetLayouts(ShaderLayouts);
        }

        if (!NewVsShader || !NewPsShader)
        {
            return nullptr;
        }

        bool CompileSucceed = VulkanShaderCompileManager::instance()->CompilePipeline(NewPipeline);

        if (!CompileSucceed)
        {
            LOG_ERROR("Pipeline Compile Failed.");
            assert(false);
            return nullptr;
        }

        ShaderStages.push_back(NewVsShader->GetShaderStageCreateInfo());
        ShaderStages.push_back(NewPsShader->GetShaderStageCreateInfo());
        VKVertexAttributeDescs = NewVsShader->GetVertexInputAttributeDescriptions();
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.stageCount = ShaderStages.size();

        VkPipelineLayoutCreateInfo LayoutCreateInfo;
        Elaine::Memory::MemoryZero(LayoutCreateInfo);
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.flags = VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;
        LayoutCreateInfo.pSetLayouts = NewPipeline->mDescriptorSetLayouts.data();
        LayoutCreateInfo.setLayoutCount = NewPipeline->mDescriptorSetLayouts.size();
        //todo 
        //LayoutCreateInfo.pPushConstantRanges

        vkCreatePipelineLayout(mDevice->GetDevice(), &LayoutCreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mLayout->mPipelineLayout);
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
        Elaine::Memory::MemoryZero(VertexInputStateCreateInfo);
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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
        RasterizationStateCreateInfo.depthClampEnable = VK_TRUE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;//TransRHICullModeToVk(InState.mCullMode);
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

        // 从 GRAPHICS_PIPELINE_STATE_DESC 的渲染目标格式构建 RenderPassKey
        RenderPassKey PipelineRenderPassKey;
        for (uint32 i = 0; i < MAX_RENDER_TARGET_COUNT; ++i)
        {
            if (InState.mRenderTargetFormats[i] == PF_Unknown) break;
            PipelineRenderPassKey.ColorFormats[PipelineRenderPassKey.NumColorTargets] = EngineToVkTextureFormat(InState.mRenderTargetFormats[i], false);
            PipelineRenderPassKey.ColorLoadOps[PipelineRenderPassKey.NumColorTargets] = VK_ATTACHMENT_LOAD_OP_CLEAR;
            PipelineRenderPassKey.ColorStoreOps[PipelineRenderPassKey.NumColorTargets] = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.ColorStoreOps[PipelineRenderPassKey.NumColorTargets] = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.ColorFinalLayouts[PipelineRenderPassKey.NumColorTargets] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            PipelineRenderPassKey.ColorSamples[PipelineRenderPassKey.NumColorTargets] = VK_SAMPLE_COUNT_1_BIT;
            PipelineRenderPassKey.NumColorTargets++;
        }

        // 如果没有指定渲染目标格式，使用默认的交换链格式
        if (PipelineRenderPassKey.NumColorTargets == 0)
        {
            PipelineRenderPassKey.NumColorTargets = 1;
            PipelineRenderPassKey.ColorFormats[0] = VK_FORMAT_B8G8R8A8_SRGB;  // 默认交换链格式
            PipelineRenderPassKey.ColorLoadOps[0] = VK_ATTACHMENT_LOAD_OP_CLEAR;
            PipelineRenderPassKey.ColorStoreOps[0] = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.ColorStoreOps[0] = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.ColorFinalLayouts[0] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            PipelineRenderPassKey.ColorSamples[0] = VK_SAMPLE_COUNT_1_BIT;
        }

        // 深度格式
        if (InState.mDepthStencilTargetFormat != PF_Unknown)
        {
            PipelineRenderPassKey.DepthFormat = EngineToVkTextureFormat(InState.mDepthStencilTargetFormat, false);
            PipelineRenderPassKey.DepthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            PipelineRenderPassKey.DepthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            PipelineRenderPassKey.StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            PipelineRenderPassKey.DepthSamples = VK_SAMPLE_COUNT_1_BIT;
        }
        else if (InState.mbDepthTestEnable || InState.mbDepthWriteEnable)
        {
            // 如果启用深度测试但没指定格式，使用默认深度格式
            PipelineRenderPassKey.DepthFormat = VK_FORMAT_D32_SFLOAT;
            PipelineRenderPassKey.DepthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            PipelineRenderPassKey.DepthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            PipelineRenderPassKey.DepthSamples = VK_SAMPLE_COUNT_1_BIT;
        }

        VulkanCommandContext* VkCmdContext = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());

        // 从缓存获取或创建 RenderPass
        VkRenderPass PipelineRenderPass = VkCmdContext->GetRenderPassCache()->GetOrCreateRenderPass(PipelineRenderPassKey);
        CreateInfo.renderPass = PipelineRenderPass;
        CreateInfo.subpass = 0;
        CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        CreateInfo.basePipelineIndex = -1;
        vkCreateGraphicsPipelines(mDevice->GetDevice(), mPipelineCache.GetHandle(), 1, &CreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mPipeline);
        mGfxPipelinePool.emplace(InState.mStateHash, NewPipeline);
        return NewPipeline;
    }

    VulkanGfxPipeline* VulkanPiplineManager::GetOrCreateGfxPipelineForRenderPass(
        const GRAPHICS_PIPELINE_STATE_DESC& InState,
        const RenderPassKey& RPKey)
    {
        // 构建变体 Key
        PipelineVariantKey VariantKey;
        VariantKey.StateHash = InState.mStateHash;
        VariantKey.RenderPass = RPKey;

        // 查找变体缓存
        auto It = mPipelineVariantCache.find(VariantKey);
        if (It != mPipelineVariantCache.end())
        {
            return It->second;
        }

        // 查找同一 StateHash 的已编译 Pipeline，以复用其 Shaders
        VulkanGfxPipeline* ExistingPipeline = nullptr;
        auto PoolIt = mGfxPipelinePool.find(InState.mStateHash);
        if (PoolIt != mGfxPipelinePool.end())
        {
            ExistingPipeline = PoolIt->second;
        }
        else
        {
            // 也在变体缓存中查找
            for (auto& Pair : mPipelineVariantCache)
            {
                if (Pair.first.StateHash == InState.mStateHash)
                {
                    ExistingPipeline = Pair.second;
                    break;
                }
            }
        }

        // 创建新的 Pipeline 变体
        VulkanGfxPipeline* NewPipeline = CreateGraphicPipelineWithRenderPass(InState, RPKey, ExistingPipeline);
        if (NewPipeline)
        {
            mPipelineVariantCache.emplace(VariantKey, NewPipeline);
        }
        return NewPipeline;
    }

    VulkanGfxPipeline* VulkanPiplineManager::CreateGraphicPipelineWithRenderPass(
        const GRAPHICS_PIPELINE_STATE_DESC& InState,
        const RenderPassKey& RPKey,
        VulkanGfxPipeline* ExistingPipeline)
    {
        VulkanGfxPipeline* NewPipeline = new VulkanGfxPipeline(mDevice);
        NewPipeline->SetPipelineState(InState);  // 保存 State 用于创建变体
        
        VkGraphicsPipelineCreateInfo CreateInfo;
        Elaine::Memory::MemoryZero(CreateInfo);
        CreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

        std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;
        std::vector<VkVertexInputAttributeDescription> VKVertexAttributeDescs;
        VulkanShader* VsShader = nullptr;
        VulkanShader* PsShader = nullptr;

        // 如果有现有 Pipeline，复用其 Shader 和 DescriptorSetLayouts
        if (ExistingPipeline && ExistingPipeline->mVsShader && ExistingPipeline->mPsShader)
        {
            // 直接复用已编译的 Shader
            VsShader = ExistingPipeline->mVsShader;
            PsShader = ExistingPipeline->mPsShader;
            NewPipeline->mVsShader = VsShader;
            NewPipeline->mPsShader = PsShader;
            
            // 复用 DescriptorSetLayouts
            NewPipeline->mDescriptorSetLayouts = ExistingPipeline->GetDescriptorSetLayouts();
            // @TODO
            NewPipeline->mDescriptorSets = ExistingPipeline->GetDescriptorSets();
        }
        else
        {
            // 没有现有 Pipeline，需要创建和编译 Shader
            if (!InState.mVSShaderCode.empty())
            {
                VsShader = mDevice->GetShaderManager()->CreateShader(InState.mVSShaderCode, Elaine::EShaderStage::VertexShader, NewPipeline, InState.mVSPath);
                NewPipeline->mVsShader = VsShader;
            }

            if (!InState.mPSShaderCode.empty())
            {
                PsShader = mDevice->GetShaderManager()->CreateShader(InState.mPSShaderCode, Elaine::EShaderStage::FragmentShader, NewPipeline, InState.mPSPath);
                NewPipeline->mPsShader = PsShader;
            }

            if (!VsShader || !PsShader)
            {
                delete NewPipeline;
                return nullptr;
            }

            bool CompileSucceed = VulkanShaderCompileManager::instance()->CompilePipeline(NewPipeline);
            if (!CompileSucceed)
            {
                LOG_ERROR("Pipeline Compile Failed.");
                delete NewPipeline;
                return nullptr;
            }
        }

        if (!VsShader || !PsShader)
        {
            delete NewPipeline;
            return nullptr;
        }

        ShaderStages.push_back(VsShader->GetShaderStageCreateInfo());
        ShaderStages.push_back(PsShader->GetShaderStageCreateInfo());
        VKVertexAttributeDescs = VsShader->GetVertexInputAttributeDescriptions();
        CreateInfo.pStages = ShaderStages.data();
        CreateInfo.stageCount = ShaderStages.size();

        VkPipelineLayoutCreateInfo LayoutCreateInfo;
        Elaine::Memory::MemoryZero(LayoutCreateInfo);
        LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        LayoutCreateInfo.flags = VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;
        LayoutCreateInfo.pSetLayouts = NewPipeline->mDescriptorSetLayouts.data();
        LayoutCreateInfo.setLayoutCount = NewPipeline->mDescriptorSetLayouts.size();

        vkCreatePipelineLayout(mDevice->GetDevice(), &LayoutCreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mLayout->mPipelineLayout);
        CreateInfo.layout = NewPipeline->mLayout->mPipelineLayout;

        // 顶点输入
        VkPipelineVertexInputStateCreateInfo VertexInputStateCreateInfo;
        Elaine::Memory::MemoryZero(VertexInputStateCreateInfo);
        VertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VertexInputStateCreateInfo.pVertexAttributeDescriptions = VKVertexAttributeDescs.data();
        VertexInputStateCreateInfo.vertexAttributeDescriptionCount = VKVertexAttributeDescs.size();

        //std::vector<VkVertexInputBindingDescription> VKVertexInputBindingDescs;
        //VkVertexInputBindingDescription TempInputBindingDesc;
        //TempInputBindingDesc.binding = 0;
        //TempInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        //TempInputBindingDesc.stride = InState.mVertexAttribute.mStride;
        //VKVertexInputBindingDescs.push_back(TempInputBindingDesc);

        VertexInputStateCreateInfo.pVertexBindingDescriptions = NewPipeline->mVsShader->GetVertexInputBindingDescription().data(); //VKVertexInputBindingDescs.data();
        VertexInputStateCreateInfo.vertexBindingDescriptionCount = NewPipeline->mVsShader->GetVertexInputBindingDescription().size(); //VKVertexInputBindingDescs.size();
        CreateInfo.pVertexInputState = &VertexInputStateCreateInfo;

        // 图元装配
        VkPipelineInputAssemblyStateCreateInfo InputAssemblyStateCreateInfo;
        Elaine::Memory::MemoryZero(InputAssemblyStateCreateInfo);
        InputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        InputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        InputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
        CreateInfo.pInputAssemblyState = &InputAssemblyStateCreateInfo;

        // 视口（动态状态）
        VkPipelineViewportStateCreateInfo ViewportStateCreateInfo;
        Elaine::Memory::MemoryZero(ViewportStateCreateInfo);
        ViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        ViewportStateCreateInfo.viewportCount = 1;
        ViewportStateCreateInfo.scissorCount = 1;
        CreateInfo.pViewportState = &ViewportStateCreateInfo;

        // 光栅化
        VkPipelineRasterizationStateCreateInfo RasterizationStateCreateInfo;
        Elaine::Memory::MemoryZero(RasterizationStateCreateInfo);
        RasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        RasterizationStateCreateInfo.depthClampEnable = VK_TRUE;
        RasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        RasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
        RasterizationStateCreateInfo.polygonMode = TransRHIPolygonModeToVk(InState.mPolygonMode);
        RasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        RasterizationStateCreateInfo.depthBiasEnable = VK_TRUE;
        RasterizationStateCreateInfo.lineWidth = 1.0f;
        CreateInfo.pRasterizationState = &RasterizationStateCreateInfo;

        // 多重采样
        VkPipelineMultisampleStateCreateInfo MultisampleStateCreateInfo;
        Elaine::Memory::MemoryZero(MultisampleStateCreateInfo);
        MultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        MultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        MultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        CreateInfo.pMultisampleState = &MultisampleStateCreateInfo;

        // 深度模板
        VkPipelineDepthStencilStateCreateInfo DepthStencilStateCreateInfo;
        Elaine::Memory::MemoryZero(DepthStencilStateCreateInfo);
        DepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        DepthStencilStateCreateInfo.depthTestEnable = InState.mbDepthTestEnable;
        DepthStencilStateCreateInfo.depthWriteEnable = InState.mbDepthWriteEnable;
        DepthStencilStateCreateInfo.depthCompareOp = TransRHICompareOpToVk(InState.mDepthOp);
        DepthStencilStateCreateInfo.stencilTestEnable = InState.mStencilTestEnable;
        CreateInfo.pDepthStencilState = &DepthStencilStateCreateInfo;

        // 颜色混合
        VkPipelineColorBlendAttachmentState BlendAttachmentState;
        Elaine::Memory::MemoryZero(BlendAttachmentState);
        BlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        BlendAttachmentState.blendEnable = VK_FALSE;

        std::vector<VkPipelineColorBlendAttachmentState> BlendAttachments(RPKey.GetNumColorAttachments(), BlendAttachmentState);

        VkPipelineColorBlendStateCreateInfo ColorBlendStateCreateInfo;
        Elaine::Memory::MemoryZero(ColorBlendStateCreateInfo);
        ColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        ColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        ColorBlendStateCreateInfo.attachmentCount = RPKey.GetNumColorAttachments();
        ColorBlendStateCreateInfo.pAttachments = BlendAttachments.data();
        CreateInfo.pColorBlendState = &ColorBlendStateCreateInfo;

        // 动态状态
        std::vector<VkDynamicState> DynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo DynamicStateCreateInfo;
        Elaine::Memory::MemoryZero(DynamicStateCreateInfo);
        DynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        DynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(DynamicStates.size());
        DynamicStateCreateInfo.pDynamicStates = DynamicStates.data();
        CreateInfo.pDynamicState = &DynamicStateCreateInfo;

        // 使用提供的 RenderPassKey 获取 RenderPass
        VulkanCommandContext* VkCmdContext = static_cast<VulkanCommandContext*>(GetVulkanDynamicRHI()->GetDefaultCommandContext());
        VkRenderPass PipelineRenderPass = VkCmdContext->GetRenderPassCache()->GetOrCreateRenderPass(RPKey);
        CreateInfo.renderPass = PipelineRenderPass;
        CreateInfo.subpass = 0;
        CreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        CreateInfo.basePipelineIndex = -1;

        VkResult Result = vkCreateGraphicsPipelines(mDevice->GetDevice(), mPipelineCache.GetHandle(), 1, &CreateInfo, VULKAN_CPU_ALLOCATOR, &NewPipeline->mPipeline);
        if (Result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create graphics pipeline with RenderPass variant!");
            delete NewPipeline;
            return nullptr;
        }

        return NewPipeline;
    }
}