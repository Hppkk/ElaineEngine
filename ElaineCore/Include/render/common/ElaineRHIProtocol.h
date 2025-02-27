#pragma once
#include "render/common/ElaineRHITypes.h"
#include "ElaineVector2.h"
#include "ElaineVector3.h"

namespace Elaine
{
	//enum RHICommand
	//{
	//	CreateBuffer,
	//	DestroyBuffer,
	//	CreateTexture,
	//	DestroyTexture,
	//	CopyBuffer,
	//	Draw,

	//};

#define MAX_RENDER_TARGET_COUNT 8

	extern const char* VS_SHADER_MAIN;
	extern const char* PS_SHADER_MAIN;
	extern const char* GS_SHADER_MAIN;
	extern const char* CS_SHADER_MAIN;

	enum ERHIAccess
	{
		Unknown = 0,

		CPURead = 1 << 0,
		Present = 1 << 1,
		IndirectArgs = 1 << 2,
		VertexOrIndexBuffer = 1 << 3,
		SRVCompute = 1 << 4,
		SRVGraphics = 1 << 5,
		CopySrc = 1 << 6,
		ResolveSrc = 1 << 7,
		DSVRead = 1 << 8,

		// Read-write states
		UAVCompute = 1 << 9,
		UAVGraphics = 1 << 10,
		RTV = 1 << 11,
		CopyDest = 1 << 12,
		ResolveDst = 1 << 13,
		DSVWrite = 1 << 14,

		// Ray tracing acceleration structure states.
		// Buffer that contains an AS must always be in either of these states.
		// BVHRead -- required for AS inputs to build/update/copy/trace commands.
		// BVHWrite -- required for AS outputs of build/update/copy commands.
		BVHRead = 1 << 15,
		BVHWrite = 1 << 16,

		// Invalid released state (transient resources)
		Discard = 1 << 17,

		// Shading Rate Source
		ShadingRateSource = 1 << 18,

		Last = ShadingRateSource,
		None = Unknown,
		Mask = (Last << 1) - 1,

		// A mask of the two possible SRV states
		SRVMask = SRVCompute | SRVGraphics,

		// A mask of the two possible UAV states
		UAVMask = UAVCompute | UAVGraphics,

		// A mask of all bits representing read-only states which cannot be combined with other write states.
		ReadOnlyExclusiveMask = CPURead | Present | IndirectArgs | VertexOrIndexBuffer | SRVGraphics | SRVCompute | CopySrc | ResolveSrc | BVHRead,

		// A mask of all bits representing read-only states on the compute pipe which cannot be combined with other write states.
		ReadOnlyExclusiveComputeMask = CPURead | IndirectArgs | SRVCompute | CopySrc | BVHRead,

		// A mask of all bits representing read-only states which may be combined with other write states.
		ReadOnlyMask = ReadOnlyExclusiveMask | DSVRead | ShadingRateSource,

		// A mask of all bits representing readable states which may also include writable states.
		ReadableMask = ReadOnlyMask | UAVMask,

		// A mask of all bits representing write-only states which cannot be combined with other read states.
		WriteOnlyExclusiveMask = RTV | CopyDest | ResolveDst,

		// A mask of all bits representing write-only states which may be combined with other read states.
		WriteOnlyMask = WriteOnlyExclusiveMask | DSVWrite,

		// A mask of all bits representing writable states which may also include readable states.
		WritableMask = WriteOnlyMask | UAVMask | BVHWrite
	};

	struct Rect
	{
		int32 MinX = 0;
		int32 MaxX = 0;
		int32 MinY = 0;
		int32 MaxY = 0;
	};

	enum ECubeFace
	{
		CubeFace_PosX = 0,
		CubeFace_NegX,
		CubeFace_PosY,
		CubeFace_NegY,
		CubeFace_PosZ,
		CubeFace_NegZ,
		CubeFace_MAX
	};

	struct ViewportBounds
	{
		float	TopLeftX = .0f;
		float	TopLeftY = .0f;
		float	Width = .0f;
		float	Height = .0f;
		float	MinDepth = .0f;
		float	MaxDepth = 1.0f;
	};

	struct ResolveParams
	{
		/** used to specify face when resolving to a cube map texture */
		ECubeFace mCubeFace;
		/** resolve RECT bounded -1 for fullscreen */
		Rect mRect;
		Rect mDestRect;
		/** The mip index to resolve in both source and dest. */
		int32 mMipIndex;
		/** Array index to resolve in the source. */
		int32 mSourceArrayIndex;
		/** Array index to resolve in the dest. */
		int32 mDestArrayIndex;
		/** States to transition to at the end of the resolve operation. */
		ERHIAccess mSourceAccessFinal = ERHIAccess::SRVMask;
		ERHIAccess mDestAccessFinal = ERHIAccess::SRVMask;
	};


	enum class PrimitiveTopologyType : uint8
	{
		Triangle,
		Patch,
		Line,
		Point,
		//Quad,

		Num,
		NumBits = 2,
	};

	enum class EShaderStage
	{
		VertexShader,
		GeometryShader,
		FragmentShader,
		ComputeShader,
		ShaderStageMax,
	};

	enum CompareFunction
	{
		CF_Less,
		CF_LessEqual,
		CF_Greater,
		CF_GreaterEqual,
		CF_Equal,
		CF_NotEqual,
		CF_Never,
		CF_Always,

		CompareFunction_Num,
		CompareFunction_NumBits = 3,

	};



	/** Describes the dimension of a texture. */
	enum class TextureDimension : uint8
	{
		Texture2D,
		Texture2DArray,
		Texture3D,
		TextureCube,
		TextureCubeArray
	};

