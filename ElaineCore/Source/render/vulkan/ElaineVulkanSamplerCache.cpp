#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanSamplerCache.h"
#include "render/vulkan/ElaineVulkanDevice.h"

namespace VulkanRHI
{
	VulkanSamplerCache::VulkanSamplerCache(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{
	}

	VulkanSamplerCache::~VulkanSamplerCache()
	{
		Destroy();
	}

	void VulkanSamplerCache::Initialize()
	{
		if (mInitialized)
			return;

		// 预创建所有类型的采样器
		for (int i = 0; i < Elaine::SAMPLER_TYPE_COUNT; ++i)
		{
			mCachedSamplers[i] = CreateSamplerByType(static_cast<Elaine::RHISamplerType>(i));
		}
		
		mInitialized = true;
	}

	void VulkanSamplerCache::Destroy()
	{
		if (!mInitialized)
			return;

		VkDevice Device = mDevice->GetDevice();
		for (int i = 0; i < Elaine::SAMPLER_TYPE_COUNT; ++i)
		{
			if (mCachedSamplers[i] != VK_NULL_HANDLE)
			{
				vkDestroySampler(Device, mCachedSamplers[i], nullptr);
				mCachedSamplers[i] = VK_NULL_HANDLE;
			}
		}
		
		mInitialized = false;
	}

	VkSampler VulkanSamplerCache::GetSampler(Elaine::RHISamplerType InType)
	{
		if (InType >= Elaine::SAMPLER_TYPE_COUNT)
			InType = Elaine::SAMPLER_DEFAULT;

		// 懒加载：如果尚未创建则创建
		if (mCachedSamplers[InType] == VK_NULL_HANDLE)
		{
			mCachedSamplers[InType] = CreateSamplerByType(InType);
		}

		return mCachedSamplers[InType];
	}

	VkSampler VulkanSamplerCache::CreateSamplerByType(Elaine::RHISamplerType InType)
	{
		VkSamplerCreateInfo SamplerInfo = {};
		SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		SamplerInfo.pNext = nullptr;
		SamplerInfo.flags = 0;

		// 设置过滤模式
		SamplerInfo.magFilter = GetFilterMode(InType);
		SamplerInfo.minFilter = GetFilterMode(InType);
		SamplerInfo.mipmapMode = GetMipmapMode(InType);

		// 设置寻址模式
		VkSamplerAddressMode AddressMode = GetAddressMode(InType);
		SamplerInfo.addressModeU = AddressMode;
		SamplerInfo.addressModeV = AddressMode;
		SamplerInfo.addressModeW = AddressMode;

		// Mipmap设置
		SamplerInfo.mipLodBias = 0.0f;
		SamplerInfo.minLod = 0.0f;
		SamplerInfo.maxLod = VK_LOD_CLAMP_NONE;  // 使用纹理的所有mip级别

		// 各向异性设置
		if (IsAnisotropyEnabled(InType))
		{
			SamplerInfo.anisotropyEnable = VK_TRUE;
			SamplerInfo.maxAnisotropy = 16.0f;  // 最大各向异性级别
		}
		else
		{
			SamplerInfo.anisotropyEnable = VK_FALSE;
			SamplerInfo.maxAnisotropy = 1.0f;
		}

		// 比较操作 (用于阴影贴图)
		if (IsCompareEnabled(InType))
		{
			SamplerInfo.compareEnable = VK_TRUE;
			SamplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		}
		else
		{
			SamplerInfo.compareEnable = VK_FALSE;
			SamplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		}

		// 边界颜色 (用于Clamp模式超出边界时)
		SamplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		SamplerInfo.unnormalizedCoordinates = VK_FALSE;

		VkSampler Sampler = VK_NULL_HANDLE;
		VkResult Result = vkCreateSampler(mDevice->GetDevice(), &SamplerInfo, nullptr, &Sampler);
		if (Result != VK_SUCCESS)
		{
			// TODO: 错误处理
			return VK_NULL_HANDLE;
		}

		return Sampler;
	}

	VkFilter VulkanSamplerCache::GetFilterMode(Elaine::RHISamplerType InType)
	{
		switch (InType)
		{
		case Elaine::SAMPLER_NEAREST_CLAMP:
		case Elaine::SAMPLER_NEAREST_REPEAT:
		case Elaine::SAMPLER_UI:
			return VK_FILTER_NEAREST;

		case Elaine::SAMPLER_LINEAR_CLAMP:
		case Elaine::SAMPLER_LINEAR_REPEAT:
		case Elaine::SAMPLER_LINEAR_MIRROR:
		case Elaine::SAMPLER_ANISO_CLAMP:
		case Elaine::SAMPLER_ANISO_REPEAT:
		case Elaine::SAMPLER_SHADOW:
		case Elaine::SAMPLER_CUBEMAP:
		case Elaine::SAMPLER_DEPTH:
		default:
			return VK_FILTER_LINEAR;
		}
	}

	VkSamplerMipmapMode VulkanSamplerCache::GetMipmapMode(Elaine::RHISamplerType InType)
	{
		switch (InType)
		{
		case Elaine::SAMPLER_NEAREST_CLAMP:
		case Elaine::SAMPLER_NEAREST_REPEAT:
		case Elaine::SAMPLER_UI:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	}

	VkSamplerAddressMode VulkanSamplerCache::GetAddressMode(Elaine::RHISamplerType InType)
	{
		switch (InType)
		{
		case Elaine::SAMPLER_LINEAR_REPEAT:
		case Elaine::SAMPLER_NEAREST_REPEAT:
		case Elaine::SAMPLER_ANISO_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;

		case Elaine::SAMPLER_LINEAR_MIRROR:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

		case Elaine::SAMPLER_LINEAR_CLAMP:
		case Elaine::SAMPLER_NEAREST_CLAMP:
		case Elaine::SAMPLER_ANISO_CLAMP:
		case Elaine::SAMPLER_SHADOW:
		case Elaine::SAMPLER_CUBEMAP:
		case Elaine::SAMPLER_UI:
		case Elaine::SAMPLER_DEPTH:
		default:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
	}

	bool VulkanSamplerCache::IsAnisotropyEnabled(Elaine::RHISamplerType InType)
	{
		switch (InType)
		{
		case Elaine::SAMPLER_ANISO_CLAMP:
		case Elaine::SAMPLER_ANISO_REPEAT:
		case Elaine::SAMPLER_CUBEMAP:
			return true;

		default:
			return false;
		}
	}

	bool VulkanSamplerCache::IsCompareEnabled(Elaine::RHISamplerType InType)
	{
		return InType == Elaine::SAMPLER_SHADOW;
	}
}
