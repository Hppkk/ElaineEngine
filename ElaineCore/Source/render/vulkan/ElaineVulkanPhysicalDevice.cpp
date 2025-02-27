#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanInstance.h"

namespace VulkanRHI
{
	VulkanPhysicalDevice::VulkanPhysicalDevice(VulkanInstance* instance)
		: mHandle(nullptr)
        , mInstance(instance)
	{
		Elaine::Memory::MemoryZero(mGpuProps);
		Elaine::Memory::MemoryZero(mPhysicalFeatures);

	}

	bool VulkanPhysicalDevice::Initilize()
	{
        uint32 UPhysicalDeviceCount;
        vkEnumeratePhysicalDevices(mInstance->GetInstance(), &UPhysicalDeviceCount, nullptr);
        if (UPhysicalDeviceCount == 0)
        {
            LOG_ERROR("VulkanRHI: enumerate physical devices failed, find zero!");
            return false;
        }
        else
        {
            //TODO mutil phydevice
            // find one device that matches our requirement
            // or find which is the best
            std::vector<VkPhysicalDevice> physical_devices(UPhysicalDeviceCount);
            vkEnumeratePhysicalDevices(mInstance->GetInstance(), &UPhysicalDeviceCount, physical_devices.data());

            std::vector<std::pair<int, VkPhysicalDevice>> ranked_physical_devices;
            for (const auto& device : physical_devices)
            {
                VkPhysicalDeviceProperties PhyDevicePorperties;
                vkGetPhysicalDeviceProperties(device, &PhyDevicePorperties);
                int score = 0;

                if (PhyDevicePorperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    score += 1000;
                }
                else if (PhyDevicePorperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
                {
                    score += 100;
                }

                ranked_physical_devices.push_back({ score, device });
            }

            std::sort(ranked_physical_devices.begin(),
                ranked_physical_devices.end(),
                [](const std::pair<int, VkPhysicalDevice>& p1, const std::pair<int, VkPhysicalDevice>& p2)
                {
                    return p1 > p2;
                });

            for (const auto& device : ranked_physical_devices)
            {
                if (IsDeviceSuitable(device.second))
                {
                    mHandle = device.second;
                    vkGetPhysicalDeviceProperties(mHandle, &mGpuProps);
                    break;
                }
            }

            if (mHandle == VK_NULL_HANDLE)
            {
                LOG_ERROR("VulkanRHI: failed to find suitable physical device.");
                return false;
            }
        }

		uint32 QueueCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &QueueCount, nullptr);
		mQueueFamilyProps.resize(QueueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(mHandle, &QueueCount, mQueueFamilyProps.data());
		vkGetPhysicalDeviceFeatures(mHandle, &mPhysicalFeatures);
        return true;
	}

    void VulkanPhysicalDevice::Destroy()
    {
        
    }

    bool VulkanPhysicalDevice::IsDeviceSuitable(VkPhysicalDevice physicalDevice)
    {
        bool is_extensions_supported = CheckDeviceExtensionSupport(physicalDevice);

        return true;
    }

    bool VulkanPhysicalDevice::CheckDeviceExtensionSupport(VkPhysicalDevice PhysicalDevice)
    {
        uint32_t ExtensionCount;
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionCount, nullptr);

        mDeviceExtensions.resize(ExtensionCount);
        vkEnumerateDeviceExtensionProperties(PhysicalDevice, nullptr, &ExtensionCount, mDeviceExtensions.data());

        return !mDeviceExtensions.empty();
    }
}