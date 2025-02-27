#pragma once
#include "ElainePrecompiledHeader.h"
#include "vulkan/ElaineVulkanRenderPass.h"
#include "vulkan/ElaineVulkanDevice.h"

namespace VulkanRHI
{
	VulkanRenderPass::VulkanRenderPass(VulkanDevice* InDevice, const VkRenderPassCreateInfo& InCreateInfo)
		:mDevice(InDevice)
	{
		vkCreateRenderPass(mDevice->GetDevice(), &InCreateInfo, VULKAN_CPU_ALLOCATOR, &mHandle);
	}

	VulkanRenderPass::~VulkanRenderPass()
	{
		vkDestroyRenderPass(mDevice->GetDevice(), mHandle, VULKAN_CPU_ALLOCATOR);
		mHandle = nullptr;
	}
}