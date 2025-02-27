#pragma once
#include "ElaineMemory.h"

namespace Elaine
{

#define RHI_MAX_EXTENSION_NAME_SIZE        256U
#define RHI_MAX_DESCRIPTION_SIZE           256U
#define RHI_MAX_MEMORY_TYPES               32U
#define RHI_MAX_PHYSICAL_DEVICE_NAME_SIZE  256U
#define RHI_UUID_SIZE                      16U
#define RHI_MAX_MEMORY_HEAPS               16U
#define RHI_SUBPASS_EXTERNAL               (~0U)
#define RHI_QUEUE_FAMILY_IGNORED           (~0U)
#define RHI_WHOLE_SIZE                     (~0ULL)
#define RHI_NULL_HANDLE                    nullptr
#define RHI_SUCCESS                        true
#define RHI_TRUE                           true
#define RHI_FALSE                          false

    /** An enumeration of the different RHI reference types. */
    enum RHIResourceType : uint8
    {
        RRT_None,

        RRT_SamplerState,
        RRT_RasterizerState,
        RRT_DepthStencilState,
        RRT_BlendState,
        RRT_VertexDeclaration,
        RRT_VertexShader,
        RRT_MeshShader,
        RRT_AmplificationShader,
        RRT_PixelShader,
        RRT_GeometryShader,
        RRT_RayTracingShader,
        RRT_ComputeShader,
        RRT_GraphicsPipelineState,
        RRT_ComputePipelineState,
        RRT_RayTracingPipelineState,
        RRT_BoundShaderState,
        RRT_UniformBufferLayout,
        RRT_UniformBuffer,
        RRT_Buffer,
        RRT_Texture,
        // @todo: texture type unification - remove these
        RRT_Texture2D,
        RRT_Texture2DArray,
        RRT_Texture3D,
        RRT_TextureCube,
        // @todo: texture type unification - remove these
        RRT_TextureReference,
        RRT_TimestampCalibrationQuery,
        RRT_GPUFence,
        RRT_RenderQuery,
        RRT_RenderQueryPool,
        RRT_ComputeFence,
        RRT_Viewport,
        RRT_UnorderedAccessView,
        RRT_ShaderResourceView,
        RRT_RayTracingAccelerationStructure,
        RRT_StagingBuffer,
        RRT_CustomPresent,
        RRT_ShaderLibrary,
        RRT_PipelineBinaryLibrary,
        RRT_Pipeline,
        RRT_Num
    };

    enum PixelFormat : uint8
    {
        PF_Unknown = 0,
        PF_A32B32G32R32F = 1,
        PF_B8G8R8A8 = 2,
        PF_G8 = 3, // G8  means Gray/Grey , not Green , typically actually uses a red format with replication of R to RGB
        PF_G16 = 4, // G16 means Gray/Grey like G8
        PF_DXT1 = 5,
        PF_DXT3 = 6,
        PF_DXT5 = 7,
        PF_UYVY = 8,
        PF_FloatRGB = 9, // 16F
        PF_FloatRGBA = 10, // 16F
        PF_DepthStencil = 11,
        PF_ShadowDepth = 12,
        PF_R32_FLOAT = 13,
        PF_G16R16 = 14,
        PF_G16R16F = 15,
        PF_G16R16F_FILTER = 16,
        PF_G32R32F = 17,
        PF_A2B10G10R10 = 18,
        PF_A16B16G16R16 = 19,
        PF_D24 = 20,
        PF_R16F = 21,
        PF_R16F_FILTER = 22,
        PF_BC5 = 23,
        PF_V8U8 = 24,
        PF_A1 = 25,
        PF_FloatR11G11B10 = 26,
        PF_A8 = 27,
        PF_R32_UINT = 28,
        PF_R32_SINT = 29,
        PF_PVRTC2 = 30,
        PF_PVRTC4 = 31,
        PF_R16_UINT = 32,
        PF_R16_SINT = 33,
        PF_R16G16B16A16_UINT = 34,
        PF_R16G16B16A16_SINT = 35,
        PF_R5G6B5_UNORM = 36,
        PF_R8G8B8A8 = 37,
        PF_A8R8G8B8 = 38,	// Only used for legacy loading; do NOT use!
        PF_BC4 = 39,
        PF_R8G8 = 40,
        PF_ATC_RGB = 41,	// Unsupported Format
        PF_ATC_RGBA_E = 42,	// Unsupported Format
        PF_ATC_RGBA_I = 43,	// Unsupported Format
        PF_X24_G8 = 44,	// Used for creating SRVs to alias a DepthStencil buffer to read Stencil. Don't use for creating textures.
        PF_ETC1 = 45,	// Unsupported Format
        PF_ETC2_RGB = 46,
        PF_ETC2_RGBA = 47,
        PF_R32G32B32A32_UINT = 48,
        PF_R16G16_UINT = 49,
        PF_ASTC_4x4 = 50,	// 8.00 bpp
        PF_ASTC_6x6 = 51,	// 3.56 bpp
        PF_ASTC_8x8 = 52,	// 2.00 bpp
        PF_ASTC_10x10 = 53,	// 1.28 bpp
        PF_ASTC_12x12 = 54,	// 0.89 bpp
        PF_BC6H = 55,
        PF_BC7 = 56,
        PF_R8_UINT = 57,
        PF_L8 = 58,
        PF_XGXR8 = 59,
        PF_R8G8B8A8_UINT = 60,
        PF_R8G8B8A8_SNORM = 61,
        PF_R16G16B16A16_UNORM = 62,
        PF_R16G16B16A16_SNORM = 63,
        PF_PLATFORM_HDR_0 = 64,
        PF_PLATFORM_HDR_1 = 65,	// Reserved.
        PF_PLATFORM_HDR_2 = 66,	// Reserved.
        PF_NV12 = 67,
        PF_R32G32_UINT = 68,
        PF_ETC2_R11_EAC = 69,
        PF_ETC2_RG11_EAC = 70,
        PF_R8 = 71,
        PF_B5G5R5A1_UNORM = 72,
        PF_ASTC_4x4_HDR = 73,
        PF_ASTC_6x6_HDR = 74,
        PF_ASTC_8x8_HDR = 75,
        PF_ASTC_10x10_HDR = 76,
        PF_ASTC_12x12_HDR = 77,
        PF_G16R16_SNORM = 78,
        PF_R8G8_UINT = 79,
        PF_R32G32B32_UINT = 80,
        PF_R32G32B32_SINT = 81,
        PF_R32G32B32F = 82,
        PF_R8_SINT = 83,
        PF_R64_UINT = 84,
        PF_R9G9B9EXP5 = 85,
        PF_MAX_ = 86,
    };

