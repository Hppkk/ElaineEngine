#pragma once
#include "render/common/ElaineRHI.h"
#include "render/vulkan/ElaineVulkanTypes.h"
#ifdef USE_VOLK
#include "Volk/volk.h"
#else
#include "vulkan.h"
#endif

namespace VulkanRHI
{
	class VulkanInstance;
	class VulkanDevice;
	class VulkanPhysicalDevice;
	class VulkanViewport;
	class VulkanRenderPass;
	class VulkanTexture;
	class VulkanTextureView;
#ifndef USE_VOLK
	extern PFN_vkGetImageMemoryRequirements2KHR vkGetImageMemoryRequirements2KHR;
	extern PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersion;
#endif

	class ElaineCoreExport VulkanDynamicRHI :public Elaine::DynamicRHI
	{
	public:
		VulkanDynamicRHI();
		~VulkanDynamicRHI();
		virtual void Initialize(const Elaine::RHI_PARAM_DESC& InDesc);
		virtual Elaine::RHICommandContext* CreateCommandContex() override;
		virtual void DestroyCommandContext(RHICommandContext* InCtx) override;
		virtual void Destroy();
		VulkanInstance* GetVkInstance() { return mInstance; }
		VulkanDevice* GetDevice() { return mDevice; }
		VulkanPhysicalDevice* GetPhyDevice() { return mPhyDevice; }
		VkSurfaceKHR GetVulkanSurface() const { return mSurface; }
		VulkanViewport* GetViewport() const { return mViewport; }
		VulkanRenderPass* GetDefaultRenderPass() const { return mDefaultRenderPass; }
		void SubmitAllCommands();
		void WaitUntilIdle();
	private:
		VulkanInstance* mInstance = nullptr;
		VulkanDevice* mDevice = nullptr;
		VulkanPhysicalDevice* mPhyDevice = nullptr;
		VkSurfaceKHR mSurface = nullptr;
		VulkanViewport* mViewport = nullptr;
		bool mbCreateComputeCtx = false;
		bool mbCreateTransferCtx = false;
		VulkanRenderPass* mDefaultRenderPass = nullptr;
	};

	VulkanDynamicRHI* GetVulkanDynamicRHI();
}