	enum VertexElementType
	{
		VET_None,
		VET_Float1,
		VET_Float2,
		VET_Float3,
		VET_Float4,
		VET_PackedNormal,	// FPackedNormal
		VET_UByte4,
		VET_UByte4N,
		VET_Color,
		VET_Short2,
		VET_Short4,
		VET_Short2N,		// 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
		VET_Half2,			// 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa 
		VET_Half4,
		VET_Short4N,		// 4 X 16 bit word, normalized 
		VET_UShort2,
		VET_UShort4,
		VET_UShort2N,		// 16 bit word normalized to (value/65535.0,value/65535.0,0,0,1)
		VET_UShort4N,		// 4 X 16 bit word unsigned, normalized 
		VET_URGB10A2N,		// 10 bit r, g, b and 2 bit a normalized to (value/1023.0f, value/1023.0f, value/1023.0f, value/3.0f)
		VET_UInt,
		VET_MAX,

		VET_NumBits = 5,
	};



	enum class BufferUsageFlags : uint32
	{
		None = 0,

		/** The buffer will be written to once. */
		Static = 1 << 0,

		/** The buffer will be written to occasionally, GPU read only, CPU write only.  The data lifetime is until the next update, or the buffer is destroyed. */
		Dynamic = 1 << 1,

		/** The buffer's data will have a lifetime of one frame.  It MUST be written to each frame, or a new one created each frame. */
		Volatile = 1 << 2,

		/** Allows an unordered access view to be created for the buffer. */
		UnorderedAccess = 1 << 3,

		/** Create a byte address buffer, which is basically a structured buffer with a uint32 type. */
		ByteAddressBuffer = 1 << 4,

		/** Buffer that the GPU will use as a source for a copy. */
		SourceCopy = 1 << 5,

		/** Create a buffer that can be bound as a stream output target. */
		StreamOutput = 1 << 6,

		/** Create a buffer which contains the arguments used by DispatchIndirect or DrawIndirect. */
		DrawIndirect = 1 << 7,

		/**
		 * Create a buffer that can be bound as a shader resource.
		 * This is only needed for buffer types which wouldn't ordinarily be used as a shader resource, like a vertex buffer.
		 */
		ShaderResource = 1 << 8,

		/** Request that this buffer is directly CPU accessible. */
		KeepCPUAccessible = 1 << 9,

		/** Buffer should go in fast vram (hint only). Requires BUF_Transient */
		FastVRAM = 1 << 10,

		/** Buffer should be allocated from transient memory. */
		Transient = None,

		/** Create a buffer that can be shared with an external RHI or process. */
		Shared = 1 << 12,

		/**
		 * Buffer contains opaque ray tracing acceleration structure data.
		 * Resources with this flag can't be bound directly to any shader stage and only can be used with ray tracing APIs.
		 * This flag is mutually exclusive with all other buffer flags except BUF_Static.
		*/
		AccelerationStructure = 1 << 13,

		VertexBuffer = 1 << 14,
		IndexBuffer = 1 << 15,
		StructuredBuffer = 1 << 16,

		/** Buffer memory is allocated independently for multiple GPUs, rather than shared via driver aliasing */
		MultiGPUAllocate = 1 << 17,

		/**
		 * Tells the render graph to not bother transferring across GPUs in multi-GPU scenarios.  Useful for cases where
		 * a buffer is read back to the CPU (such as streaming request buffers), or written to each frame by CPU (such
		 * as indirect arg buffers), and the other GPU doesn't actually care about the data.
		*/
		MultiGPUGraphIgnore = 1 << 18,

		/** Allows buffer to be used as a scratch buffer for building ray tracing acceleration structure,
		 * which implies unordered access. Only changes the buffer alignment and can be combined with other flags.
		**/
		RayTracingScratch = (1 << 19) | UnorderedAccess,

		// Helper bit-masks
		AnyDynamic = (Dynamic | Volatile),
	};

	enum class ClearDepthStencil
	{
		Depth,
		Stencil,
		DepthStencil,
	};


	enum class GpuVendorId
	{
		Unknown = -1,
		NotQueried = 0,

		Amd = 0x1002,
		ImgTec = 0x1010,
		Nvidia = 0x10DE,
		Arm = 0x13B5,
		Broadcom = 0x14E4,
		Qualcomm = 0x5143,
		Intel = 0x8086,
		Apple = 0x106B,
		Vivante = 0x7a05,
		VeriSilicon = 0x1EB1,

		Kazan = 0x10003,	// VkVendorId
		Codeplay = 0x10004,	// VkVendorId
		Mesa = 0x10005,	// VkVendorId
	};

	inline GpuVendorId RHIConvertToGpuVendorId(uint32 VendorId)
	{
		switch ((GpuVendorId)VendorId)
		{
		case GpuVendorId::NotQueried:
			return GpuVendorId::NotQueried;

		case GpuVendorId::Amd:
		case GpuVendorId::Mesa:
		case GpuVendorId::ImgTec:
		case GpuVendorId::Nvidia:
		case GpuVendorId::Arm:
		case GpuVendorId::Broadcom:
		case GpuVendorId::Qualcomm:
		case GpuVendorId::Intel:
			return (GpuVendorId)VendorId;

		default:
			break;
		}

		return GpuVendorId::Unknown;
	}





	enum EPrimitiveType
	{
		// Topology that defines a triangle N with 3 vertex extremities: 3*N+0, 3*N+1, 3*N+2.
		PT_TriangleList,

		// Topology that defines a triangle N with 3 vertex extremities: N+0, N+1, N+2.
		PT_TriangleStrip,

		// Topology that defines a line with 2 vertex extremities: 2*N+0, 2*N+1.
		PT_LineList,