    enum class PixelFormatChannelFlags : uint8
    {
        R = 1 << 0,
        G = 1 << 1,
        B = 1 << 2,
        A = 1 << 3,
        RG = R | G,
        RGB = R | G | B,
        RGBA = R | G | B | A,

        None = 0,
    };

    inline bool IsHDR(PixelFormat PixelFormat)
    {
        return PixelFormat == PF_FloatRGBA || PixelFormat == PF_BC6H || PixelFormat == PF_R16F || PixelFormat == PF_R32_FLOAT || PixelFormat == PF_A32B32G32R32F;
    }

    enum class TextureCreateFlags : uint64
    {
        None = 0,

        // Texture can be used as a render target
        RenderTargetable = 1ull << 0,
        // Texture can be used as a resolve target
        ResolveTargetable = 1ull << 1,
        // Texture can be used as a depth-stencil target.
        DepthStencilTargetable = 1ull << 2,
        // Texture can be used as a shader resource.
        ShaderResource = 1ull << 3,
        // Texture is encoded in sRGB gamma space
        SRGB = 1ull << 4,
        // Texture data is writable by the CPU
        CPUWritable = 1ull << 5,
        // Texture will be created with an un-tiled format
        NoTiling = 1ull << 6,
        // Texture will be used for video decode
        VideoDecode = 1ull << 7,
        // Texture that may be updated every frame
        Dynamic = 1ull << 8,
        // Texture will be used as a render pass attachment that will be read from
        InputAttachmentRead = 1ull << 9,
        /** Texture represents a foveation attachment */
        Foveation = 1ull << 10,
        // Prefer 3D internal surface tiling mode for volume textures when possible
        Tiling3D = 1ull << 11,
        // This texture has no GPU or CPU backing. It only exists in tile memory on TBDR GPUs (i.e., mobile).
        Memoryless = 1ull << 12,
        // Create the texture with the flag that allows mip generation later, only applicable to D3D11
        GenerateMipCapable = 1ull << 13,
        // The texture can be partially allocated in fastvram
        FastVRAMPartialAlloc = 1ull << 14,
        // Do not create associated shader resource view, only applicable to D3D11 and D3D12
        DisableSRVCreation = 1ull << 15,
        // Do not allow Delta Color Compression (DCC) to be used with this texture
        DisableDCC = 1ull << 16,
        // UnorderedAccessView (DX11 only)
        // Warning: Causes additional synchronization between draw calls when using a render target allocated with this flag, use sparingly
        // See: GCNPerformanceTweets.pdf Tip 37
        UAV = 1ull << 17,
        // Render target texture that will be displayed on screen (back buffer)
        Presentable = 1ull << 18,
        // Texture data is accessible by the CPU
        CPUReadback = 1ull << 19,
        // Texture was processed offline (via a texture conversion process for the current platform)
        OfflineProcessed = 1ull << 20,
        // Texture needs to go in fast VRAM if available (HINT only)
        FastVRAM = 1ull << 21,
        // by default the texture is not showing up in the list - this is to reduce clutter, using the FULL option this can be ignored
        HideInVisualizeTexture = 1ull << 22,
        // Texture should be created in virtual memory, with no physical memory allocation made
        // You must make further calls to RHIVirtualTextureSetFirstMipInMemory to allocate physical memory
        // and RHIVirtualTextureSetFirstMipVisible to map the first mip visible to the GPU
        Virtual = 1ull << 23,
        // Creates a RenderTargetView for each array slice of the texture
        // Warning: if this was specified when the resource was created, you can't use SV_RenderTargetArrayIndex to route to other slices!
        TargetArraySlicesIndependently = 1ull << 24,
        // Texture that may be shared with DX9 or other devices
        Shared = 1ull << 25,
        // RenderTarget will not use full-texture fast clear functionality.
        NoFastClear = 1ull << 26,
        // Texture is a depth stencil resolve target
        DepthStencilResolveTarget = 1ull << 27,
        // Flag used to indicted this texture is a streamable 2D texture, and should be counted towards the texture streaming pool budget.
        Streamable = 1ull << 28,
        // Render target will not FinalizeFastClear; Caches and meta data will be flushed, but clearing will be skipped (avoids potentially trashing metadata)
        NoFastClearFinalize = 1ull << 29,
        // Hint to the driver that this resource is managed properly by the engine for Alternate-Frame-Rendering in mGPU usage.
        AFRManual = 1ull << 30,
        // Workaround for 128^3 volume textures getting bloated 4x due to tiling mode on some platforms.
        ReduceMemoryWithTilingMode = 1ull << 31,
        /** Texture should be allocated from transient memory. */
        Transient = None,
        /** Texture needs to support atomic operations */
        AtomicCompatible = 1ull << 33,
        /** Texture should be allocated for external access. Vulkan only */
        External = 1ull << 34,
        /** Don't automatically transfer across GPUs in multi-GPU scenarios.  For example, if you are transferring it yourself manually. */
        MultiGPUGraphIgnore = 1ull << 35,
        /** Texture needs to support atomic operations */
        Atomic64Compatible = 1ull << 36,
    };


