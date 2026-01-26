#pragma once
#include "render/vulkan/ElaineVulkanTypes.h"
#include "render/common/ElaineRHIProtocol.h"

namespace VulkanRHI
{
	class VulkanDevice;

	/**
	 * Vulkan采样器缓存
	 * - 根据RHISamplerType枚举创建并缓存VkSampler对象
	 * - 上层无需关心VkSampler的创建和管理
	 * - 引擎初始化时创建所有预定义采样器
	 */
	class ElaineCoreExport VulkanSamplerCache
	{
	public:
		VulkanSamplerCache(VulkanDevice* InDevice);
		~VulkanSamplerCache();

		// 初始化所有预定义采样器
		void Initialize();
		
		// 销毁所有采样器
		void Destroy();

		// 根据类型获取采样器 (核心接口)
		VkSampler GetSampler(Elaine::RHISamplerType InType);

		// 获取默认采样器
		VkSampler GetDefaultSampler() { return GetSampler(Elaine::SAMPLER_DEFAULT); }

	private:
		// 根据类型创建采样器
		VkSampler CreateSamplerByType(Elaine::RHISamplerType InType);

		// 获取过滤模式
		VkFilter GetFilterMode(Elaine::RHISamplerType InType);
		VkSamplerMipmapMode GetMipmapMode(Elaine::RHISamplerType InType);
		
		// 获取寻址模式
		VkSamplerAddressMode GetAddressMode(Elaine::RHISamplerType InType);

		// 是否启用各向异性
		bool IsAnisotropyEnabled(Elaine::RHISamplerType InType);
		
		// 是否启用比较操作(用于阴影贴图)
		bool IsCompareEnabled(Elaine::RHISamplerType InType);

	private:
		VulkanDevice* mDevice = nullptr;
		VkSampler mCachedSamplers[Elaine::SAMPLER_TYPE_COUNT] = { VK_NULL_HANDLE };
		bool mInitialized = false;
	};
}