		// Topology that defines a quad N with 4 vertex extremities: 4*N+0, 4*N+1, 4*N+2, 4*N+3.
		// Supported only if GRHISupportsQuadTopology == true.
		PT_QuadList,

		// Topology that defines a point N with a single vertex N.
		PT_PointList,

		// Topology that defines a screen aligned rectangle N with only 3 vertex corners:
		//    3*N + 0 is upper-left corner,
		//    3*N + 1 is upper-right corner,
		//    3*N + 2 is the lower-left corner.
		// Supported only if GRHISupportsRectTopology == true.
		PT_RectList,

		PT_Num,
		PT_NumBits = 6
	};

	/**
	* Action to take when a render target is set.
	*/
	enum class ERenderTargetLoadAction : uint8
	{
		// Untouched contents of the render target are undefined. Any existing content is not preserved.
		ENoAction,

		// Existing contents are preserved.
		ELoad,

		// The render target is cleared to the fast clear value specified on the resource.
		EClear,

		Num,
		NumBits = 2,
	};

	/**
	* Action to take when a render target is unset or at the end of a pass.
	*/
	enum class ERenderTargetStoreAction : uint8
	{
		// Contents of the render target emitted during the pass are not stored back to memory.
		ENoAction,

		// Contents of the render target emitted during the pass are stored back to memory.
		EStore,

		// Contents of the render target emitted during the pass are resolved using a box filter and stored back to memory.
		EMultisampleResolve,

		Num,
		NumBits = 2,
	};

	enum
	{
		MaxSimultaneousRenderTargets = 8,
		MaxSimultaneousRenderTargets_NumBits = 3,
	};

	enum class ExclusiveDepthStencil
	{

		// don't use those directly, use the combined versions below
		// 4 bits are used for depth and 4 for stencil to make the hex value readable and non overlapping
		DepthNop = 0x00,
		DepthRead = 0x01,
		DepthWrite = 0x02,
		DepthMask = 0x0f,
		StencilNop = 0x00,
		StencilRead = 0x10,
		StencilWrite = 0x20,
		StencilMask = 0xf0,

		// use those:
		DepthNop_StencilNop = DepthNop + StencilNop,
		DepthRead_StencilNop = DepthRead + StencilNop,
		DepthWrite_StencilNop = DepthWrite + StencilNop,
		DepthNop_StencilRead = DepthNop + StencilRead,
		DepthRead_StencilRead = DepthRead + StencilRead,
		DepthWrite_StencilRead = DepthWrite + StencilRead,
		DepthNop_StencilWrite = DepthNop + StencilWrite,
		DepthRead_StencilWrite = DepthRead + StencilWrite,
		DepthWrite_StencilWrite = DepthWrite + StencilWrite,
	};

	enum class ERenderTargetActions : uint8
	{
		LoadOpMask = 2,

#define RTACTION_MAKE_MASK(Load, Store) (((uint8)ERenderTargetLoadAction::Load << (uint8)LoadOpMask) | (uint8)ERenderTargetStoreAction::Store)

		DontLoad_DontStore = RTACTION_MAKE_MASK(ENoAction, ENoAction),

		DontLoad_Store = RTACTION_MAKE_MASK(ENoAction, EStore),
		Clear_Store = RTACTION_MAKE_MASK(EClear, EStore),
		Load_Store = RTACTION_MAKE_MASK(ELoad, EStore),

		Clear_DontStore = RTACTION_MAKE_MASK(EClear, ENoAction),
		Load_DontStore = RTACTION_MAKE_MASK(ELoad, ENoAction),
		Clear_Resolve = RTACTION_MAKE_MASK(EClear, EMultisampleResolve),
		Load_Resolve = RTACTION_MAKE_MASK(ELoad, EMultisampleResolve),

#undef RTACTION_MAKE_MASK
	};


	enum class EDepthStencilTargetActions : uint8
	{
		DepthMask = 4,

#define RTACTION_MAKE_MASK(Depth, Stencil) (((uint8)ERenderTargetActions::Depth << (uint8)DepthMask) | (uint8)ERenderTargetActions::Stencil)

		DontLoad_DontStore = RTACTION_MAKE_MASK(DontLoad_DontStore, DontLoad_DontStore),
		DontLoad_StoreDepthStencil = RTACTION_MAKE_MASK(DontLoad_Store, DontLoad_Store),
		DontLoad_StoreStencilNotDepth = RTACTION_MAKE_MASK(DontLoad_DontStore, DontLoad_Store),
		ClearDepthStencil_StoreDepthStencil = RTACTION_MAKE_MASK(Clear_Store, Clear_Store),
		LoadDepthStencil_StoreDepthStencil = RTACTION_MAKE_MASK(Load_Store, Load_Store),
		LoadDepthNotStencil_StoreDepthNotStencil = RTACTION_MAKE_MASK(Load_Store, DontLoad_DontStore),
		LoadDepthNotStencil_DontStore = RTACTION_MAKE_MASK(Load_DontStore, DontLoad_DontStore),
		LoadDepthStencil_StoreStencilNotDepth = RTACTION_MAKE_MASK(Load_DontStore, Load_Store),

		ClearDepthStencil_DontStoreDepthStencil = RTACTION_MAKE_MASK(Clear_DontStore, Clear_DontStore),
		LoadDepthStencil_DontStoreDepthStencil = RTACTION_MAKE_MASK(Load_DontStore, Load_DontStore),
		ClearDepthStencil_StoreDepthNotStencil = RTACTION_MAKE_MASK(Clear_Store, Clear_DontStore),
		ClearDepthStencil_StoreStencilNotDepth = RTACTION_MAKE_MASK(Clear_DontStore, Clear_Store),
		ClearDepthStencil_ResolveDepthNotStencil = RTACTION_MAKE_MASK(Clear_Resolve, Clear_DontStore),
		ClearDepthStencil_ResolveStencilNotDepth = RTACTION_MAKE_MASK(Clear_DontStore, Clear_Resolve),
		LoadDepthClearStencil_StoreDepthStencil = RTACTION_MAKE_MASK(Load_Store, Clear_Store),