    enum RHIFormat : int
    {
        RHI_FORMAT_UNDEFINED = 0,
        RHI_FORMAT_R4G4_UNORM_PACK8 = 1,
        RHI_FORMAT_R4G4B4A4_UNORM_PACK16 = 2,
        RHI_FORMAT_B4G4R4A4_UNORM_PACK16 = 3,
        RHI_FORMAT_R5G6B5_UNORM_PACK16 = 4,
        RHI_FORMAT_B5G6R5_UNORM_PACK16 = 5,
        RHI_FORMAT_R5G5B5A1_UNORM_PACK16 = 6,
        RHI_FORMAT_B5G5R5A1_UNORM_PACK16 = 7,
        RHI_FORMAT_A1R5G5B5_UNORM_PACK16 = 8,
        RHI_FORMAT_R8_UNORM = 9,
        RHI_FORMAT_R8_SNORM = 10,
        RHI_FORMAT_R8_USCALED = 11,
        RHI_FORMAT_R8_SSCALED = 12,
        RHI_FORMAT_R8_UINT = 13,
        RHI_FORMAT_R8_SINT = 14,
        RHI_FORMAT_R8_SRGB = 15,
        RHI_FORMAT_R8G8_UNORM = 16,
        RHI_FORMAT_R8G8_SNORM = 17,
        RHI_FORMAT_R8G8_USCALED = 18,
        RHI_FORMAT_R8G8_SSCALED = 19,
        RHI_FORMAT_R8G8_UINT = 20,
        RHI_FORMAT_R8G8_SINT = 21,
        RHI_FORMAT_R8G8_SRGB = 22,
        RHI_FORMAT_R8G8B8_UNORM = 23,
        RHI_FORMAT_R8G8B8_SNORM = 24,
        RHI_FORMAT_R8G8B8_USCALED = 25,
        RHI_FORMAT_R8G8B8_SSCALED = 26,
        RHI_FORMAT_R8G8B8_UINT = 27,
        RHI_FORMAT_R8G8B8_SINT = 28,
        RHI_FORMAT_R8G8B8_SRGB = 29,
        RHI_FORMAT_B8G8R8_UNORM = 30,
        RHI_FORMAT_B8G8R8_SNORM = 31,
        RHI_FORMAT_B8G8R8_USCALED = 32,
        RHI_FORMAT_B8G8R8_SSCALED = 33,
        RHI_FORMAT_B8G8R8_UINT = 34,
        RHI_FORMAT_B8G8R8_SINT = 35,
        RHI_FORMAT_B8G8R8_SRGB = 36,
        RHI_FORMAT_R8G8B8A8_UNORM = 37,
        RHI_FORMAT_R8G8B8A8_SNORM = 38,
        RHI_FORMAT_R8G8B8A8_USCALED = 39,
        RHI_FORMAT_R8G8B8A8_SSCALED = 40,
        RHI_FORMAT_R8G8B8A8_UINT = 41,
        RHI_FORMAT_R8G8B8A8_SINT = 42,
        RHI_FORMAT_R8G8B8A8_SRGB = 43,
        RHI_FORMAT_B8G8R8A8_UNORM = 44,
        RHI_FORMAT_B8G8R8A8_SNORM = 45,
        RHI_FORMAT_B8G8R8A8_USCALED = 46,
        RHI_FORMAT_B8G8R8A8_SSCALED = 47,
        RHI_FORMAT_B8G8R8A8_UINT = 48,
        RHI_FORMAT_B8G8R8A8_SINT = 49,
        RHI_FORMAT_B8G8R8A8_SRGB = 50,
        RHI_FORMAT_A8B8G8R8_UNORM_PACK32 = 51,
        RHI_FORMAT_A8B8G8R8_SNORM_PACK32 = 52,
        RHI_FORMAT_A8B8G8R8_USCALED_PACK32 = 53,
        RHI_FORMAT_A8B8G8R8_SSCALED_PACK32 = 54,
        RHI_FORMAT_A8B8G8R8_UINT_PACK32 = 55,
        RHI_FORMAT_A8B8G8R8_SINT_PACK32 = 56,
        RHI_FORMAT_A8B8G8R8_SRGB_PACK32 = 57,
        RHI_FORMAT_A2R10G10B10_UNORM_PACK32 = 58,
        RHI_FORMAT_A2R10G10B10_SNORM_PACK32 = 59,
        RHI_FORMAT_A2R10G10B10_USCALED_PACK32 = 60,
        RHI_FORMAT_A2R10G10B10_SSCALED_PACK32 = 61,
        RHI_FORMAT_A2R10G10B10_UINT_PACK32 = 62,
        RHI_FORMAT_A2R10G10B10_SINT_PACK32 = 63,
        RHI_FORMAT_A2B10G10R10_UNORM_PACK32 = 64,
        RHI_FORMAT_A2B10G10R10_SNORM_PACK32 = 65,
        RHI_FORMAT_A2B10G10R10_USCALED_PACK32 = 66,
        RHI_FORMAT_A2B10G10R10_SSCALED_PACK32 = 67,
        RHI_FORMAT_A2B10G10R10_UINT_PACK32 = 68,
        RHI_FORMAT_A2B10G10R10_SINT_PACK32 = 69,
        RHI_FORMAT_R16_UNORM = 70,
        RHI_FORMAT_R16_SNORM = 71,
        RHI_FORMAT_R16_USCALED = 72,
        RHI_FORMAT_R16_SSCALED = 73,
        RHI_FORMAT_R16_UINT = 74,
        RHI_FORMAT_R16_SINT = 75,
        RHI_FORMAT_R16_SFLOAT = 76,
        RHI_FORMAT_R16G16_UNORM = 77,
        RHI_FORMAT_R16G16_SNORM = 78,
        RHI_FORMAT_R16G16_USCALED = 79,
        RHI_FORMAT_R16G16_SSCALED = 80,
        RHI_FORMAT_R16G16_UINT = 81,
        RHI_FORMAT_R16G16_SINT = 82,
        RHI_FORMAT_R16G16_SFLOAT = 83,
        RHI_FORMAT_R16G16B16_UNORM = 84,
        RHI_FORMAT_R16G16B16_SNORM = 85,
        RHI_FORMAT_R16G16B16_USCALED = 86,
        RHI_FORMAT_R16G16B16_SSCALED = 87,
        RHI_FORMAT_R16G16B16_UINT = 88,
        RHI_FORMAT_R16G16B16_SINT = 89,
        RHI_FORMAT_R16G16B16_SFLOAT = 90,
        RHI_FORMAT_R16G16B16A16_UNORM = 91,
        RHI_FORMAT_R16G16B16A16_SNORM = 92,
        RHI_FORMAT_R16G16B16A16_USCALED = 93,
        RHI_FORMAT_R16G16B16A16_SSCALED = 94,
        RHI_FORMAT_R16G16B16A16_UINT = 95,
        RHI_FORMAT_R16G16B16A16_SINT = 96,
        RHI_FORMAT_R16G16B16A16_SFLOAT = 97,
        RHI_FORMAT_R32_UINT = 98,
        RHI_FORMAT_R32_SINT = 99,
        RHI_FORMAT_R32_SFLOAT = 100,
        RHI_FORMAT_R32G32_UINT = 101,
        RHI_FORMAT_R32G32_SINT = 102,
        RHI_FORMAT_R32G32_SFLOAT = 103,
        RHI_FORMAT_R32G32B32_UINT = 104,
        RHI_FORMAT_R32G32B32_SINT = 105,
        RHI_FORMAT_R32G32B32_SFLOAT = 106,
        RHI_FORMAT_R32G32B32A32_UINT = 107,
        RHI_FORMAT_R32G32B32A32_SINT = 108,
        RHI_FORMAT_R32G32B32A32_SFLOAT = 109,
        RHI_FORMAT_R64_UINT = 110,
        RHI_FORMAT_R64_SINT = 111,
        RHI_FORMAT_R64_SFLOAT = 112,
        RHI_FORMAT_R64G64_UINT = 113,
        RHI_FORMAT_R64G64_SINT = 114,
        RHI_FORMAT_R64G64_SFLOAT = 115,
        RHI_FORMAT_R64G64B64_UINT = 116,
        RHI_FORMAT_R64G64B64_SINT = 117,
        RHI_FORMAT_R64G64B64_SFLOAT = 118,
        RHI_FORMAT_R64G64B64A64_UINT = 119,
        RHI_FORMAT_R64G64B64A64_SINT = 120,
        RHI_FORMAT_R64G64B64A64_SFLOAT = 121,
        RHI_FORMAT_B10G11R11_UFLOAT_PACK32 = 122,
        RHI_FORMAT_E5B9G9R9_UFLOAT_PACK32 = 123,
        RHI_FORMAT_D16_UNORM = 124,
        RHI_FORMAT_X8_D24_UNORM_PACK32 = 125,
        RHI_FORMAT_D32_SFLOAT = 126,
        RHI_FORMAT_S8_UINT = 127,
        RHI_FORMAT_D16_UNORM_S8_UINT = 128,
        RHI_FORMAT_D24_UNORM_S8_UINT = 129,
        RHI_FORMAT_D32_SFLOAT_S8_UINT = 130,
        RHI_FORMAT_BC1_RGB_UNORM_BLOCK = 131,
        RHI_FORMAT_BC1_RGB_SRGB_BLOCK = 132,
        RHI_FORMAT_BC1_RGBA_UNORM_BLOCK = 133,
        RHI_FORMAT_BC1_RGBA_SRGB_BLOCK = 134,
        RHI_FORMAT_BC2_UNORM_BLOCK = 135,
        RHI_FORMAT_BC2_SRGB_BLOCK = 136,
        RHI_FORMAT_BC3_UNORM_BLOCK = 137,
        RHI_FORMAT_BC3_SRGB_BLOCK = 138,
        RHI_FORMAT_BC4_UNORM_BLOCK = 139,
        RHI_FORMAT_BC4_SNORM_BLOCK = 140,
        RHI_FORMAT_BC5_UNORM_BLOCK = 141,
        RHI_FORMAT_BC5_SNORM_BLOCK = 142,
        RHI_FORMAT_BC6H_UFLOAT_BLOCK = 143,
        RHI_FORMAT_BC6H_SFLOAT_BLOCK = 144,
        RHI_FORMAT_BC7_UNORM_BLOCK = 145,
        RHI_FORMAT_BC7_SRGB_BLOCK = 146,
        RHI_FORMAT_ETC2_R8G8B8_UNORM_BLOCK = 147,
        RHI_FORMAT_ETC2_R8G8B8_SRGB_BLOCK = 148,
        RHI_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK = 149,
        RHI_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK = 150,
        RHI_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK = 151,
        RHI_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK = 152,
        RHI_FORMAT_EAC_R11_UNORM_BLOCK = 153,
        RHI_FORMAT_EAC_R11_SNORM_BLOCK = 154,
        RHI_FORMAT_EAC_R11G11_UNORM_BLOCK = 155,
        RHI_FORMAT_EAC_R11G11_SNORM_BLOCK = 156,
        RHI_FORMAT_ASTC_4x4_UNORM_BLOCK = 157,
        RHI_FORMAT_ASTC_4x4_SRGB_BLOCK = 158,
        RHI_FORMAT_ASTC_5x4_UNORM_BLOCK = 159,
        RHI_FORMAT_ASTC_5x4_SRGB_BLOCK = 160,
        RHI_FORMAT_ASTC_5x5_UNORM_BLOCK = 161,
        RHI_FORMAT_ASTC_5x5_SRGB_BLOCK = 162,
        RHI_FORMAT_ASTC_6x5_UNORM_BLOCK = 163,
        RHI_FORMAT_ASTC_6x5_SRGB_BLOCK = 164,
        RHI_FORMAT_ASTC_6x6_UNORM_BLOCK = 165,
        RHI_FORMAT_ASTC_6x6_SRGB_BLOCK = 166,
        RHI_FORMAT_ASTC_8x5_UNORM_BLOCK = 167,
        RHI_FORMAT_ASTC_8x5_SRGB_BLOCK = 168,
        RHI_FORMAT_ASTC_8x6_UNORM_BLOCK = 169,
        RHI_FORMAT_ASTC_8x6_SRGB_BLOCK = 170,
        RHI_FORMAT_ASTC_8x8_UNORM_BLOCK = 171,
        RHI_FORMAT_ASTC_8x8_SRGB_BLOCK = 172,
        RHI_FORMAT_ASTC_10x5_UNORM_BLOCK = 173,
        RHI_FORMAT_ASTC_10x5_SRGB_BLOCK = 174,
        RHI_FORMAT_ASTC_10x6_UNORM_BLOCK = 175,
        RHI_FORMAT_ASTC_10x6_SRGB_BLOCK = 176,
        RHI_FORMAT_ASTC_10x8_UNORM_BLOCK = 177,
        RHI_FORMAT_ASTC_10x8_SRGB_BLOCK = 178,
        RHI_FORMAT_ASTC_10x10_UNORM_BLOCK = 179,
        RHI_FORMAT_ASTC_10x10_SRGB_BLOCK = 180,
        RHI_FORMAT_ASTC_12x10_UNORM_BLOCK = 181,
        RHI_FORMAT_ASTC_12x10_SRGB_BLOCK = 182,
        RHI_FORMAT_ASTC_12x12_UNORM_BLOCK = 183,
        RHI_FORMAT_ASTC_12x12_SRGB_BLOCK = 184,
        RHI_FORMAT_G8B8G8R8_422_UNORM = 1000156000,
        RHI_FORMAT_B8G8R8G8_422_UNORM = 1000156001,
        RHI_FORMAT_G8_B8_R8_3PLANE_420_UNORM = 1000156002,
        RHI_FORMAT_G8_B8R8_2PLANE_420_UNORM = 1000156003,
        RHI_FORMAT_G8_B8_R8_3PLANE_422_UNORM = 1000156004,
        RHI_FORMAT_G8_B8R8_2PLANE_422_UNORM = 1000156005,
        RHI_FORMAT_G8_B8_R8_3PLANE_444_UNORM = 1000156006,
        RHI_FORMAT_R10X6_UNORM_PACK16 = 1000156007,
        RHI_FORMAT_R10X6G10X6_UNORM_2PACK16 = 1000156008,
        RHI_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16 = 1000156009,
        RHI_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16 = 1000156010,
        RHI_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16 = 1000156011,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16 = 1000156012,
        RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16 = 1000156013,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16 = 1000156014,
        RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16 = 1000156015,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16 = 1000156016,
        RHI_FORMAT_R12X4_UNORM_PACK16 = 1000156017,
        RHI_FORMAT_R12X4G12X4_UNORM_2PACK16 = 1000156018,
        RHI_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16 = 1000156019,
        RHI_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16 = 1000156020,
        RHI_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16 = 1000156021,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16 = 1000156022,
        RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16 = 1000156023,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16 = 1000156024,
        RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16 = 1000156025,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16 = 1000156026,
        RHI_FORMAT_G16B16G16R16_422_UNORM = 1000156027,
        RHI_FORMAT_B16G16R16G16_422_UNORM = 1000156028,
        RHI_FORMAT_G16_B16_R16_3PLANE_420_UNORM = 1000156029,
        RHI_FORMAT_G16_B16R16_2PLANE_420_UNORM = 1000156030,
        RHI_FORMAT_G16_B16_R16_3PLANE_422_UNORM = 1000156031,
        RHI_FORMAT_G16_B16R16_2PLANE_422_UNORM = 1000156032,
        RHI_FORMAT_G16_B16_R16_3PLANE_444_UNORM = 1000156033,
        RHI_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG = 1000054000,
        RHI_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG = 1000054001,
        RHI_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG = 1000054002,
        RHI_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG = 1000054003,
        RHI_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG = 1000054004,
        RHI_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG = 1000054005,
        RHI_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG = 1000054006,
        RHI_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG = 1000054007,
        RHI_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT = 1000066000,
        RHI_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT = 1000066001,
        RHI_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT = 1000066002,
        RHI_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT = 1000066003,
        RHI_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT = 1000066004,
        RHI_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT = 1000066005,
        RHI_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT = 1000066006,
        RHI_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT = 1000066007,
        RHI_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT = 1000066008,
        RHI_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT = 1000066009,
        RHI_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT = 1000066010,
        RHI_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT = 1000066011,
        RHI_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT = 1000066012,
        RHI_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT = 1000066013,
        RHI_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT = 1000330000,
        RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT = 1000330001,
        RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT = 1000330002,
        RHI_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT = 1000330003,
        RHI_FORMAT_A4R4G4B4_UNORM_PACK16_EXT = 1000340000,
        RHI_FORMAT_A4B4G4R4_UNORM_PACK16_EXT = 1000340001,
        RHI_FORMAT_G8B8G8R8_422_UNORM_KHR = RHI_FORMAT_G8B8G8R8_422_UNORM,
        RHI_FORMAT_B8G8R8G8_422_UNORM_KHR = RHI_FORMAT_B8G8R8G8_422_UNORM,
        RHI_FORMAT_G8_B8_R8_3PLANE_420_UNORM_KHR = RHI_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        RHI_FORMAT_G8_B8R8_2PLANE_420_UNORM_KHR = RHI_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        RHI_FORMAT_G8_B8_R8_3PLANE_422_UNORM_KHR = RHI_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        RHI_FORMAT_G8_B8R8_2PLANE_422_UNORM_KHR = RHI_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        RHI_FORMAT_G8_B8_R8_3PLANE_444_UNORM_KHR = RHI_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        RHI_FORMAT_R10X6_UNORM_PACK16_KHR = RHI_FORMAT_R10X6_UNORM_PACK16,
        RHI_FORMAT_R10X6G10X6_UNORM_2PACK16_KHR = RHI_FORMAT_R10X6G10X6_UNORM_2PACK16,
        RHI_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16_KHR = RHI_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        RHI_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16_KHR = RHI_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        RHI_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16_KHR = RHI_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_KHR = RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_KHR = RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16_KHR = RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16_KHR = RHI_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16_KHR = RHI_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        RHI_FORMAT_R12X4_UNORM_PACK16_KHR = RHI_FORMAT_R12X4_UNORM_PACK16,
        RHI_FORMAT_R12X4G12X4_UNORM_2PACK16_KHR = RHI_FORMAT_R12X4G12X4_UNORM_2PACK16,
        RHI_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16_KHR = RHI_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        RHI_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16_KHR = RHI_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        RHI_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16_KHR = RHI_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_KHR = RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_KHR = RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16_KHR = RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16_KHR = RHI_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16_KHR = RHI_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        RHI_FORMAT_G16B16G16R16_422_UNORM_KHR = RHI_FORMAT_G16B16G16R16_422_UNORM,
        RHI_FORMAT_B16G16R16G16_422_UNORM_KHR = RHI_FORMAT_B16G16R16G16_422_UNORM,
        RHI_FORMAT_G16_B16_R16_3PLANE_420_UNORM_KHR = RHI_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        RHI_FORMAT_G16_B16R16_2PLANE_420_UNORM_KHR = RHI_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        RHI_FORMAT_G16_B16_R16_3PLANE_422_UNORM_KHR = RHI_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        RHI_FORMAT_G16_B16R16_2PLANE_422_UNORM_KHR = RHI_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        RHI_FORMAT_G16_B16_R16_3PLANE_444_UNORM_KHR = RHI_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
        RHI_FORMAT_MAX_ENUM = 0x7FFFFFFF
    };

