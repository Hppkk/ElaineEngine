#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "render/ElaineWindowSystem.h"
#include "render/vulkan/ElaineVulkanUtil.h"
#include "render/vulkan/ElaineVulkanInstance.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanCommandContext.h"
#include "render/vulkan/ElaineVulkanViewport.h"
#include "render/vulkan/ElaineVulkanRenderPass.h"
#include "render/vulkan/ElaineVulkanSwapChain.h"
#include "render/vulkan/ElaineVulkanShader.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#ifdef WIN32
#include "vulkan_win32.h"
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID 
#include "vulkan_android.h"
#endif

namespace VulkanRHI
{
	VulkanDynamicRHI* GVulkanDynamicRHI = nullptr;
	PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;

	VulkanDynamicRHI* GetVulkanDynamicRHI()
	{
		return GVulkanDynamicRHI;
	}

	VulkanDynamicRHI::VulkanDynamicRHI()
	{
		GVulkanDynamicRHI = this;
	}

	VulkanDynamicRHI::~VulkanDynamicRHI()
	{
		GVulkanDynamicRHI = nullptr;
	}

	void VulkanDynamicRHI::Initilize(const Elaine::RHI_PARAM_DESC& InDesc)
	{
		mWindowHandle = InDesc.WindowHandle;

		mInstance = new VulkanInstance();
		mInstance->Initilize();

		mPhyDevice = new VulkanPhysicalDevice(mInstance);
		mPhyDevice->Initilize();

		mDevice = new VulkanDevice(mPhyDevice);
		mDevice->Initilize();

		{
			VulkanRHI::vkGetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)vkGetInstanceProcAddr(mInstance->GetInstance(), "vkGetImageMemoryRequirements2KHR");
		}

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		VkWin32SurfaceCreateInfoKHR SurfaceCreateInfo;
		Elaine::Memory::MemoryZero(SurfaceCreateInfo);
		SurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		SurfaceCreateInfo.hinstance = GetModuleHandle(NULL);
		SurfaceCreateInfo.hwnd = (HWND)mWindowHandle;
		vkCreateWin32SurfaceKHR(mInstance->GetInstance(), &SurfaceCreateInfo, VULKAN_CPU_ALLOCATOR, &mSurface);
#elif ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID
		//todo
#endif

		mDefaultCommandContext = new VulkanCommandContext(mDevice, mDevice->GetGraphicQueue());
		VulkanCommandContext* CurrentCtx = static_cast<VulkanCommandContext*>(mDefaultCommandContext);
		CurrentCtx->Initilize();
		CurrentCtx->mDevice = mDevice;
		CurrentCtx->mInstance = mInstance;
		CurrentCtx->mPhyDevice = mPhyDevice;

		//todo async
		if (mDevice->GetGraphicQueue() != mDevice->GetComputeQueue())
		{
			mDefaultComputeContext = new VulkanCommandContext(mDevice, mDevice->GetComputeQueue());
			VulkanCommandContext* CurrentCtx = static_cast<VulkanCommandContext*>(mDefaultComputeContext);
			CurrentCtx->Initilize();
			CurrentCtx->mDevice = mDevice;
			CurrentCtx->mInstance = mInstance;
			CurrentCtx->mPhyDevice = mPhyDevice;
			mbCreateComputeCtx = true;
		}

		//todo async
		if (mDevice->GetGraphicQueue() != mDevice->GetTransferQueue())
		{
			mDefaultTransferContext = new VulkanCommandContext(mDevice, mDevice->GetTransferQueue());
			VulkanCommandContext* CurrentCtx = static_cast<VulkanCommandContext*>(mDefaultTransferContext);
			CurrentCtx->Initilize();
			CurrentCtx->mDevice = mDevice;
			CurrentCtx->mInstance = mInstance;
			CurrentCtx->mPhyDevice = mPhyDevice;
			mbCreateTransferCtx = true;
		}