		ClearStencilDontLoadDepth_StoreStencilNotDepth = RTACTION_MAKE_MASK(DontLoad_DontStore, Clear_Store),

#undef RTACTION_MAKE_MASK
	};


	/** Maximum number of miplevels in a texture. */
	enum { MAX_TEXTURE_MIP_COUNT = 15 };

	struct RHITextureDesc
	{
		TextureCreateFlags mFlags;
		/** Pixel format used to create RHI texture. */
		PixelFormat mFormat = PF_Unknown;

		/** Texture format used when creating the UAV. PF_Unknown means to use the default one (same as Format). */
		PixelFormat mUAVFormat = PF_Unknown;
		/** Extent of the texture in x and y. */
		Vector2 mExtent = Vector2(1, 1);

		/** Depth of the texture if the dimension is 3D. */
		uint16 mDepth = 1;

		/** The number of array elements in the texture. (Keep at 1 if dimension is 3D). */
		uint16 mArraySize = 1;

		/** Number of mips in the texture mip-map chain. */
		uint8 mNumMips = 1;

		/** Number of samples in the texture. >1 for MSAA. */
		uint8 mNumSamples : 5;

		/** Texture dimension to use when creating the RHI texture. */
		TextureDimension mDimension : 3;
	};

	struct RHICopyTextureInfo
	{
		Rect GetSourceRect() const
		{
			return Rect((int32)SourcePosition.x, (int32)SourcePosition.y, (int32)(SourcePosition.x + Size.x), (int32)(SourcePosition.y + Size.y));
		}

		Rect GetDestRect() const
		{
			return Rect((int32)DestPosition.x, (int32)DestPosition.y, (int32)(DestPosition.x + Size.x), (int32)(DestPosition.y + Size.y));
		}

		// Number of texels to copy. By default it will copy the whole resource if no size is specified.
		Vector3 Size = Vector3::ZERO;

		// Position of the copy from the source texture/to destination texture
		Vector3 SourcePosition = Vector3::ZERO;
		Vector3 DestPosition = Vector3::ZERO;

		uint32 SourceSliceIndex = 0;
		uint32 DestSliceIndex = 0;
		uint32 NumSlices = 1;

		// Mips to copy and destination mips
		uint32 SourceMipIndex = 0;
		uint32 DestMipIndex = 0;
		uint32 NumMips = 1;
	};

	struct ElaineCoreExport PixelFormatInfo
	{
		PixelFormatInfo() = delete;
		PixelFormatInfo(
			PixelFormat InFormat,
			const char* InName,
			int32 InBlockSizeX,
			int32 InBlockSizeY,
			int32 InBlockSizeZ,
			int32 InBlockBytes,
			int32 InNumComponents,
			uint32 InPlatformFormat,
			bool  InSupported);

		const char* Name;
		PixelFormat					PFormat;
		int32						BlockSizeX;
		int32						BlockSizeY;
		int32						BlockSizeZ;
		int32						BlockBytes;
		int32						NumComponents;


		/** Platform specific converted format (initialized by RHI module - invalid otherwise) */
		uint32						PlatformFormat{ 0 };

		/** Whether the texture format is supported on the current platform/ rendering combination */
		uint8						Supported : 1;
		uint8						bIs24BitUnormDepthStencil : 1;	// If false, 32 bit float is assumed (initialized by RHI module - invalid otherwise)

		/**
		* Get 2D/3D image/texture size in bytes. This is for storage of the encoded image data, and does not adjust
		* for any GPU alignment/padding constraints. It is also not valid for tiled or packed mip tails (i.e. cooked mips
		* for consoles). Only use these when you know you're working with bog standard textures/images in block based pixel formats.
		*/
		uint64 Get2DImageSizeInBytes(uint32 InWidth, uint32 InHeight) const;
		uint64 Get2DTextureMipSizeInBytes(uint32 InTextureWidth, uint32 InTextureHeight, uint32 InMipIndex) const;
		uint64 Get2DTextureSizeInBytes(uint32 InTextureWidth, uint32 InTextureHeight, uint32 InMipCount) const;
		uint64 Get3DImageSizeInBytes(uint32 InWidth, uint32 InHeight, uint32 InDepth) const;
		uint64 Get3DTextureMipSizeInBytes(uint32 InTextureWidth, uint32 InTextureHeight, uint32 InTextureDepth, uint32 InMipIndex) const;
		uint64 Get3DTextureSizeInBytes(uint32 InTextureWidth, uint32 InTextureHeight, uint32 InTextureDepth, uint32 InMipCount) const;

		/**
		* Get the number of compressed blocks necessary to hold the given dimensions.
		*/
		uint64 GetBlockCountForWidth(uint32 InWidth) const;
		uint64 GetBlockCountForHeight(uint32 InHeight) const;
	};

	extern ElaineCoreExport PixelFormatInfo GPixelFormats[PF_MAX_];

	inline constexpr EDepthStencilTargetActions MakeDepthStencilTargetActions(const ERenderTargetActions Depth, const ERenderTargetActions Stencil)
	{
		return (EDepthStencilTargetActions)(((uint8)Depth << (uint8)EDepthStencilTargetActions::DepthMask) | (uint8)Stencil);
	}

	inline ERenderTargetActions GetDepthActions(EDepthStencilTargetActions Action)
	{
		return (ERenderTargetActions)((uint8)Action >> (uint8)EDepthStencilTargetActions::DepthMask);
	}