    enum RHIDynamicState : int
    {
        RHI_DYNAMIC_STATE_VIEWPORT = 0,
        RHI_DYNAMIC_STATE_SCISSOR = 1,
        RHI_DYNAMIC_STATE_LINE_WIDTH = 2,
        RHI_DYNAMIC_STATE_DEPTH_BIAS = 3,
        RHI_DYNAMIC_STATE_BLEND_CONSTANTS = 4,
        RHI_DYNAMIC_STATE_DEPTH_BOUNDS = 5,
        RHI_DYNAMIC_STATE_STENCIL_COMPARE_MASK = 6,
        RHI_DYNAMIC_STATE_STENCIL_WRITE_MASK = 7,
        RHI_DYNAMIC_STATE_STENCIL_REFERENCE = 8,
        RHI_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV = 1000087000,
        RHI_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT = 1000099000,
        RHI_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT = 1000143000,
        RHI_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR = 1000347000,
        RHI_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV = 1000164004,
        RHI_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV = 1000164006,
        RHI_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV = 1000205001,
        RHI_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR = 1000226000,
        RHI_DYNAMIC_STATE_LINE_STIPPLE_EXT = 1000259000,
        RHI_DYNAMIC_STATE_CULL_MODE_EXT = 1000267000,
        RHI_DYNAMIC_STATE_FRONT_FACE_EXT = 1000267001,
        RHI_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT = 1000267002,
        RHI_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT = 1000267003,
        RHI_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT = 1000267004,
        RHI_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT = 1000267005,
        RHI_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT = 1000267006,
        RHI_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT = 1000267007,
        RHI_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT = 1000267008,
        RHI_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT = 1000267009,
        RHI_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT = 1000267010,
        RHI_DYNAMIC_STATE_STENCIL_OP_EXT = 1000267011,
        RHI_DYNAMIC_STATE_VERTEX_INPUT_EXT = 1000352000,
        RHI_DYNAMIC_STATE_PATCH_CONTROL_POINTS_EXT = 1000377000,
        RHI_DYNAMIC_STATE_RASTERIZER_DISCARD_ENABLE_EXT = 1000377001,
        RHI_DYNAMIC_STATE_DEPTH_BIAS_ENABLE_EXT = 1000377002,
        RHI_DYNAMIC_STATE_LOGIC_OP_EXT = 1000377003,
        RHI_DYNAMIC_STATE_PRIMITIVE_RESTART_ENABLE_EXT = 1000377004,
        RHI_DYNAMIC_STATE_COLOR_WRITE_ENABLE_EXT = 1000381000,
        RHI_DYNAMIC_STATE_MAX_ENUM = 0x7FFFFFFF
    };

