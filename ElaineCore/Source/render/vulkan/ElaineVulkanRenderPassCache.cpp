#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanRenderPassCache.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/common/ElaineRHIProtocol.h"

namespace VulkanRHI
{
	//=============================================================================
	// RenderPassKey
	//=============================================================================
	bool RenderPassKey::operator==(const RenderPassKey& Other) const
	{
		if (NumColorTargets != Other.NumColorTargets) return false;
		if (DepthFormat != Other.DepthFormat) return false;
		if (DepthLoadOp != Other.DepthLoadOp) return false;
		if (DepthStoreOp != Other.DepthStoreOp) return false;
		if (StencilLoadOp != Other.StencilLoadOp) return false;
		if (StencilStoreOp != Other.StencilStoreOp) return false;

		for (uint32 i = 0; i < NumColorTargets; ++i)
		{
			if (ColorFormats[i] != Other.ColorFormats[i]) return false;
			if (ColorFinalLayouts[i] != Other.ColorFinalLayouts[i]) return false;
			if (ColorLoadOps[i] != Other.ColorLoadOps[i]) return false;
			if (ColorStoreOps[i] != Other.ColorStoreOps[i]) return false;
			if (ColorSamples[i] != Other.ColorSamples[i]) return false;
		}
		return true;
	}

	size_t RenderPassKey::GetHash() const
	{
		size_t Hash = 0;
		auto HashCombine = [&Hash](size_t Value) {
			Hash ^= Value + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		};

		HashCombine(NumColorTargets);
		HashCombine(static_cast<size_t>(DepthFormat));
		HashCombine(static_cast<size_t>(DepthLoadOp));
		HashCombine(static_cast<size_t>(DepthStoreOp));
		HashCombine(static_cast<size_t>(StencilLoadOp));
		HashCombine(static_cast<size_t>(StencilStoreOp));

		for (uint32 i = 0; i < NumColorTargets; ++i)
		{
			HashCombine(static_cast<size_t>(ColorFormats[i]));
			HashCombine(static_cast<size_t>(ColorFinalLayouts[i]));
			HashCombine(static_cast<size_t>(ColorLoadOps[i]));
			HashCombine(static_cast<size_t>(ColorStoreOps[i]));
			HashCombine(static_cast<size_t>(ColorSamples[i]));
		}
		return Hash;
	}

