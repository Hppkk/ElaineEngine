#pragma once
#include "render/common/ElaineRHITypes.h"

namespace VulkanRHI
{
	class ElaineCoreExport VulkanBarrier
	{
	public:
		
	};

	class VulkanSemaphore;
	class VulkanTexture;

	class ElaineCoreExport VulkanPipelineBarrier
	{
	public:

		VulkanPipelineBarrier()
		{
			Elaine::Memory::MemoryZero(mMemoryBarrier);
			mMemoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		}

		void AddMemoryBarrier(VkAccessFlags InSrcAccessFlags, VkAccessFlags InDstAccessFlags, VkPipelineStageFlags InSrcStageMask, VkPipelineStageFlags InDstStageMask);
		void AddImageLayoutTransition(VkImage InImage, VkImageLayout InSrcLayout, VkImageLayout InDstLayout, const VkImageSubresourceRange& InSubresourceRange);
		void AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, const struct VulkanImageLayout& InInSrcLayout, VkImageLayout InDstLayout);
		void AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, VkImageLayout InSrcLayout, const struct VulkanImageLayout& InDstLayout);
		void AddImageLayoutTransition(VkImage InImage, VkImageAspectFlags InAspectMask, const struct VulkanImageLayout& InSrcLayout, const struct VulkanImageLayout& InDstLayout);
		void AddImageAccessTransition(const VulkanTexture& InSurface, Elaine::ERHIAccess InSrcAccess, Elaine::ERHIAccess InDstAccess, const VkImageSubresourceRange& InSubresourceRange, VkImageLayout& InOutLayout);
		void Execute(VkCommandBuffer InCmdBuffer);

		static VkImageSubresourceRange MakeSubresourceRange(VkImageAspectFlags InAspectMask, uint32 InFirstMip = 0,
			uint32 InNumMips = VK_REMAINING_MIP_LEVELS, uint32 InFirstLayer = 0, uint32 InNumLayers = VK_REMAINING_ARRAY_LAYERS);

	private:
		Elaine::ERHIPipeline mSrcPipelines;
		Elaine::ERHIPipeline mDstPipelines;
		VkPipelineStageFlags mSrcStageMask = 0; 
		VkPipelineStageFlags mDstStageMask = 0;
		VkMemoryBarrier mMemoryBarrier;
		std::vector<VkImageMemoryBarrier> mImageBarriers;
		std::vector<VkBufferMemoryBarrier> mBufferBarriers;
		VulkanSemaphore* mSemaphore = nullptr;
		bool mbHasMemoryBarrier = false;
	};

	void VulkanSetImageLayout(VkCommandBuffer InCmdBuffer, VkImage InImage, VkImageLayout InOldLayout, VkImageLayout InNewLayout, const VkImageSubresourceRange& InSubresourceRange);

}