    enum RHIBlendOp : int
    {
        RHI_BLEND_OP_ADD = 0,
        RHI_BLEND_OP_SUBTRACT = 1,
        RHI_BLEND_OP_REVERSE_SUBTRACT = 2,
        RHI_BLEND_OP_MIN = 3,
        RHI_BLEND_OP_MAX = 4,
        RHI_BLEND_OP_ZERO_EXT = 1000148000,
        RHI_BLEND_OP_SRC_EXT = 1000148001,
        RHI_BLEND_OP_DST_EXT = 1000148002,
        RHI_BLEND_OP_SRC_OVER_EXT = 1000148003,
        RHI_BLEND_OP_DST_OVER_EXT = 1000148004,
        RHI_BLEND_OP_SRC_IN_EXT = 1000148005,
        RHI_BLEND_OP_DST_IN_EXT = 1000148006,
        RHI_BLEND_OP_SRC_OUT_EXT = 1000148007,
        RHI_BLEND_OP_DST_OUT_EXT = 1000148008,
        RHI_BLEND_OP_SRC_ATOP_EXT = 1000148009,
        RHI_BLEND_OP_DST_ATOP_EXT = 1000148010,
        RHI_BLEND_OP_XOR_EXT = 1000148011,
        RHI_BLEND_OP_MULTIPLY_EXT = 1000148012,
        RHI_BLEND_OP_SCREEN_EXT = 1000148013,
        RHI_BLEND_OP_OVERLAY_EXT = 1000148014,
        RHI_BLEND_OP_DARKEN_EXT = 1000148015,
        RHI_BLEND_OP_LIGHTEN_EXT = 1000148016,
        RHI_BLEND_OP_COLORDODGE_EXT = 1000148017,
        RHI_BLEND_OP_COLORBURN_EXT = 1000148018,
        RHI_BLEND_OP_HARDLIGHT_EXT = 1000148019,
        RHI_BLEND_OP_SOFTLIGHT_EXT = 1000148020,
        RHI_BLEND_OP_DIFFERENCE_EXT = 1000148021,
        RHI_BLEND_OP_EXCLUSION_EXT = 1000148022,
        RHI_BLEND_OP_INVERT_EXT = 1000148023,
        RHI_BLEND_OP_INVERT_RGB_EXT = 1000148024,
        RHI_BLEND_OP_LINEARDODGE_EXT = 1000148025,
        RHI_BLEND_OP_LINEARBURN_EXT = 1000148026,
        RHI_BLEND_OP_VIVIDLIGHT_EXT = 1000148027,
        RHI_BLEND_OP_LINEARLIGHT_EXT = 1000148028,
        RHI_BLEND_OP_PINLIGHT_EXT = 1000148029,
        RHI_BLEND_OP_HARDMIX_EXT = 1000148030,
        RHI_BLEND_OP_HSL_HUE_EXT = 1000148031,
        RHI_BLEND_OP_HSL_SATURATION_EXT = 1000148032,
        RHI_BLEND_OP_HSL_COLOR_EXT = 1000148033,
        RHI_BLEND_OP_HSL_LUMINOSITY_EXT = 1000148034,
        RHI_BLEND_OP_PLUS_EXT = 1000148035,
        RHI_BLEND_OP_PLUS_CLAMPED_EXT = 1000148036,
        RHI_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT = 1000148037,
        RHI_BLEND_OP_PLUS_DARKER_EXT = 1000148038,
        RHI_BLEND_OP_MINUS_EXT = 1000148039,
        RHI_BLEND_OP_MINUS_CLAMPED_EXT = 1000148040,
        RHI_BLEND_OP_CONTRAST_EXT = 1000148041,
        RHI_BLEND_OP_INVERT_OVG_EXT = 1000148042,
        RHI_BLEND_OP_RED_EXT = 1000148043,
        RHI_BLEND_OP_GREEN_EXT = 1000148044,
        RHI_BLEND_OP_BLUE_EXT = 1000148045,
        RHI_BLEND_OP_MAX_ENUM = 0x7FFFFFFF
    };