	inline ERenderTargetActions GetStencilActions(EDepthStencilTargetActions Action)
	{
		return (ERenderTargetActions)((uint8)Action & ((1 << (uint8)EDepthStencilTargetActions::DepthMask) - 1));
	}

	inline ERenderTargetActions MakeRenderTargetActions(ERenderTargetLoadAction Load, ERenderTargetStoreAction Store)
	{
		return (ERenderTargetActions)(((uint8)Load << (uint8)ERenderTargetActions::LoadOpMask) | (uint8)Store);
	}

	inline ERenderTargetLoadAction GetLoadAction(ERenderTargetActions Action)
	{
		return (ERenderTargetLoadAction)((uint8)Action >> (uint8)ERenderTargetActions::LoadOpMask);
	}

	inline ERenderTargetStoreAction GetStoreAction(ERenderTargetActions Action)
	{
		return (ERenderTargetStoreAction)((uint8)Action & ((1 << (uint8)ERenderTargetActions::LoadOpMask) - 1));
	}

	struct GraphicsPipelineRenderTargetsInfo
	{
		uint32															RenderTargetsEnabled = 0;
		std::array<uint8, MaxSimultaneousRenderTargets>				RenderTargetFormats;
		std::array<TextureCreateFlags, MaxSimultaneousRenderTargets>	RenderTargetFlags;
		PixelFormat													DepthStencilTargetFormat = PF_Unknown;
		TextureCreateFlags												DepthStencilTargetFlag = TextureCreateFlags::None;
		ERenderTargetLoadAction											DepthTargetLoadAction = ERenderTargetLoadAction::ENoAction;
		ERenderTargetStoreAction										DepthTargetStoreAction = ERenderTargetStoreAction::ENoAction;
		ERenderTargetLoadAction											StencilTargetLoadAction = ERenderTargetLoadAction::ENoAction;
		ERenderTargetStoreAction										StencilTargetStoreAction = ERenderTargetStoreAction::ENoAction;
		ExclusiveDepthStencil											DepthStencilAccess;
		uint16															NumSamples = 0;
		uint8															MultiViewCount = 0;
		bool															bHasFragmentDensityAttachment = false;
	};

	struct RHIRenderPassInfo
	{
		struct ColorEntry
		{
			RHITexture* RenderTarget = nullptr;
			RHITexture* ResolveTarget = nullptr;
			int32                ArraySlice = -1;
			uint8                MipIndex = 0;
			ERenderTargetActions Action = ERenderTargetActions::DontLoad_DontStore;
		};
		std::array<ColorEntry, MaxSimultaneousRenderTargets> ColorRenderTargets;

		struct DepthStencilEntry
		{
			RHITexture* DepthStencilTarget = nullptr;
			RHITexture* ResolveTarget = nullptr;
			EDepthStencilTargetActions Action = EDepthStencilTargetActions::DontLoad_DontStore;
			ExclusiveDepthStencil ExclusiveDepthStencil;
		};
		DepthStencilEntry DepthStencilRenderTarget;

		// Controls the area for a multisample resolve or raster UAV (i.e. no fixed-function targets) operation.
		Rect ResolveRect;

		ResolveParams ResolveParameters;

		// Some RHIs can use a texture to control the sampling and/or shading resolution of different areas 
		RHITexture* ShadingRateTexture = nullptr;

		// Some RHIs require a hint that occlusion queries will be used in this render pass
		uint32 NumOcclusionQueries = 0;
		bool bOcclusionQueries = false;

		// if this renderpass should be multiview, and if so how many views are required
		uint8 MultiViewCount = 0;

		// Hint for some RHI's that renderpass will have specific sub-passes 
		//ESubpassHint SubpassHint = ESubpassHint::None;


		RHIRenderPassInfo() = default;
		RHIRenderPassInfo(const RHIRenderPassInfo&) = default;
		RHIRenderPassInfo& operator=(const RHIRenderPassInfo&) = default;

		// Color, no depth, optional resolve, optional mip, optional array slice
		explicit RHIRenderPassInfo(RHITexture* ColorRT, ERenderTargetActions ColorAction, RHITexture* ResolveRT = nullptr, uint8 InMipIndex = 0, int32 InArraySlice = -1)
		{
			ColorRenderTargets[0].RenderTarget = ColorRT;
			ColorRenderTargets[0].ResolveTarget = ResolveRT;
			ColorRenderTargets[0].ArraySlice = InArraySlice;
			ColorRenderTargets[0].MipIndex = InMipIndex;
			ColorRenderTargets[0].Action = ColorAction;
		}

		// Color MRTs, no depth
		explicit RHIRenderPassInfo(int32 NumColorRTs, RHITexture* ColorRTs[], ERenderTargetActions ColorAction)
		{
			for (int32 Index = 0; Index < NumColorRTs; ++Index)
			{
				ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
				ColorRenderTargets[Index].ArraySlice = -1;
				ColorRenderTargets[Index].Action = ColorAction;
			}
			DepthStencilRenderTarget.DepthStencilTarget = nullptr;
			DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
			DepthStencilRenderTarget.ExclusiveDepthStencil = ExclusiveDepthStencil::DepthNop_StencilNop;
			DepthStencilRenderTarget.ResolveTarget = nullptr;
		}

		// Color MRTs, no depth
		explicit RHIRenderPassInfo(int32 NumColorRTs, RHITexture* ColorRTs[], ERenderTargetActions ColorAction, RHITexture* ResolveTargets[])
		{
			for (int32 Index = 0; Index < NumColorRTs; ++Index)
			{
				ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
				ColorRenderTargets[Index].ResolveTarget = ResolveTargets[Index];
				ColorRenderTargets[Index].ArraySlice = -1;
				ColorRenderTargets[Index].MipIndex = 0;
				ColorRenderTargets[Index].Action = ColorAction;
			}
			DepthStencilRenderTarget.DepthStencilTarget = nullptr;
			DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
			DepthStencilRenderTarget.ExclusiveDepthStencil = ExclusiveDepthStencil::DepthNop_StencilNop;
			DepthStencilRenderTarget.ResolveTarget = nullptr;
		}

