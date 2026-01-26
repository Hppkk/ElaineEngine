#pragma once
#include "ElaineCorePrerequirements.h"
#include "render/vulkan/ElaineVulkanTypes.h"
#include <unordered_map>
#include <array>

namespace VulkanRHI
{
	class VulkanDevice;

	//=============================================================================
	// RenderPassKey - 用于 RenderPass 缓存的 Key
	//=============================================================================
	struct RenderPassKey
	{
		static const uint32 MaxColorAttachments = 8;

		uint32 NumColorTargets = 0;
		std::array<VkFormat, MaxColorAttachments> ColorFormats = {};
		std::array<VkImageLayout, MaxColorAttachments> ColorFinalLayouts = {};
		std::array<VkAttachmentLoadOp, MaxColorAttachments> ColorLoadOps = {};
		std::array<VkAttachmentStoreOp, MaxColorAttachments> ColorStoreOps = {};
		std::array<VkSampleCountFlagBits, MaxColorAttachments> ColorSamples = {};

		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
		VkAttachmentLoadOp DepthLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp DepthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkAttachmentLoadOp StencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp StencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkSampleCountFlagBits DepthSamples = VK_SAMPLE_COUNT_1_BIT;

		bool operator==(const RenderPassKey& Other) const;

		size_t GetHash() const;

		// NumColorAttachments 别名，供 Pipeline 使用
		uint32 GetNumColorAttachments() const { return NumColorTargets; }
	};

	//=============================================================================
	// RenderPassKeyHasher - 哈希函数
	//=============================================================================
	struct RenderPassKeyHasher
	{
		size_t operator()(const RenderPassKey& Key) const
		{
			return Key.GetHash();
		}
	};

	//=============================================================================
	// VulkanRenderPassCache - RenderPass 缓存
	//=============================================================================
	class ElaineCoreExport VulkanRenderPassCache
	{
	public:
		VulkanRenderPassCache(VulkanDevice* InDevice);
		~VulkanRenderPassCache();

		// 根据 Key 获取或创建 RenderPass
		VkRenderPass GetOrCreateRenderPass(const RenderPassKey& Key);

		// 从 RHIRenderPassInfo 创建 Key 并获取 RenderPass
		VkRenderPass GetOrCreateRenderPass(const Elaine::RHIRenderPassInfo& Info);

		// 从 RHIRenderPassInfo 构建 Key（供外部使用）
		static RenderPassKey BuildRenderPassKey(const Elaine::RHIRenderPassInfo& Info);

		// 清空缓存
		void Clear();

		// 获取缓存统计
		size_t GetCacheSize() const { return mCache.size(); }

	private:
		VkRenderPass CreateRenderPass(const RenderPassKey& Key);

		VulkanDevice* mDevice = nullptr;
		std::unordered_map<RenderPassKey, VkRenderPass, RenderPassKeyHasher> mCache;
	};

	//=============================================================================
	// VulkanFramebufferCache - Framebuffer 缓存 (简化版)
	//=============================================================================
	struct FramebufferKey
	{
		VkRenderPass RenderPass = VK_NULL_HANDLE;
		std::array<VkImageView, 9> Attachments = {};  // 8 color + 1 depth
		uint32 NumAttachments = 0;
		uint32 Width = 0;
		uint32 Height = 0;
		uint32 Layers = 1;

		bool operator==(const FramebufferKey& Other) const;
		size_t GetHash() const;
	};

	struct FramebufferKeyHasher
	{
		size_t operator()(const FramebufferKey& Key) const
		{
			return Key.GetHash();
		}
	};

	class ElaineCoreExport VulkanFramebufferCache
	{
	public:
		VulkanFramebufferCache(VulkanDevice* InDevice);
		~VulkanFramebufferCache();

		VkFramebuffer GetOrCreateFramebuffer(const FramebufferKey& Key);
		void Clear();

	private:
		VulkanDevice* mDevice = nullptr;
		std::unordered_map<FramebufferKey, VkFramebuffer, FramebufferKeyHasher> mCache;
	};
}