    enum RHIBlendFactor
    {
        RHI_BLEND_FACTOR_ZERO = 0,
        RHI_BLEND_FACTOR_ONE = 1,
        RHI_BLEND_FACTOR_SRC_COLOR = 2,
        RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
        RHI_BLEND_FACTOR_DST_COLOR = 4,
        RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
        RHI_BLEND_FACTOR_SRC_ALPHA = 6,
        RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
        RHI_BLEND_FACTOR_DST_ALPHA = 8,
        RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
        RHI_BLEND_FACTOR_CONSTANT_COLOR = 10,
        RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
        RHI_BLEND_FACTOR_CONSTANT_ALPHA = 12,
        RHI_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA = 13,
        RHI_BLEND_FACTOR_SRC_ALPHA_SATURATE = 14,
        RHI_BLEND_FACTOR_SRC1_COLOR = 15,
        RHI_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR = 16,
        RHI_BLEND_FACTOR_SRC1_ALPHA = 17,
        RHI_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA = 18,
        RHI_BLEND_FACTOR_MAX_ENUM = 0x7FFFFFFF
    };


    enum RHIColorComponentFlagBits
    {
        RHI_COLOR_COMPONENT_R_BIT = 0x00000001,
        RHI_COLOR_COMPONENT_G_BIT = 0x00000002,
        RHI_COLOR_COMPONENT_B_BIT = 0x00000004,
        RHI_COLOR_COMPONENT_A_BIT = 0x00000008,
        RHI_COLOR_COMPONENT_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
    };