		VkRenderPassCreateInfo RenderPassCreateInfo;
		Memory::MemoryZero(RenderPassCreateInfo);
		RenderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		VkAttachmentDescription ColorAttachmentDescription;
		Memory::MemoryZero(ColorAttachmentDescription);
		ColorAttachmentDescription.flags = 0;
		ColorAttachmentDescription.format = VK_FORMAT_B8G8R8A8_SRGB;//mViewport->GetSwapChain()->GetVkFormat();
		ColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		ColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		ColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		ColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		ColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		ColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		ColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription DepthAttachmentDescription;
		Memory::MemoryZero(DepthAttachmentDescription);
		DepthAttachmentDescription.flags = 0;
		DepthAttachmentDescription.format = EngineToVkTextureFormat(PF_DepthStencil, false);
		DepthAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		DepthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		DepthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		DepthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		std::vector<VkAttachmentDescription> AttachmentDescriptions{ ColorAttachmentDescription ,DepthAttachmentDescription };
		RenderPassCreateInfo.attachmentCount = AttachmentDescriptions.size();
		RenderPassCreateInfo.pAttachments = AttachmentDescriptions.data();


		VkAttachmentReference ColorAttachmentReference;
		ColorAttachmentReference.attachment = 0;
		ColorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference DepthAttachmentReference;
		DepthAttachmentReference.attachment = 1;
		DepthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription SubpassDescription;
		Memory::MemoryZero(SubpassDescription);
		SubpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		SubpassDescription.colorAttachmentCount = 1;
		SubpassDescription.pColorAttachments = &ColorAttachmentReference;
		SubpassDescription.pDepthStencilAttachment = &DepthAttachmentReference;

		VkSubpassDependency SubpassDependency;
		SubpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		SubpassDependency.dstSubpass = 0;
		SubpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		SubpassDependency.srcAccessMask = 0;
		SubpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		SubpassDependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
		RenderPassCreateInfo.subpassCount = 1;
		RenderPassCreateInfo.pSubpasses = &SubpassDescription;
		RenderPassCreateInfo.dependencyCount = 1;
		RenderPassCreateInfo.pDependencies = &SubpassDependency;

		mDefaultRenderPass = new VulkanRenderPass(mDevice, RenderPassCreateInfo);



		mViewport = new VulkanViewport(mDevice, mWindowHandle, InDesc.Width, InDesc.Height, false, PF_B8G8R8A8);

		new VulkanShaderCompileManager();

	}

	void VulkanDynamicRHI::Destroy()
	{
		if (mbCreateComputeCtx)
		{
			SAFE_DELETE(mDefaultComputeContext);
		}

		if (mbCreateTransferCtx)
		{
			SAFE_DELETE(mDefaultTransferContext);
		}
		SAFE_DELETE(mDefaultCommandContext);
		for (auto CurrentCtx : mCommandContexts)
		{
			SAFE_DELETE(CurrentCtx);
		}
		mCommandContexts.clear();
		SAFE_DELETE(mDevice);
		mPhyDevice->Destroy();
		SAFE_DELETE(mPhyDevice);
		mInstance->Destroy();
		SAFE_DELETE(mInstance);

		delete VulkanShaderCompileManager::instance();
	}

	Elaine::RHICommandContext* VulkanDynamicRHI::CreateCommandContex()
	{
		Elaine::RHICommandContext* NewCommandCtx = new VulkanCommandContext(mDevice, mDevice->GetGraphicQueue());
		mCommandContexts.push_back(NewCommandCtx);
		return NewCommandCtx;
	}

	void VulkanDynamicRHI::DestroyCommandContext(RHICommandContext* InCtx)
	{
		auto Iter = std::find(mCommandContexts.begin(), mCommandContexts.end(), InCtx);
		if (Iter == mCommandContexts.end())
			return;
		SAFE_DELETE(InCtx);
		mCommandContexts.erase(Iter);
	}

	void VulkanDynamicRHI::SubmitAllCommands()
	{

	}

	void VulkanDynamicRHI::WaitUntilIdle()
	{

	}
}