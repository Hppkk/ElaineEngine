#pragma once
#include "render/common/ElaineRHIProtocol.h"
#include "vulkan.h"


using namespace Elaine;

namespace VulkanRHI
{
    class VulkanDevice;
    class VulkanPhysicalDevice;
   

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR        capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR>   presentModes;
    };

    struct VulkanCpuReadbackBuffer
    {
        VkBuffer Buffer;
        uint32 MipOffsets[MAX_TEXTURE_MIP_COUNT];
        uint32 MipSize[MAX_TEXTURE_MIP_COUNT];
    };

    struct VulkanImageCreateInfo
    {
        VkImageCreateInfo mImageCreateInfo;
        VkImageFormatListCreateInfoKHR mImageFormatListCreateInfo;
        VkFormat mFormatsUsed[2];
    };

    // Converts the internal texture dimension to Vulkan view type
    inline VkImageViewType TextureDimensionToVkImageViewType(TextureDimension Dimension)
    {
        switch (Dimension)
        {
        case TextureDimension::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
        case TextureDimension::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        case TextureDimension::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
        case TextureDimension::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
        case TextureDimension::TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
        default: return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
        }
    }

    inline VkPrimitiveTopology TransRHIPrimitiveToVk(EPrimitiveType InRHIPrimitiveType)
    {
        switch (InRHIPrimitiveType)
        {
        case PT_TriangleList:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case PT_TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PT_LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PT_PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    inline VkPolygonMode TransRHIPolygonModeToVk(RHIPolygonMode InRHIPolygonMode)
    {
        switch (InRHIPolygonMode)
        {
        case POLYGON_FILL:
            return VK_POLYGON_MODE_FILL;
        case POLYGON_LINE:
            return VK_POLYGON_MODE_LINE;
        case POLYGON_POINT:
            return VK_POLYGON_MODE_POINT;
        default:
            return VK_POLYGON_MODE_FILL;
        }
    }

    inline VkCullModeFlagBits TransRHICullModeToVk(RHICullMode InRHICullMode)
    {
        switch (InRHICullMode)
        {
        case CULL_NONE:
            return VK_CULL_MODE_NONE;
        case CULL_FRONT:
            return VK_CULL_MODE_FRONT_BIT;
        case CULL_BACK:
            return VK_CULL_MODE_BACK_BIT;
        case CULL_ALL:
            return VK_CULL_MODE_FRONT_AND_BACK;
        default:
            return VK_CULL_MODE_BACK_BIT;
        }
    }

    inline VkSampleCountFlagBits TransRHIMultiSampleToVk(RHIMultiSampleCount InRHIMultiSampleCount)
    {
        switch (InRHIMultiSampleCount)
        {
        case SAMPLE_COUNT_1:
            return VK_SAMPLE_COUNT_1_BIT;
        case  SAMPLE_COUNT_2:
            return VK_SAMPLE_COUNT_2_BIT;
        case SAMPLE_COUNT_4:
            return VK_SAMPLE_COUNT_4_BIT;
        case SAMPLE_COUNT_8:
            return VK_SAMPLE_COUNT_8_BIT;
        case SAMPLE_COUNT_16:
            return VK_SAMPLE_COUNT_16_BIT;
        case SAMPLE_COUNT_32:
            return VK_SAMPLE_COUNT_32_BIT;
        case SAMPLE_COUNT_64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    inline VkCompareOp TransRHICompareOpToVk(RHICompareOp InRHICompareOp)
    {
        switch (InRHICompareOp)
        {
        case COMPARE_OP_NEVER:
            return VK_COMPARE_OP_NEVER;
        case COMPARE_OP_LESS:
            return VK_COMPARE_OP_LESS;
        case COMPARE_OP_EQUAL:
            return VK_COMPARE_OP_EQUAL;
        case COMPARE_OP_LESS_OR_EQUAL:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case COMPARE_OP_GREATER:
            return VK_COMPARE_OP_GREATER;
        case COMPARE_OP_NOT_EQUAL:
            return VK_COMPARE_OP_NOT_EQUAL;
        case COMPARE_OP_GREATER_OR_EQUAL:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case COMPARE_OP_ALWAYS:
            return VK_COMPARE_OP_ALWAYS;
        default:
            return VK_COMPARE_OP_LESS;
        }
    }

    inline VkFormat EngineToVkTextureFormat(PixelFormat InFormat, const bool bIsSRGB)
    {
        VkFormat Format = (VkFormat)GPixelFormats[InFormat].PlatformFormat;
        if (bIsSRGB)
        {
            switch (Format)
            {
            case VK_FORMAT_B8G8R8A8_UNORM:				Format = VK_FORMAT_B8G8R8A8_SRGB; break;
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:		Format = VK_FORMAT_A8B8G8R8_SRGB_PACK32; break;
            case VK_FORMAT_R8_UNORM:					Format = VK_FORMAT_R8_SRGB; break;
            case VK_FORMAT_R8G8_UNORM:					Format = VK_FORMAT_R8G8_SRGB; break;
            case VK_FORMAT_R8G8B8_UNORM:				Format = VK_FORMAT_R8G8B8_SRGB; break;
            case VK_FORMAT_R8G8B8A8_UNORM:				Format = VK_FORMAT_R8G8B8A8_SRGB; break;
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK:			Format = VK_FORMAT_BC1_RGB_SRGB_BLOCK; break;
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:		Format = VK_FORMAT_BC1_RGBA_SRGB_BLOCK; break;
            case VK_FORMAT_BC2_UNORM_BLOCK:				Format = VK_FORMAT_BC2_SRGB_BLOCK; break;
            case VK_FORMAT_BC3_UNORM_BLOCK:				Format = VK_FORMAT_BC3_SRGB_BLOCK; break;
            case VK_FORMAT_BC7_UNORM_BLOCK:				Format = VK_FORMAT_BC7_SRGB_BLOCK; break;
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:		Format = VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK; break;
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:	Format = VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK; break;
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:	Format = VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_4x4_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_5x4_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_5x5_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_6x5_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_6x6_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_8x5_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_8x6_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_8x8_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_10x5_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_10x6_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_10x8_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_10x10_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_12x10_SRGB_BLOCK; break;
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK:		Format = VK_FORMAT_ASTC_12x12_SRGB_BLOCK; break;
                //		case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG:	Format = VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG; break;
                //		case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG:	Format = VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG; break;
                //		case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG:	Format = VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG; break;
                //		case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG:	Format = VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG; break;
            default:	break;
            }
        }

        return Format;
    }

    inline VkFormat EngineToVkVertexFormat(VertexElementType InElementType)
    {
        switch (InElementType)
        {
        case Elaine::VET_None:
            return VK_FORMAT_UNDEFINED;
        case Elaine::VET_Float1:
            return VK_FORMAT_R8_SNORM;
        case Elaine::VET_Float2:
            return VK_FORMAT_R8G8_SNORM;
        case Elaine::VET_Float3:
            return VK_FORMAT_R8G8B8_SNORM;
        case Elaine::VET_Float4:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case Elaine::VET_PackedNormal:
            break;
        case Elaine::VET_UByte4:
            break;
        case Elaine::VET_UByte4N:
            break;
        case Elaine::VET_Color:
            break;
        case Elaine::VET_Short2:
            break;
        case Elaine::VET_Short4:
            break;
        case Elaine::VET_Short2N:
            break;
        case Elaine::VET_Half2:
            break;
        case Elaine::VET_Half4:
            break;
        case Elaine::VET_Short4N:
            break;
        case Elaine::VET_UShort2:
            break;
        case Elaine::VET_UShort4:
            break;
        case Elaine::VET_UShort2N:
            break;
        case Elaine::VET_UShort4N:
            break;
        case Elaine::VET_URGB10A2N:
            break;
        case Elaine::VET_UInt:
            break;
        case Elaine::VET_MAX:
            break;
        }
        return VK_FORMAT_UNDEFINED;
    }

    inline VkIndexType EngineToVkIndexType(RHIIndexType InType)
    {
        switch (InType)
        {
        case Elaine::INDEX_TYPE_UINT16:
            return VK_INDEX_TYPE_UINT16;
        case Elaine::INDEX_TYPE_UINT32:
            return VK_INDEX_TYPE_UINT32;
        case Elaine::INDEX_TYPE_NONE_KHR:
            return VK_INDEX_TYPE_NONE_KHR;
        case Elaine::INDEX_TYPE_UINT8_EXT:
            return VK_INDEX_TYPE_UINT8_EXT;
        default:
            break;
        }
        return VK_INDEX_TYPE_UINT16;
    }
}