    enum class ERHIPipeline : uint8
    {
        Graphics = 1 << 0,
        AsyncCompute = 1 << 1,

        None = 0,
        All = Graphics | AsyncCompute,
        Num = 2
    };


    //-----------------------------------------------
    //class
    //-----------------------------------------------
    class RHIResource
    {
    public:
        RHIResource(RHIResourceType InType = RRT_None) :mType(InType) {}
        void* mDevice = nullptr; /// RHI Type Pointer
        RHIResourceType mType = RRT_None;
    };

    class RHIBuffer : public RHIResource
    {
    public:
        RHIBuffer() :RHIResource(RRT_Buffer) {}
    };
    class RHIBufferView : public RHIResource
    {
    
    };
    class RHICommandBuffer : public RHIResource {};
    class RHICommandPool : public RHIResource {};
    class RHIDescriptorPool : public RHIResource {};
    class RHIDescriptorSet : public RHIResource {};
    class RHIDescriptorSetLayout : public RHIResource {};
    //class RHIDeviceMemory {};
    //class RHIEvent {};
    //class RHIFence {};
    class RHIFramebuffer : public RHIResource {};
    class RHITexture : public RHIResource 
    {
    public:
        RHITexture() :RHIResource(RRT_Texture){}
        PixelFormat GetFormat();
        TextureCreateFlags GetFlags();
        uint16 GetNumSamples();
    };
    class RHITextureView : public RHIResource {};
    class RHIPipeline : public RHIResource 
    {
    public:
        RHIPipeline() : RHIResource(RRT_Pipeline) {}
    };
    class RHIPipelineCache : public RHIResource {};
    class RHIPipelineLayout : public RHIResource {};
    //class RHIRenderPass {};
    class RHISampler : public RHIResource {};
    class RHIShader : public RHIResource
    {
    public:
        RHIShader(){ }
        const std::string& GetShaderString() { return mShaderString; }
    protected:
        std::string mShaderString;
    };
    class RHIRenderQuery :public RHIResource {};
    class RHIGPUFence :public RHIResource
    {
    public:
        RHIGPUFence() :RHIResource(RRT_GPUFence) {}
    };
    class RHIGraphicsPipelineState : public RHIResource
    {
    public:
        RHIGraphicsPipelineState() : RHIResource(RRT_GraphicsPipelineState) {}

