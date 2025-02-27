#pragma once
#include "ElaineCorePrerequirements.h"

namespace VulkanRHI
{
	class VulkanDevice;

	class ElaineCoreExport VulkanRenderPass
	{
	public:
		VulkanRenderPass(VulkanDevice* InDevice, const VkRenderPassCreateInfo& InCreateInfo);
		~VulkanRenderPass();
		VkRenderPass GetHandle() const { return mHandle; }
	private:
		VulkanDevice* mDevice = nullptr;
		VkRenderPass mHandle = nullptr;
	
	};
}