		// Color MRTs and depth
		explicit RHIRenderPassInfo(int32 NumColorRTs, RHITexture* ColorRTs[], ERenderTargetActions ColorAction, RHITexture* DepthRT, EDepthStencilTargetActions DepthActions, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
		{
			for (int32 Index = 0; Index < NumColorRTs; ++Index)
			{
				ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
				ColorRenderTargets[Index].ResolveTarget = nullptr;
				ColorRenderTargets[Index].ArraySlice = -1;
				ColorRenderTargets[Index].MipIndex = 0;
				ColorRenderTargets[Index].Action = ColorAction;
			}
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = nullptr;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		}

		// Color MRTs and depth
		explicit RHIRenderPassInfo(int32 NumColorRTs, RHITexture* ColorRTs[], ERenderTargetActions ColorAction, RHITexture* ResolveRTs[], RHITexture* DepthRT, EDepthStencilTargetActions DepthActions, RHITexture* ResolveDepthRT, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
		{
			for (int32 Index = 0; Index < NumColorRTs; ++Index)
			{
				ColorRenderTargets[Index].RenderTarget = ColorRTs[Index];
				ColorRenderTargets[Index].ResolveTarget = ResolveRTs[Index];
				ColorRenderTargets[Index].ArraySlice = -1;
				ColorRenderTargets[Index].MipIndex = 0;
				ColorRenderTargets[Index].Action = ColorAction;
			}
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		}

		// Depth, no color
		explicit RHIRenderPassInfo(RHITexture* DepthRT, EDepthStencilTargetActions DepthActions, RHITexture* ResolveDepthRT = nullptr, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
		{
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		}

		// Depth, no color, occlusion queries
		explicit RHIRenderPassInfo(RHITexture* DepthRT, uint32 InNumOcclusionQueries, EDepthStencilTargetActions DepthActions, RHITexture* ResolveDepthRT = nullptr, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
			: NumOcclusionQueries(InNumOcclusionQueries)
		{
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
		}

		// Color and depth
		explicit RHIRenderPassInfo(RHITexture* ColorRT, ERenderTargetActions ColorAction, RHITexture* DepthRT, EDepthStencilTargetActions DepthActions, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
		{
			ColorRenderTargets[0].RenderTarget = ColorRT;
			ColorRenderTargets[0].ResolveTarget = nullptr;
			ColorRenderTargets[0].ArraySlice = -1;
			ColorRenderTargets[0].MipIndex = 0;
			ColorRenderTargets[0].Action = ColorAction;
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = nullptr;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
			Memory::MemoryZero(&ColorRenderTargets[1], sizeof(ColorEntry) * (MaxSimultaneousRenderTargets - 1));
		}

		// Color and depth with resolve
		explicit RHIRenderPassInfo(RHITexture* ColorRT, ERenderTargetActions ColorAction, RHITexture* ResolveColorRT,
			RHITexture* DepthRT, EDepthStencilTargetActions DepthActions, RHITexture* ResolveDepthRT, ExclusiveDepthStencil InEDS = ExclusiveDepthStencil::DepthWrite_StencilWrite)
		{

			ColorRenderTargets[0].RenderTarget = ColorRT;
			ColorRenderTargets[0].ResolveTarget = ResolveColorRT;
			ColorRenderTargets[0].ArraySlice = -1;
			ColorRenderTargets[0].MipIndex = 0;
			ColorRenderTargets[0].Action = ColorAction;
			DepthStencilRenderTarget.DepthStencilTarget = DepthRT;
			DepthStencilRenderTarget.ResolveTarget = ResolveDepthRT;
			DepthStencilRenderTarget.Action = DepthActions;
			DepthStencilRenderTarget.ExclusiveDepthStencil = InEDS;
			Memory::MemoryZero(&ColorRenderTargets[1], sizeof(ColorEntry) * (MaxSimultaneousRenderTargets - 1));
		}

		inline int32 GetNumColorRenderTargets() const
		{
			int32 ColorIndex = 0;
			for (; ColorIndex < MaxSimultaneousRenderTargets; ++ColorIndex)
			{
				const ColorEntry& Entry = ColorRenderTargets[ColorIndex];
				if (!Entry.RenderTarget)
				{
					break;
				}
			}

			return ColorIndex;
		}

		GraphicsPipelineRenderTargetsInfo ExtractRenderTargetsInfo() const
		{
			GraphicsPipelineRenderTargetsInfo RenderTargetsInfo;

			RenderTargetsInfo.NumSamples = 1;
			int32 RenderTargetIndex = 0;

			for (; RenderTargetIndex < MaxSimultaneousRenderTargets; ++RenderTargetIndex)
			{
				RHITexture* RenderTarget = ColorRenderTargets[RenderTargetIndex].RenderTarget;
				if (!RenderTarget)
				{
					break;
				}

				RenderTargetsInfo.RenderTargetFormats[RenderTargetIndex] = (uint8)RenderTarget->GetFormat();
				RenderTargetsInfo.RenderTargetFlags[RenderTargetIndex] = RenderTarget->GetFlags();
				RenderTargetsInfo.NumSamples |= RenderTarget->GetNumSamples();
			}

			RenderTargetsInfo.RenderTargetsEnabled = RenderTargetIndex;
			for (; RenderTargetIndex < MaxSimultaneousRenderTargets; ++RenderTargetIndex)
			{
				RenderTargetsInfo.RenderTargetFormats[RenderTargetIndex] = PF_Unknown;
			}

			if (DepthStencilRenderTarget.DepthStencilTarget)
			{
				RenderTargetsInfo.DepthStencilTargetFormat = DepthStencilRenderTarget.DepthStencilTarget->GetFormat();
				RenderTargetsInfo.DepthStencilTargetFlag = DepthStencilRenderTarget.DepthStencilTarget->GetFlags();
				RenderTargetsInfo.NumSamples |= DepthStencilRenderTarget.DepthStencilTarget->GetNumSamples();
			}
			else
			{
				RenderTargetsInfo.DepthStencilTargetFormat = PF_Unknown;
			}

			const ERenderTargetActions DepthActions = GetDepthActions(DepthStencilRenderTarget.Action);
			const ERenderTargetActions StencilActions = GetStencilActions(DepthStencilRenderTarget.Action);
			RenderTargetsInfo.DepthTargetLoadAction = GetLoadAction(DepthActions);
			RenderTargetsInfo.DepthTargetStoreAction = GetStoreAction(DepthActions);
			RenderTargetsInfo.StencilTargetLoadAction = GetLoadAction(StencilActions);
			RenderTargetsInfo.StencilTargetStoreAction = GetStoreAction(StencilActions);
			RenderTargetsInfo.DepthStencilAccess = DepthStencilRenderTarget.ExclusiveDepthStencil;

			RenderTargetsInfo.MultiViewCount = MultiViewCount;
			RenderTargetsInfo.bHasFragmentDensityAttachment = ShadingRateTexture != nullptr;

			return RenderTargetsInfo;
		}

		//void ConvertToRenderTargetsInfo(RHISetRenderTargetsInfo& OutRTInfo) const;

		//////////////////////////////////////////////////////////////////////////
		// Deprecated
		//////////////////////////////////////////////////////////////////////////

		bool IsMSAA() const { return bIsMSAA; }


		bool bIsMSAA = false;

		bool bTooManyUAVs = false;

		bool bGeneratingMips = false;
	};


	struct RHIRectScissor2D
	{

	};

	enum RHICullMode
	{
		CULL_NONE, //无剔除
		CULL_FRONT, //剔除正面
		CULL_BACK, //剔除背面
		CULL_ALL, //正面和背面全都剔除
	};

	enum RHIPolygonMode
	{
		POLYGON_FILL, //绘制实心三角形
		POLYGON_LINE, //绘制三角形的线框
		POLYGON_POINT, //绘制三角形的顶点
	};

	enum RHIMultiSampleCount
	{
		SAMPLE_COUNT_1 = 0x00000001,
		SAMPLE_COUNT_2 = 0x00000002,
		SAMPLE_COUNT_4 = 0x00000004,
		SAMPLE_COUNT_8 = 0x00000008,
		SAMPLE_COUNT_16 = 0x00000010,
		SAMPLE_COUNT_32 = 0x00000020,
		SAMPLE_COUNT_64 = 0x00000040,
	};

	enum RHICompareOp
	{
		COMPARE_OP_NEVER,					//一定不通过测试
		COMPARE_OP_LESS,					//表示若新片段的深度值小于深度模板附件中已有的深度值时，通过深度测试
		COMPARE_OP_EQUAL,					//表示若新片段的深度值等于DS附件中已有的深度值时，通过深度测试
		COMPARE_OP_LESS_OR_EQUAL,			//表示若新深度值小于等于DS附件中已有的深度值时，通过深度测试
		COMPARE_OP_GREATER,					//表示若新深度值大于DS附件中已有的深度值时，通过深度测试
		COMPARE_OP_NOT_EQUAL,				//表示若新深度值不等于DS附件中已有的深度值时，通过深度测试
		COMPARE_OP_GREATER_OR_EQUAL,		//表示若新深度值大于等于DS附件中已有的深度值时，通过深度测试
		COMPARE_OP_ALWAYS,					//表示无论深度值如何，一定通过深度测试
	};

	enum RHIStencilOp
	{
		STENCIL_OP_KEEP,						//表示不改变模板缓冲中原有的数值
		STENCIL_OP_ZERO,						//表示使用0作为写入值
		STENCIL_OP_REPLACE,						//表示使用reference作为写入值
		STENCIL_OP_INCREMENT_AND_CLAMP,			//表示使用DS附件中原有数值 + 1作为写入值，并钳制到255
		STENCIL_OP_DECREMENT_AND_CLAMP,			//表示使用DS附件中原有数值 - 1作为写入值，并钳制到0
		STENCIL_OP_INVERT,						//表示将DS附件中原有数值按位取反，以该数值为写入值
		STENCIL_OP_INCREMENT_AND_WRAP,			//表示使用DS附件中原有数值 + 1作为写入值，若原有数值为255，写入值为0
		STENCIL_OP_DECREMENT_AND_WRAP,			//表示使用DS附件中原有数值 - 1作为写入值，若原有数值为0，写入值为255
	};

#define RHI_STREAM_MAX 4
#define RHI_MAX_VS_BUFFER_COUNT 2
#define RHI_MAX_PS_BUFFER_COUNT 8

	enum RHI_TEXTURE_REG
	{
		RTR_TEXTURE0,
		RTR_TEXTURE1,
		RTR_TEXTURE2,
		RTR_TEXTURE3,
		RTR_TEXTURE4,
		RTR_TEXTURE5,
		RTR_TEXTURE6,
		RTR_TEXTURE7,
		RTR_TEXTURE8,
		RTR_TEXTURE9,
		RTR_TEXTURE10,
		RTR_TEXTURE11,
		RTR_TEXTURE_MAX,
	};

	enum RHI_CUSTOM_BUFFER_REG
	{
		RBR_BUFFER0,
		RBR_BUFFER1,
		RBR_BUFFER2,
		RBR_BUFFER3,
		RBR_BUFFER4,
		RBR_BUFFER5,
		RBR_BUFFER6,
		RBR_BUFFER7,
		RBR_BUFFER_MAX,
	};

	enum RHI_UNIFORM_REG
	{
		RUR_WORLD_MATRIX,
		RUR_VIEW_MATRIX,
		RUR_CAMERA_WORLD_POSITION,

		RUR_UNIFORM0,
		RUR_UNIFORM1,
		RUR_UNIFORM2,
		RUR_UNIFORM3,
		RUR_UNIFORM4,
		RUR_UNIFORM5,
		RUR_UNIFORM6,
		RUR_UNIFORM7,
		RUR_UNIFORM8,
		RUR_UNIFORM9,
		RUR_UNIFORM10,
		RUR_UNIFORM11,
		RUR_UNIFORM_MAX,
	};

	enum RHI_STREAM_INPUT
	{
		STREAM_VERTEXBUFFER,
		STREAM_CUSTOMBUFFER0,
		STREAM_CUSTOMBUFFER1,
		STREAM_CUSTOMBUFFER2,
		STREAM_CUSTOMBUFFER3,
		STREAM_INDEXBUFFER,
		STREAM_INPUT_MAX,
	};

	enum RHI_VERTEX_ATTRIBUTE
	{
		RVA_POSITION0,
		RVA_POSITION1,
		RVA_POSITION2,
		RVA_POSITION3,
		RVA_POSITION4,
		RVA_POSITION5,
		RVA_POSITION6,
		RVA_POSITION7,
		RVA_NORMAL0,
		RVA_NORMAL1,
		RVA_NORMAL2,
		RVA_NORMAL3,
		RVA_COLOR0,
		RVA_COLOR1,
		RVA_COLOR2,
		RVA_COLOR3,
		RVA_TEXCOORD0,
		RVA_TEXCOORD1,
		RVA_TEXCOORD2,
		RVA_TEXCOORD3,
	};

	enum RHIIndexType
	{
		INDEX_TYPE_UINT16 = 0,
		INDEX_TYPE_UINT32 = 1,
		INDEX_TYPE_NONE_KHR = 1000165000,
		INDEX_TYPE_UINT8_EXT = 1000265000,
		INDEX_TYPE_MAX_ENUM = 0x7FFFFFFF
	};

	struct RHIVertexBufferDataDesc
	{
		RHIBuffer* mRHIBuffer = nullptr;
		size_t mOffset[16];
		VertexElementType mFormat[16];
		size_t mAttributeSize = 0;
		size_t mStride = 0;
	};

	struct RHIIStreamDesc
	{
		RHIBuffer* mIStreamBuffer[STREAM_INPUT_MAX] = {nullptr};
	};

	struct RHIDrawData
	{
		RHIIStreamDesc	mStreamInput;
		RHITexture*		mPSTextures[RTR_TEXTURE_MAX] = { nullptr };
		//RHIBuffer*		mVsBuffers[RHI_MAX_VS_BUFFER_COUNT];
		RHIBuffer*		mPsBuffers[RHI_MAX_PS_BUFFER_COUNT] = { nullptr };
	};

	struct GRAPHICS_PIPELINE_STATE_DESC
	{
		std::string						mVSShaderCode;
		std::string						mPSShaderCode;
		//RHIShader*						mVSShader;
		//RHIShader*						mPSShader;
		RHIVertexBufferDataDesc			mVertexAttribute;
		RHIIndexType					mIndexType = INDEX_TYPE_UINT16;
		RHIDrawData*					mRHIDrawData = nullptr;
		EPrimitiveType					mPrimitiveType = PT_TriangleList;
		PixelFormat						mRenderTargetFormats[MAX_RENDER_TARGET_COUNT];
		TextureCreateFlags				mRenderTargetFlags[MAX_RENDER_TARGET_COUNT];
		PixelFormat						mDepthStencilTargetFormat;
		TextureCreateFlags				mDepthStencilTargetFlag;
		ERenderTargetLoadAction			mDepthTargetLoadAction;
		ERenderTargetStoreAction		mDepthTargetStoreAction;
		ERenderTargetLoadAction			mStencilTargetLoadAction;
		ERenderTargetStoreAction		mStencilTargetStoreAction;
		ExclusiveDepthStencil			mDepthStencilAccess;
		RHISampler**					mSamples = nullptr;
		RHITexture**					mTextures = nullptr;
		RHITexture*						mRenderTarget[MAX_RENDER_TARGET_COUNT] = { nullptr };
		uint16							mNumSamples = 0;
		uint8							mMultiViewCount = 1;
		bool							mbTessellation = false;
		bool							mbUseMultiViewport = false;
		bool							mbUseDefultViewPort = true;
		std::vector<ViewportBounds>		mViewBounds;
		std::vector<RHIRectScissor2D>	mScissors;
		uint64							mStateHash = 0;
		RHICullMode						mCullMode = CULL_BACK;
		RHIPolygonMode					mPolygonMode = POLYGON_FILL;
		RHIMultiSampleCount				mMultiSampleCount = SAMPLE_COUNT_1;
		bool							mbDepthTestEnable = true;
		bool							mbDepthWriteEnable = true;
		RHICompareOp					mDepthOp = COMPARE_OP_LESS;
		bool							mStencilTestEnable = false;
		bool							mbEnableColorBlend = false;
		//SubPassInfo					mSubpass; todo for some support subpass
	};

	struct ComputePipelineStateDesc
	{

	};
}


