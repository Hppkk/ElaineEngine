#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanBarrier.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "render/vulkan/ElaineVulkanTexture.h"

namespace VulkanRHI
{
	static VkPipelineStageFlags GetVkStageFlagsForLayout(VkImageLayout Layout)
	{
		VkPipelineStageFlags Flags = 0;

		switch (Layout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			Flags = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;

		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			Flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR:
			Flags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR:
			Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			Flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			break;

		case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
			Flags = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
			break;

		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			Flags = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
			break;

		case VK_IMAGE_LAYOUT_GENERAL:
		case VK_IMAGE_LAYOUT_UNDEFINED:
			Flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;

		default:
			break;
		}

		return Flags;
	}

	static VkAccessFlags GetVkAccessMaskForLayout(VkImageLayout Layout)
	{
		VkAccessFlags Flags = 0;

		switch (Layout)
		{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			Flags = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			Flags = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			Flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR:
			Flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR:
			Flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;

		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			Flags = VK_ACCESS_SHADER_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
		case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR:
		case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR:
			Flags = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			break;

		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			Flags = 0;
			break;

		case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
			Flags = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
			break;

		case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
			Flags = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
			break;

		case VK_IMAGE_LAYOUT_GENERAL:
		case VK_IMAGE_LAYOUT_UNDEFINED:
			Flags = 0;
			break;

		default:
			break;
		}

		return Flags;
	}

	static void SetupImageBarrier(VkImageMemoryBarrier& ImgBarrier, VkImage Image, VkAccessFlags SrcAccessFlags, VkAccessFlags DstAccessFlags, VkImageLayout SrcLayout, VkImageLayout DstLayout, const VkImageSubresourceRange& SubresRange)
	{
		ImgBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		ImgBarrier.pNext = nullptr;
		ImgBarrier.srcAccessMask = SrcAccessFlags;
		ImgBarrier.dstAccessMask = DstAccessFlags;
		ImgBarrier.oldLayout = SrcLayout;
		ImgBarrier.newLayout = DstLayout;
		ImgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		ImgBarrier.image = Image;
		ImgBarrier.subresourceRange = SubresRange;
	}

	void VulkanPipelineBarrier::AddMemoryBarrier(VkAccessFlags InSrcAccessFlags, VkAccessFlags InDstAccessFlags, VkPipelineStageFlags InSrcStageMask, VkPipelineStageFlags InDstStageMask)
	{
		const VkAccessFlags ReadMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
			VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_MEMORY_READ_BIT;
		const bool SrcAccessIsRead = ((InSrcAccessFlags & (~ReadMask)) == 0);
		if (!SrcAccessIsRead)
		{
			mMemoryBarrier.srcAccessMask |= InSrcAccessFlags;
			mMemoryBarrier.dstAccessMask |= InDstAccessFlags;
		}
		mSrcStageMask |= InSrcStageMask;
		mDstStageMask |= InDstStageMask;
		mbHasMemoryBarrier = true;
	}
	void VulkanPipelineBarrier::AddImageLayoutTransition(VkImage InImage, VkImageLayout InSrcLayout, VkImageLayout InDstLayout, const VkImageSubresourceRange& InSubresourceRange)
	{
		mSrcStageMask |= GetVkStageFlagsForLayout(InSrcLayout);
		mDstStageMask |= GetVkStageFlagsForLayout(InDstLayout);
		VkAccessFlags SrcAccessFlags = GetVkAccessMaskForLayout(InSrcLayout);
		VkAccessFlags DstAccessFlags = GetVkAccessMaskForLayout(InDstLayout);
		mImageBarriers.push_back({});
		VkImageMemoryBarrier& ImageBarrier = mImageBarriers.back();
		SetupImageBarrier(ImageBarrier, InImage, SrcAccessFlags, DstAccessFlags, InSrcLayout, InDstLayout, InSubresourceRange);
	}
	void VulkanPipelineBarrier::AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, const struct VulkanImageLayout& InInSrcLayout, VkImageLayout InDstLayout)
	{

	}
	void VulkanPipelineBarrier::AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, VkImageLayout InSrcLayout, const struct VulkanImageLayout& InDstLayout)
	{

	}
	void VulkanPipelineBarrier::AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, const struct VulkanImageLayout& InSrcLayout,
		const struct VulkanImageLayout& InDstLayout)
	{

	}
	void VulkanPipelineBarrier::AddImageAccessTransition(const VulkanTexture& InSurface, Elaine::ERHIAccess InSrcAccess,
		Elaine::ERHIAccess InDstAccess, const VkImageSubresourceRange& InSubresourceRange, VkImageLayout& InOutLayout)
	{

	}
	void VulkanPipelineBarrier::Execute(VkCommandBuffer InCmdBuffer)
	{
		if (mbHasMemoryBarrier || mImageBarriers.size() != 0)
		{
			vkCmdPipelineBarrier(InCmdBuffer, mSrcStageMask, mDstStageMask, 0, mbHasMemoryBarrier ? 1 : 0, &mMemoryBarrier, 0, nullptr, mImageBarriers.size(), mImageBarriers.data());
		}
	}

	VkImageSubresourceRange VulkanPipelineBarrier::MakeSubresourceRange(VkImageAspectFlags InAspectMask, uint32 InFirstMip,
		uint32 InNumMips, uint32 InFirstLayer, uint32 InNumLayers)
	{
		VkImageSubresourceRange Range;
		Range.aspectMask = InAspectMask;
		Range.baseMipLevel = InFirstMip;
		Range.levelCount = InNumMips;
		Range.baseArrayLayer = InFirstLayer;
		Range.layerCount = InNumLayers;
		return Range;
	}

	void VulkanSetImageLayout(VkCommandBuffer InCmdBuffer, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, const VkImageSubresourceRange& InSubresourceRange)
	{
		VulkanPipelineBarrier Barrier;
		Barrier.AddImageLayoutTransition(InImage, InOldLayout, InNewLayout, InSubresourceRange);
		Barrier.Execute(InCmdBuffer);
	}
}