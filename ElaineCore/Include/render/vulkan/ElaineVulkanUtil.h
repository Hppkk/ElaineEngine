#pragma once

#define VMA_IMPLEMENTATION 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

namespace VulkanRHI
{
    template<typename BitsType>
    constexpr bool VKHasAnyFlags(VkFlags Flags, BitsType Contains)
    {
        return (Flags & Contains) != 0;
    }

    static inline bool IsAstcLdrFormat(VkFormat Format)
    {
        return Format >= VK_FORMAT_ASTC_4x4_UNORM_BLOCK && Format <= VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    }

    static inline bool IsAstcSrgbFormat(VkFormat Format)
    {
        switch (Format)
        {
        case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x4_SRGB_BLOCK:
        case VK_FORMAT_ASTC_5x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x5_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x6_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x8_SRGB_BLOCK:
        case VK_FORMAT_ASTC_10x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x10_SRGB_BLOCK:
        case VK_FORMAT_ASTC_12x12_SRGB_BLOCK:
            return true;
        default:
            return false;
        }
    }

    //class VulkanUtil
    //{
    //public:
    //    static uint32_t
    //        findMemoryType(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties_flag);
    //    static VkShaderModule createShaderModule(VkDevice device, const std::vector<unsigned char>& shader_code);
    //    static void           createBuffer(VkPhysicalDevice      physical_device,
    //        VkDevice              device,
    //        VkDeviceSize          size,
    //        VkBufferUsageFlags    usage,
    //        VkMemoryPropertyFlags properties,
    //        VkBuffer& buffer,
    //        VkDeviceMemory& buffer_memory);
    //    static void           createBufferAndInitialize(VkDevice              device,
    //        VkPhysicalDevice      physicalDevice,
    //        VkBufferUsageFlags    usageFlags,
    //        VkMemoryPropertyFlags memoryPropertyFlags,
    //        VkBuffer* buffer,
    //        VkDeviceMemory* memory,
    //        VkDeviceSize          size,
    //        void* data = nullptr,
    //        int                   datasize = 0);
    //    static void           copyBuffer(RHI* rhi,
    //        VkBuffer     srcBuffer,
    //        VkBuffer     dstBuffer,
    //        VkDeviceSize srcOffset,
    //        VkDeviceSize dstOffset,
    //        VkDeviceSize size);
    //    static void           createImage(VkPhysicalDevice      physical_device,
    //        VkDevice              device,
    //        uint32_t              image_width,
    //        uint32_t              image_height,
    //        VkFormat              format,
    //        VkImageTiling         image_tiling,
    //        VkImageUsageFlags     image_usage_flags,
    //        VkMemoryPropertyFlags memory_property_flags,
    //        VkImage& image,
    //        VkDeviceMemory& memory,
    //        VkImageCreateFlags    image_create_flags,
    //        uint32_t              array_layers,
    //        uint32_t              miplevels);
    //    static VkImageView    createImageView(VkDevice           device,
    //        VkImage& image,
    //        VkFormat           format,
    //        VkImageAspectFlags image_aspect_flags,
    //        VkImageViewType    view_type,
    //        uint32_t           layout_count,
    //        uint32_t           miplevels);
    //    static void           createGlobalImage(RHI* rhi,
    //        VkImage& image,
    //        VkImageView& image_view,
    //        VmaAllocation& image_allocation,
    //        uint32_t           texture_image_width,
    //        uint32_t           texture_image_height,
    //        void* texture_image_pixels,
    //        RHIFormat texture_image_format,
    //        uint32_t           miplevels = 0);
    //    static void           createCubeMap(RHI* rhi,
    //        VkImage& image,
    //        VkImageView& image_view,
    //        VmaAllocation& image_allocation,
    //        uint32_t             texture_image_width,
    //        uint32_t             texture_image_height,
    //        std::array<void*, 6> texture_image_pixels,
    //        RHIFormat   texture_image_format,
    //        uint32_t             miplevels);
    //    static void           generateTextureMipMaps(RHI* rhi,
    //        VkImage  image,
    //        VkFormat image_format,
    //        uint32_t texture_width,
    //        uint32_t texture_height,
    //        uint32_t layers,
    //        uint32_t miplevels);
    //    static void           transitionImageLayout(RHI* rhi,
    //        VkImage            image,
    //        VkImageLayout      old_layout,
    //        VkImageLayout      new_layout,
    //        uint32_t           layer_count,
    //        uint32_t           miplevels,
    //        VkImageAspectFlags aspect_mask_bits);
    //    static void           copyBufferToImage(RHI* rhi,
    //        VkBuffer buffer,
    //        VkImage  image,
    //        uint32_t width,
    //        uint32_t height,
    //        uint32_t layer_count);
    //    static void genMipmappedImage(RHI* rhi, VkImage image, uint32_t width, uint32_t height, uint32_t mip_levels);

    //    static VkSampler
    //        getOrCreateMipmapSampler(VkPhysicalDevice physical_device, VkDevice device, uint32_t width, uint32_t height);
    //    static void      destroyMipmappedSampler(VkDevice device);
    //    static VkSampler getOrCreateNearestSampler(VkPhysicalDevice physical_device, VkDevice device);
    //    static VkSampler getOrCreateLinearSampler(VkPhysicalDevice physical_device, VkDevice device);
    //    static void      destroyNearestSampler(VkDevice device);
    //    static void      destroyLinearSampler(VkDevice device);

    //private:
    //    static std::unordered_map<uint32_t, VkSampler> m_mipmap_sampler_map;
    //    static VkSampler                               m_nearest_sampler;
    //    static VkSampler                               m_linear_sampler;
    //};
}