	//=============================================================================
	// VulkanRenderPassCache
	//=============================================================================
	VulkanRenderPassCache::VulkanRenderPassCache(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{
	}

	VulkanRenderPassCache::~VulkanRenderPassCache()
	{
		Clear();
	}

	VkRenderPass VulkanRenderPassCache::GetOrCreateRenderPass(const RenderPassKey& Key)
	{
		auto It = mCache.find(Key);
		if (It != mCache.end())
		{
			return It->second;
		}

		VkRenderPass NewPass = CreateRenderPass(Key);
		mCache[Key] = NewPass;
		return NewPass;
	}

	VkRenderPass VulkanRenderPassCache::GetOrCreateRenderPass(const Elaine::RHIRenderPassInfo& Info)
	{
		RenderPassKey Key = BuildRenderPassKey(Info);
		return GetOrCreateRenderPass(Key);
	}

	RenderPassKey VulkanRenderPassCache::BuildRenderPassKey(const Elaine::RHIRenderPassInfo& Info)
	{
		RenderPassKey Key;

		// 填充颜色附件信息
		for (uint32 i = 0; i < Elaine::MaxSimultaneousRenderTargets; ++i)
		{
			const auto& ColorRT = Info.ColorRenderTargets[i];
			if (ColorRT.RenderTarget == nullptr) break;

			VulkanTexture* VkTexture = static_cast<VulkanTexture*>(ColorRT.RenderTarget);
			Key.ColorFormats[Key.NumColorTargets] = VkTexture->GetVkFormat();
			Key.ColorLoadOps[Key.NumColorTargets] = GetVkLoadOp(ColorRT.Action);
			Key.ColorStoreOps[Key.NumColorTargets] = GetVkStoreOp(ColorRT.Action);
			Key.ColorFinalLayouts[Key.NumColorTargets] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			if (VkTexture->IsSwapchainImage() && (Key.ColorStoreOps[Key.NumColorTargets] == VK_ATTACHMENT_STORE_OP_STORE))
			{
				Key.ColorFinalLayouts[Key.NumColorTargets] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			}
			Key.ColorSamples[Key.NumColorTargets] = VK_SAMPLE_COUNT_1_BIT;
			Key.NumColorTargets++;
		}

		// 填充深度附件信息
		if (Info.DepthStencilRenderTarget.DepthStencilTarget != nullptr)
		{
			VulkanTexture* VkDepthTexture = static_cast<VulkanTexture*>(Info.DepthStencilRenderTarget.DepthStencilTarget);
			Key.DepthFormat = VkDepthTexture->GetVkFormat();
			Key.DepthLoadOp = GetVkDepthLoadOp(Info.DepthStencilRenderTarget.Action);
			Key.DepthStoreOp = GetVkDepthStoreOp(Info.DepthStencilRenderTarget.Action);
			Key.StencilLoadOp = GetVkStencilLoadOp(Info.DepthStencilRenderTarget.Action);
			Key.StencilStoreOp = GetVkStencilStoreOp(Info.DepthStencilRenderTarget.Action);
			Key.DepthSamples = VK_SAMPLE_COUNT_1_BIT;
		}

		return Key;
	}

	void VulkanRenderPassCache::Clear()
	{
		for (auto& Pair : mCache)
		{
			if (Pair.second != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(mDevice->GetDevice(), Pair.second, nullptr);
			}
		}
		mCache.clear();
	}

	VkRenderPass VulkanRenderPassCache::CreateRenderPass(const RenderPassKey& Key)
	{
		std::vector<VkAttachmentDescription> Attachments;
		std::vector<VkAttachmentReference> ColorRefs;
		VkAttachmentReference DepthRef = {};
		bool bHasDepth = false;

		// 颜色附件
		for (uint32 i = 0; i < Key.NumColorTargets; ++i)
		{
			VkAttachmentDescription Attachment = {};
			Attachment.format = Key.ColorFormats[i];
			Attachment.samples = Key.ColorSamples[i];
			Attachment.loadOp = Key.ColorLoadOps[i];
			Attachment.storeOp = Key.ColorStoreOps[i];
			Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			Attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			Attachment.finalLayout = Key.ColorFinalLayouts[i];

			VkAttachmentReference Ref = {};
			Ref.attachment = static_cast<uint32>(Attachments.size());
			Ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			Attachments.push_back(Attachment);
			ColorRefs.push_back(Ref);
		}

		// 深度附件
		if (Key.DepthFormat != VK_FORMAT_UNDEFINED)
		{
			VkAttachmentDescription Attachment = {};
			Attachment.format = Key.DepthFormat;
			Attachment.samples = Key.DepthSamples;
			Attachment.loadOp = Key.DepthLoadOp;
			Attachment.storeOp = Key.DepthStoreOp;
			Attachment.stencilLoadOp = Key.StencilLoadOp;
			Attachment.stencilStoreOp = Key.StencilStoreOp;
			Attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			Attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			DepthRef.attachment = static_cast<uint32>(Attachments.size());
			DepthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			bHasDepth = true;

			Attachments.push_back(Attachment);
		}

		// Subpass
		VkSubpassDescription Subpass = {};
		Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		Subpass.colorAttachmentCount = static_cast<uint32>(ColorRefs.size());
		Subpass.pColorAttachments = ColorRefs.empty() ? nullptr : ColorRefs.data();
		Subpass.pDepthStencilAttachment = bHasDepth ? &DepthRef : nullptr;

		// RenderPass
		VkRenderPassCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		CreateInfo.attachmentCount = static_cast<uint32>(Attachments.size());
		CreateInfo.pAttachments = Attachments.data();
		CreateInfo.subpassCount = 1;
		CreateInfo.pSubpasses = &Subpass;

		VkRenderPass RenderPass;
		VkResult Result = vkCreateRenderPass(mDevice->GetDevice(), &CreateInfo, nullptr, &RenderPass);
		if (Result != VK_SUCCESS)
		{
			LOG_ERROR("VulkanRHI: Failed to create RenderPass!");
			return VK_NULL_HANDLE;
		}

		return RenderPass;
	}

	//=============================================================================
	// FramebufferKey
	//=============================================================================
	bool FramebufferKey::operator==(const FramebufferKey& Other) const
	{
		if (RenderPass != Other.RenderPass) return false;
		if (NumAttachments != Other.NumAttachments) return false;
		if (Width != Other.Width) return false;
		if (Height != Other.Height) return false;
		if (Layers != Other.Layers) return false;

		for (uint32 i = 0; i < NumAttachments; ++i)
		{
			if (Attachments[i] != Other.Attachments[i]) return false;
		}
		return true;
	}

	size_t FramebufferKey::GetHash() const
	{
		size_t Hash = 0;
		auto HashCombine = [&Hash](size_t Value) {
			Hash ^= Value + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
		};

		HashCombine(reinterpret_cast<size_t>(RenderPass));
		HashCombine(NumAttachments);
		HashCombine(Width);
		HashCombine(Height);
		HashCombine(Layers);

		for (uint32 i = 0; i < NumAttachments; ++i)
		{
			HashCombine(reinterpret_cast<size_t>(Attachments[i]));
		}
		return Hash;
	}

	//=============================================================================
	// VulkanFramebufferCache
	//=============================================================================
	VulkanFramebufferCache::VulkanFramebufferCache(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{
	}

	VulkanFramebufferCache::~VulkanFramebufferCache()
	{
		Clear();
	}

	VkFramebuffer VulkanFramebufferCache::GetOrCreateFramebuffer(const FramebufferKey& Key)
	{
		auto It = mCache.find(Key);
		if (It != mCache.end())
		{
			return It->second;
		}

		// 创建新 Framebuffer
		VkFramebufferCreateInfo CreateInfo = {};
		CreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		CreateInfo.renderPass = Key.RenderPass;
		CreateInfo.attachmentCount = Key.NumAttachments;
		CreateInfo.pAttachments = Key.Attachments.data();
		CreateInfo.width = Key.Width;
		CreateInfo.height = Key.Height;
		CreateInfo.layers = Key.Layers;

		VkFramebuffer Framebuffer;
		VkResult Result = vkCreateFramebuffer(mDevice->GetDevice(), &CreateInfo, nullptr, &Framebuffer);
		if (Result != VK_SUCCESS)
		{
			LOG_ERROR("VulkanRHI: Failed to create Framebuffer!");
			return VK_NULL_HANDLE;
		}

		mCache[Key] = Framebuffer;
		return Framebuffer;
	}

	void VulkanFramebufferCache::Clear()
	{
		for (auto& Pair : mCache)
		{
			if (Pair.second != VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(mDevice->GetDevice(), Pair.second, nullptr);
			}
		}
		mCache.clear();
	}
}