        inline void SetSortKey(uint64 InSortKey) { mSortKey = InSortKey; }
        inline uint64 GetSortKey() const { return mSortKey; }

    private:
        uint64 mSortKey = 0;
    };

    class RHIUniformBuffer :public RHIResource
    {
        RHIUniformBuffer() :RHIResource(RRT_UniformBuffer) {}
    };

    class RHIViewport :public RHIResource
    {
    public:
        RHIViewport() :RHIResource(RRT_Viewport) {}
    };

    //-----------------------------------------------
    //struct
    //-----------------------------------------------

    struct LinearColor
    {
        union
        {
            struct
            {
                float	R,
                    G,
                    B,
                    A;
            };
            float RGBA[4];
        };
    };



    struct RHIMemoryBarrier;
    struct RHICopyDescriptorSet;
    struct RHIDescriptorImageInfo;
    struct RHIDescriptorBufferInfo;
    struct RHIOffset2D;
    struct RHISpecializationMapEntry;
    struct RHIBufferMemoryBarrier;
    struct RHIImageSubresourceRange;
    struct RHIImageMemoryBarrier;
    struct RHIExtent2D;
    struct RHIExtent3D;
    struct RHIApplicationInfo;
    struct RHIAttachmentDescription;
    struct RHIBufferCopy;
    struct RHIBufferCreateInfo;
    struct RHIBufferImageCopy;
    struct RHICommandBufferAllocateInfo;
    struct RHICommandBufferBeginInfo;
    struct RHICommandBufferInheritanceInfo;
    struct RHICommandPoolCreateInfo;
    struct RHIDescriptorPoolSize;
    struct RHIDescriptorPoolCreateInfo;
    struct RHIDescriptorSetAllocateInfo;
    struct RHIDescriptorSetLayoutBinding;
    struct RHIDescriptorSetLayoutCreateInfo;
    struct RHIDeviceCreateInfo;
    struct RHIDeviceQueueCreateInfo;
    struct RHIExtensionProperties;
    struct RHIFenceCreateInfo;
    struct RHIFormatProperties;
    struct RHIFramebufferCreateInfo;
    struct RHIGraphicsPipelineCreateInfo;
    struct RHIComputePipelineCreateInfo;
    struct RHIImageBlit;
    struct RHIImageCreateInfo;
    struct RHIImageFormatProperties;
    struct RHIImageViewCreateInfo;
    struct RHIInstanceCreateInfo;
    struct RHILayerProperties;
    struct RHIMemoryAllocateInfo;
    struct RHIMemoryHeap;
    struct RHIMemoryRequirements;
    struct RHIMemoryType;
    struct RHIPhysicalDeviceFeatures;
    struct RHIPhysicalDeviceLimits;
    struct RHIPhysicalDeviceMemoryProperties;
    struct RHIPhysicalDeviceProperties;
    struct RHIPhysicalDeviceSparseProperties;
    struct RHIPipelineColorBlendStateCreateInfo;
    struct RHIPipelineDepthStencilStateCreateInfo;
    struct RHIPipelineDynamicStateCreateInfo;
    struct RHIPipelineInputAssemblyStateCreateInfo;
    struct RHIPipelineLayoutCreateInfo;
    struct RHIPipelineMultisampleStateCreateInfo;
    struct RHIPipelineRasterizationStateCreateInfo;
    struct RHIPipelineShaderStageCreateInfo;
    struct RHIPipelineTessellationStateCreateInfo;
    struct RHIPipelineVertexInputStateCreateInfo;
    struct RHIPipelineViewportStateCreateInfo;
    struct RHIPushConstantRange;
    struct RHIQueueFamilyProperties;
    struct RHIRenderPassCreateInfo;
    struct RHISamplerCreateInfo;
    struct RHISemaphoreCreateInfo;
    struct RHIShaderModuleCreateInfo;
    struct RHISubmitInfo;
    struct RHISubpassDependency;
    struct RHISubpassDescription;
    struct RHIWriteDescriptorSet;
    struct RHIOffset3D;
    struct RHIAttachmentReference;
    struct RHIComponentMapping;
    struct RHIImageSubresourceLayers;
    struct RHIPipelineColorBlendAttachmentState;
    struct RHIRect2D;
    struct RHISpecializationInfo;
    struct RHIStencilOpState;
    struct RHIVertexInputAttributeDescription;
    struct RHIVertexInputBindingDescription;
    
    struct RHIRenderPassBeginInfo;
    union RHIClearValue;
    union RHIClearColorValue;
    struct RHIClearDepthStencilValue;




}