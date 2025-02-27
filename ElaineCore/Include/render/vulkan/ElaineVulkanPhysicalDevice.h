

namespace VulkanRHI
{
	class VulkanInstance;

	class ElaineCoreExport VulkanPhysicalDevice
	{
	public:
		VulkanPhysicalDevice(VulkanInstance* instance);

		bool Initilize();

		void Destroy();


		const VkPhysicalDevice GetPhysicalDeviceHandle() const
		{
			return mHandle;
		}

		const VkPhysicalDeviceProperties& GetDeviceProperties() const
		{
			return mGpuProps;
		}

		inline const VkPhysicalDeviceLimits& GetLimits() const
		{
			return mGpuProps.limits;
		}

		const std::vector<VkExtensionProperties>& GetDeviceExtensions() const
		{
			return mDeviceExtensions;
		}

		const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProps() const
		{
			return mQueueFamilyProps;
		}

		const VkPhysicalDeviceFeatures& GetPhysicalFeatures() const
		{
			return mPhysicalFeatures;
		}

		VulkanInstance* GetVulkanInstance() const
		{
			return mInstance;
		}

	private:

		bool IsDeviceSuitable(VkPhysicalDevice physicalDevice);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice physical_device);
	private:
		VkPhysicalDevice						mHandle;
		VulkanInstance*							mInstance;
		VkPhysicalDeviceProperties				mGpuProps;
		std::vector<VkQueueFamilyProperties>	mQueueFamilyProps;
		VkPhysicalDeviceFeatures				mPhysicalFeatures;
		std::vector<VkExtensionProperties>		mDeviceExtensions;
	};
}