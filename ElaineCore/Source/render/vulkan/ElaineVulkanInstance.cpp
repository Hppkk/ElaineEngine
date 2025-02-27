#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanInstance.h"

namespace VulkanRHI
{
    VulkanInstance::VulkanInstance()
        : mHandle(nullptr)
        , mVersion(0u)
        , mEnableDebugUtilsLabel(true)
        , mDebugMessenger(nullptr)
    {

    }

    void VulkanInstance::Initilize()
    {
        CreateInstance();

        if (Root::instance()->isEnableVulkanValidationLayer())
        {
            VkDebugUtilsMessengerCreateInfoEXT createInfo;
            PopulateDebugMessengerCreateInfo(createInfo);
            if (VK_SUCCESS != CreateDebugUtilsMessengerEXT(mHandle, &createInfo, nullptr, &mDebugMessenger))
            {
                LOG_ERROR("failed to set up debug messenger!");
            }
        }
    }

    void VulkanInstance::Destroy()
    {
        DestroyDebugUtilsMessengerEXT(mHandle, mDebugMessenger, nullptr);
        vkDestroyInstance(mHandle, nullptr);
    }

    bool VulkanInstance::CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties.layerName) == 0)
            {
                return true;
            }
        }

        return false;
    }

    void VulkanInstance::CreateInstance()
    {
        if (Elaine::Root::instance()->isEnableVulkanValidationLayer())
        {
            if (!CheckValidationLayerSupport())
            {
                LOG_INFO("VulkanRHI: Validation layer is not support.");
            }
        }

        mVersion = VK_API_VERSION_1_3;
        // app info
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "ElaineRenderer";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "ElaineEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = mVersion;

        // create info
        VkInstanceCreateInfo instance_create_info{};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &appInfo; // the appInfo is stored here

        auto extensions = GetRequiredExtensions();
        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        instance_create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (Elaine::Root::instance()->isEnableVulkanValidationLayer())
        {
            instance_create_info.enabledLayerCount = static_cast<uint32_t>(mLayers.size());
            instance_create_info.ppEnabledLayerNames = mLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            instance_create_info.pNext = &debugCreateInfo;
        }
        else
        {
            instance_create_info.enabledLayerCount = 0;
            instance_create_info.pNext = nullptr;
        }

        if (vkCreateInstance(&instance_create_info, nullptr, &mHandle) != VK_SUCCESS)
        {
            LOG_ERROR("VulkanRHI: vulkan create instance failed!");
        }
    }

    std::vector<const char*> VulkanInstance::GetRequiredExtensions()
    {
        uint32_t     glfwExtensionCount = 0;
        //const char** glfwExtensions;
        //vkEnumerateInstanceExtensionProperties
        //glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions{ "VK_KHR_surface" ,"VK_KHR_win32_surface"};

        if (Elaine::Root::instance()->isEnableVulkanValidationLayer() || mEnableDebugUtilsLabel)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

#if defined(__MACH__)
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
#endif

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT,
        VkDebugUtilsMessageTypeFlagsEXT,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*)
    {
        LOG_DEBUG("VulkanRHI: Validation Layer Messmage: " + pCallbackData->pMessage);
        return VK_FALSE;
    }

    void VulkanInstance::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func =
            (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT     debugMessenger,
        const VkAllocationCallbacks* pAllocator)
    {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            func(instance, debugMessenger, pAllocator);
        }
    }
}