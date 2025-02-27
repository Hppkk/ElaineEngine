#pragma once

namespace VulkanRHI
{
	class ElaineCoreExport VulkanInstance
	{
	public:
		VulkanInstance();
		void Initilize();
		void Destroy();
		const VkInstance GetInstance() const
		{
			return mHandle;
		}

		const uint32 GetApiVersion() const
		{
			return mVersion;
		}

		const std::vector<char const*>& GetLayers() const
		{
			return mLayers;
		}

	private:
		void CreateInstance();
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	private:
		VkInstance						mHandle;
		uint32							mVersion;
		bool							mEnableDebugUtilsLabel;
		VkDebugUtilsMessengerEXT		mDebugMessenger;
		const std::vector<char const*>	mLayers { "VK_LAYER_KHRONOS_validation" };
	};
}