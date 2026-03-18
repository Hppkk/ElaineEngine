#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanUtil.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "render/vulkan/ElaineVulkanBarrier.h"
#include "render/ElaineRenderSystem.h"
#define VULKAN_SUPPORTS_EXTERNAL_MEMORY 1

namespace VulkanRHI
{

	static const VkImageTiling GVulkanViewTypeTilingMode[VK_IMAGE_VIEW_TYPE_CUBE_ARRAY - VK_IMAGE_VIEW_TYPE_1D + 1] =
	{
		VK_IMAGE_TILING_LINEAR,		// VK_IMAGE_VIEW_TYPE_1D
		VK_IMAGE_TILING_OPTIMAL,	// VK_IMAGE_VIEW_TYPE_2D
		VK_IMAGE_TILING_OPTIMAL,	// VK_IMAGE_VIEW_TYPE_3D
		VK_IMAGE_TILING_OPTIMAL,	// VK_IMAGE_VIEW_TYPE_CUBE
		VK_IMAGE_TILING_LINEAR,		// VK_IMAGE_VIEW_TYPE_1D_ARRAY
		VK_IMAGE_TILING_OPTIMAL,	// VK_IMAGE_VIEW_TYPE_2D_ARRAY
		VK_IMAGE_TILING_OPTIMAL,	// VK_IMAGE_VIEW_TYPE_CUBE_ARRAY
	};

	VkImageAspectFlags GenerateVkImageAspectBits(RHITextureDesc InDesc)
	{
		if ((InDesc.mFlags & TextureCreateFlags::DepthStencilTargetable) != TextureCreateFlags::None)
		{
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		
		
		return VK_IMAGE_ASPECT_COLOR_BIT;
		

	}

	VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc)
		: RHITexture(InDesc)
		, mDevice(InDevice)
		, mViewFormat(VK_FORMAT_UNDEFINED)
		, mMemProps(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		, mTiling(VK_IMAGE_TILING_MAX_ENUM)
		, mFullAspectMask(0)
		, mPartialAspectMask(0)
		, mCpuReadbackBuffer(nullptr)
	{
		if (EnumHasAnyFlags(InDesc.mFlags, TextureCreateFlags::CPUReadback))
		{
			mCpuReadbackBuffer = new VulkanCpuReadbackBuffer();
			uint32 Size = 0;
			for (uint32 Mip = 0; Mip < InDesc.mNumMips; ++Mip)
			{
				uint32 LocalSize = 0;
				//todo 计算要分配的buffer大小
			}
			return;
		}
		mFullAspectMask = GenerateVkImageAspectBits(InDesc);
		VulkanImageCreateInfo ImageCreateInfo;
		GenerateImageCreateInfo(ImageCreateInfo, mDevice, mDesc, &mStorageFormat, &mViewFormat);
		mImageLayout = ImageCreateInfo.mImageCreateInfo.initialLayout;
		mAccess = ERHIAccess::Unknown;
		vkCreateImage(InDevice->GetDevice(), &ImageCreateInfo.mImageCreateInfo, VULKAN_CPU_ALLOCATOR, &mHandle);
		vkGetImageMemoryRequirements(InDevice->GetDevice(), mHandle, &mMemRequirements);
		if (ImageCreateInfo.mImageCreateInfo.tiling != VK_IMAGE_TILING_OPTIMAL)
		{
			mMemProps |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		}

		const bool bRenderTarget = EnumHasAnyFlags(InDesc.mFlags, (TextureCreateFlags)((uint64)TextureCreateFlags::RenderTargetable | (uint64)TextureCreateFlags::DepthStencilTargetable | (uint64)TextureCreateFlags::ResolveTargetable));
		const bool bUAV = EnumHasAnyFlags(InDesc.mFlags, TextureCreateFlags::UAV);
		const bool bDynamic = EnumHasAnyFlags(InDesc.mFlags, TextureCreateFlags::Dynamic);
		const bool bExternal = EnumHasAnyFlags(InDesc.mFlags, TextureCreateFlags::External);
		VkMemoryPropertyFlags MemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bool bMemoryless = EnumHasAnyFlags(InDesc.mFlags, TextureCreateFlags::Memoryless) && InDevice->GetDeviceMemoryManager()->SupportsMemoryless();
		if (bMemoryless)
		{
			if (bRenderTarget)
			{
				MemoryFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			}
			else
			{
				bMemoryless = false;
			}
		}
		
		{
			VulkanAllocationMetaType MetaType = (bRenderTarget || bUAV) ? VulkanAllocationMetaImageRenderTarget : VulkanAllocationMetaImageOther;

			if (bExternal)
			{
				// 查询驱动支持的外部内存 Handle 类型
				mExternalHandleType = QuerySupportedExternalHandleType(
					InDevice,
					ImageCreateInfo.mImageCreateInfo.format,
					ImageCreateInfo.mImageCreateInfo.imageType,
					ImageCreateInfo.mImageCreateInfo.tiling,
					ImageCreateInfo.mImageCreateInfo.usage,
					ImageCreateInfo.mImageCreateInfo.flags);

				if (mExternalHandleType == (VkExternalMemoryHandleTypeFlagBits)0)
				{
					LOG_ERROR("VulkanTexture: No supported external memory handle type found for this format/usage");
				}

				// D3D11_TEXTURE handle type requires dedicated allocation
				if (!InDevice->GetMemoryManager()->AllocateDedicatedImageMemory(mAllocation, this, mHandle, mMemRequirements, MemoryFlags, MetaType, bExternal, __FILE__, __LINE__, mExternalHandleType))
				{
					LOG_ERROR("VulkanTexture: Failed to allocate dedicated image memory for external texture");
				}
			}
			else if (!InDevice->GetMemoryManager()->AllocateImageMemory(mAllocation, this, mMemRequirements, MemoryFlags, MetaType, bExternal, __FILE__, __LINE__))
			{
			}
		}
		mAllocation.BindImage(mDevice, mHandle);

		mTiling = ImageCreateInfo.mImageCreateInfo.tiling;
		const VkImageViewType ViewType = GetViewType();
		const bool bArray = ViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		if (mViewFormat == VK_FORMAT_UNDEFINED)
		{
			mStorageFormat = EngineToVkTextureFormat(InDesc.mFormat, false);
			mViewFormat = EngineToVkTextureFormat(InDesc.mFormat, EnumHasAllFlags(InDesc.mFlags, TextureCreateFlags::SRGB));
		}

		if (ViewType != VK_IMAGE_VIEW_TYPE_MAX_ENUM)
		{
			mDefaultView.Create(*InDevice, mHandle, ViewType, GetFullAspectMask(), InDesc.mFormat, mViewFormat, 0, Math::max(InDesc.mNumMips, (uint8)1u), 0, bArray ? Math::max((uint16)1u, InDesc.mArraySize) : Math::max((uint16)1u, InDesc.mDepth), true);
		}

		if (mFullAspectMask == mPartialAspectMask)
		{
			mPartialView = &mDefaultView;
		}
		else
		{
			//mPartialView = new VulkanTextureView;
			//mPartialView->Create(*InDevice, mHandle, ViewType, mPartialAspectMask, InDesc.mFormat, mViewFormat, 0, Math::max(InDesc.mNumMips, (uint8)1u), 0, bArray ? Math::max((uint16)1u, InDesc.mArraySize) : Math::max((uint16)1u, InDesc.mDepth), false);
		}


	}

	// Swapchain 图像包装构造函数（不创建 VkImage，不分配内存）
	VulkanTexture::VulkanTexture(VulkanDevice* InDevice, const RHITextureDesc& InDesc, VkImage InExistingImage)
		: RHITexture(InDesc)
		, mDevice(InDevice)
		, mHandle(InExistingImage)  // 使用已有的 VkImage
		, mViewFormat(VK_FORMAT_UNDEFINED)
		, mMemProps(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		, mTiling(VK_IMAGE_TILING_OPTIMAL)
		, mFullAspectMask(0)
		, mPartialAspectMask(0)
		, mCpuReadbackBuffer(nullptr)
	{
		// Swapchain 图像已经由 Vulkan 创建，不需要创建或分配内存
		mFullAspectMask = GenerateVkImageAspectBits(InDesc);
		mPartialAspectMask = mFullAspectMask;
		
		// 设置 Format
		mStorageFormat = EngineToVkTextureFormat(InDesc.mFormat, EnumHasAllFlags(InDesc.mFlags, TextureCreateFlags::SRGB));
		mViewFormat = EngineToVkTextureFormat(InDesc.mFormat, EnumHasAllFlags(InDesc.mFlags, TextureCreateFlags::SRGB));
		
		// 设置初始布局（Swapchain 图像通常初始为 UNDEFINED 或 COLOR_ATTACHMENT_OPTIMAL）
		mImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		mAccess = ERHIAccess::Unknown;
		
		// 创建 View
		const VkImageViewType ViewType = GetViewType();
		if (ViewType != VK_IMAGE_VIEW_TYPE_MAX_ENUM)
		{
			const bool bArray = ViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			mDefaultView.Create(*InDevice, mHandle, ViewType, GetFullAspectMask(), InDesc.mFormat, mViewFormat, 
				0, Math::max(InDesc.mNumMips, (uint8)1u), 0, 
				bArray ? Math::max((uint16)1u, InDesc.mArraySize) : Math::max((uint16)1u, InDesc.mDepth), true);
		}
		
		mPartialView = &mDefaultView;
		mbOwnsImage = false;  // 外部 VkImage，不拥有所有权
	}

	void VulkanTexture::Evict(VulkanDevice& InDevice, VulkanCommandContext& Context)
	{
		uint64 Size = GetMemorySize();
		static uint64 TotalSize = 0;
		TotalSize += Size;

		{
			const TextureCreateFlags UEFlags = GetDesc().mFlags;
			const bool bRenderTarget = EnumHasAnyFlags(UEFlags, TextureCreateFlags((uint64)TextureCreateFlags::RenderTargetable | (uint64)TextureCreateFlags::DepthStencilTargetable | (uint64)TextureCreateFlags::ResolveTargetable));
			const bool bUAV = EnumHasAnyFlags(UEFlags, TextureCreateFlags::UAV);

			mMemProps = InDevice.GetDeviceMemoryManager()->GetEvictedMemoryProperties();

			// Create a new host allocation to move the surface to
			VulkanAllocation HostAllocation;
			const VulkanAllocationMetaType MetaType = VulkanAllocationMetaImageOther;
			if (!InDevice.GetMemoryManager()->AllocateImageMemory(HostAllocation, this, mMemRequirements, mMemProps, MetaType, false, __FILE__, __LINE__))
			{

			}

			InternalMoveSurface(InDevice, Context, HostAllocation);

			// Delete the original allocation and swap in the new host allocation
			mDevice->GetMemoryManager()->FreeVulkanAllocation(mAllocation);
			mAllocation.Swap(HostAllocation);
		}
	}

	void VulkanTexture::Move(VulkanDevice& InDevice, VulkanCommandContext& Context, VulkanAllocation& NewAllocation)
	{
		uint64 Size = GetMemorySize();
		static uint64 TotalSize = 0;
		TotalSize += Size;
		{
			const TextureCreateFlags UEFlags = GetDesc().mFlags;
			const bool bRenderTarget = EnumHasAnyFlags(UEFlags, TextureCreateFlags((uint64)TextureCreateFlags::RenderTargetable | (uint64)TextureCreateFlags::DepthStencilTargetable | (uint64)TextureCreateFlags::ResolveTargetable));
			const bool bUAV = EnumHasAnyFlags(UEFlags, TextureCreateFlags::UAV);
			                                                                                                                                                                                                                                                                                                                                       
			InternalMoveSurface(InDevice, Context, NewAllocation);

			// Swap in the new allocation for this surface
			mAllocation.Swap(NewAllocation);
		}
		InvalidateViews(InDevice);
	}

	void VulkanTexture::InvalidateViews(VulkanDevice& InDevice)
	{
		mDefaultView.Destroy(InDevice);
		const RHITextureDesc& Desc = GetDesc();
		const uint32 NumMips = Desc.mNumMips;
		const VkImageViewType ViewType = GetViewType();
		const bool bArray = ViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY || ViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		const uint32 SizeZOrArraySize = bArray ? Math::max((uint16)1u, Desc.mArraySize) : Math::max((uint16)1u, Desc.mDepth);

		if (ViewType != VK_IMAGE_VIEW_TYPE_MAX_ENUM)
		{
			mDefaultView.Create(InDevice, mHandle, ViewType, GetFullAspectMask(), GetDesc().mFormat, mViewFormat, 0, Math::max(NumMips, 1u), 0, SizeZOrArraySize, true);
		}
		if (mPartialView != &mDefaultView)
		{
			mPartialView->Destroy(*mDevice);
			mPartialView->Create(InDevice, mHandle, ViewType, mPartialAspectMask, GetDesc().mFormat, mViewFormat, 0, Math::max(NumMips, 1u), 0, SizeZOrArraySize, false);
		}

	}

	void VulkanTexture::InternalMoveSurface(VulkanDevice& InDevice, VulkanCommandContext& Context, VulkanAllocation& DestAllocation)
	{
		//VulkanImageCreateInfo ImageCreateInfo;                                                        
		//const RHITextureDesc& Desc = GetDesc();
		//VulkanTexture::GenerateImageCreateInfo(ImageCreateInfo, &InDevice, Desc, &mStorageFormat, &mViewFormat);

		//VkImage MovedImage;
		//vkCreateImage(InDevice.GetDevice(), &ImageCreateInfo.mImageCreateInfo, VULKAN_CPU_ALLOCATOR, &MovedImage);
		//
		//const TextureCreateFlags UEFlags = Desc.mFlags;
		//const bool bRenderTarget = EnumHasAnyFlags(UEFlags, TextureCreateFlags((uint64)TextureCreateFlags::RenderTargetable | (uint64)TextureCreateFlags::DepthStencilTargetable | (uint64)TextureCreateFlags::ResolveTargetable));
		//const bool bCPUReadback = EnumHasAnyFlags(UEFlags, TextureCreateFlags::CPUReadback);
		//const bool bMemoryless = EnumHasAnyFlags(UEFlags, TextureCreateFlags::Memoryless);
		//const bool bExternal = EnumHasAnyFlags(UEFlags, TextureCreateFlags::External);

		//VkMemoryRequirements MovedMemReqs;
		//vkGetImageMemoryRequirements(InDevice.GetDevice(), MovedImage, &MovedMemReqs);


		//DestAllocation.BindImage(&InDevice, MovedImage);

		//// Copy Original -> Moved
		//VulkanCommandBuffer* CmdBuffer = Context.GetCommandBufferManager()->GetActiveCmdBuffer();
		//VkCommandBuffer VkCmdBuffer = CmdBuffer->GetHandle();

		//const uint32 NumberOfArrayLevels = GetNumberOfArrayLevels();
		//VulkanImageLayout* MovedLayout = &Context.GetLayoutManager().FindOrAddFullLayoutRW(MovedImage, VK_IMAGE_LAYOUT_UNDEFINED, GetNumMips(), NumberOfArrayLevels);
		//VulkanImageLayout* OriginalLayout = Context.GetLayoutManager().GetFullLayout(Image);
		//// Account for map resize, should rarely happen...
		//if (OriginalLayout == nullptr)
		//{
		//	OriginalLayout = &Context.GetLayoutManager().GetOrAddFullLayout(*this, VK_IMAGE_LAYOUT_UNDEFINED);
		//	MovedLayout = &Context.GetLayoutManager().GetFullLayoutChecked(MovedImage);
		//}

		//checkf((OriginalLayout->NumMips == GetNumMips()), TEXT("NumMips reported by LayoutManager (%d) differs from surface (%d)"), OriginalLayout->NumMips, GetNumMips());
		//checkf((OriginalLayout->NumLayers == NumberOfArrayLevels), TEXT("NumLayers reported by LayoutManager (%d) differs from surface (%d)"), OriginalLayout->NumLayers, NumberOfArrayLevels);
		//{
		//	// Transition to copying layouts
		//	{
		//		FVulkanPipelineBarrier Barrier;
		//		Barrier.AddImageLayoutTransition(Image, FullAspectMask, *OriginalLayout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		//		Barrier.AddImageLayoutTransition(MovedImage, FullAspectMask, *MovedLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		//		Barrier.Execute(VkCmdBuffer);
		//	}
		//	{
		//		VkImageCopy Regions[MAX_TEXTURE_MIP_COUNT];
		//		check(Desc.NumMips <= MAX_TEXTURE_MIP_COUNT);
		//		FMemory::Memzero(Regions);
		//		for (uint32 i = 0; i < Desc.NumMips; ++i)
		//		{
		//			VkImageCopy& Region = Regions[i];
		//			Region.extent.width = FMath::Max(1, Desc.Extent.X >> i);
		//			Region.extent.height = FMath::Max(1, Desc.Extent.Y >> i);
		//			Region.extent.depth = FMath::Max(1, Desc.Depth >> i);
		//			Region.srcSubresource.aspectMask = FullAspectMask;
		//			Region.dstSubresource.aspectMask = FullAspectMask;
		//			Region.srcSubresource.baseArrayLayer = 0;
		//			Region.dstSubresource.baseArrayLayer = 0;
		//			Region.srcSubresource.layerCount = NumberOfArrayLevels;
		//			Region.dstSubresource.layerCount = NumberOfArrayLevels;
		//			Region.srcSubresource.mipLevel = i;
		//			Region.dstSubresource.mipLevel = i;
		//		}
		//		VulkanRHI::vkCmdCopyImage(VkCmdBuffer,
		//			Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		//			MovedImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		//			Desc.NumMips, &Regions[0]);
		//	}

		//	// Put the destination image in exactly the same layout the original image was
		//	{
		//		FVulkanPipelineBarrier Barrier;
		//		Barrier.AddImageLayoutTransition(MovedImage, FullAspectMask, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, *OriginalLayout);
		//		Barrier.Execute(VkCmdBuffer);
		//	}

		//	// Update the tracked layouts
		//	*MovedLayout = *OriginalLayout;
		//	OriginalLayout->Set(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, FVulkanPipelineBarrier::MakeSubresourceRange(FullAspectMask));
		//}

		//{
		//	check(Image != VK_NULL_HANDLE);
		//	InDevice.NotifyDeletedImage(Image, bRenderTarget);
		//	InDevice.GetDeferredDeletionQueue().EnqueueResource(VulkanRHI::FDeferredDeletionQueue2::EType::Image, Image);

		//	if (GVulkanLogDefrag)
		//	{
		//		FGenericPlatformMisc::LowLevelOutputDebugStringf(TEXT("** MOVE IMAGE %p -> %p\n"), Image, MovedImage);
		//	}
		//}

		//Image = MovedImage;
	}

	void VulkanTexture::DestroySurface()
	{
		//const bool bIsLocalOwner = (ImageOwnerType == EImageOwnerType::LocalOwner);
		//const bool bHasExternalOwner = (ImageOwnerType == EImageOwnerType::ExternalOwner);

		//if (CpuReadbackBuffer)
		//{
		//	Device->GetDeferredDeletionQueue().EnqueueResource(VulkanRHI::FDeferredDeletionQueue2::EType::Buffer, CpuReadbackBuffer->Buffer);
		//	Device->GetMemoryManager().FreeVulkanAllocation(Allocation);
		//	delete CpuReadbackBuffer;

		//}
		//else if (bIsLocalOwner || bHasExternalOwner)
		//{
		//	const bool bRenderTarget = EnumHasAnyFlags(GetDesc().Flags, TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable | TexCreate_ResolveTargetable);
		//	FRHICommandList& RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
		//	if (!IsInRenderingThread() || (RHICmdList.Bypass() || !IsRunningRHIInSeparateThread()))
		//	{
		//		Device->NotifyDeletedImage(Image, bRenderTarget);
		//	}
		//	else
		//	{
		//		check(IsInRenderingThread());
		//		new (RHICmdList.AllocCommand<FRHICommandOnDestroyImage>()) FRHICommandOnDestroyImage(Image, Device, bRenderTarget);
		//	}

		//	if (bIsLocalOwner)
		//	{
		//		// If we don't own the allocation, it's transient memory not included in stats
		//		if (Allocation.HasAllocation())
		//		{
		//			VulkanTextureDestroyed(Allocation.Size, GetViewType(), bRenderTarget);
		//		}

		//		if (Image != VK_NULL_HANDLE)
		//		{
		//			Device->GetDeferredDeletionQueue().EnqueueResource(VulkanRHI::FDeferredDeletionQueue2::EType::Image, Image);
		//			Device->GetMemoryManager().FreeVulkanAllocation(Allocation);
		//			Image = VK_NULL_HANDLE;
		//		}
		//	}

		//	ImageOwnerType = EImageOwnerType::None;
		//}
	}

	VkExternalMemoryHandleTypeFlagBits VulkanTexture::QuerySupportedExternalHandleType(
		VulkanDevice* InDevice,
		VkFormat Format,
		VkImageType ImageType,
		VkImageTiling Tiling,
		VkImageUsageFlags Usage,
		VkImageCreateFlags CreateFlags)
	{
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		// 按优先级尝试的 handle 类型列表
		const VkExternalMemoryHandleTypeFlagBits CandidateTypes[] = {
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT,
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT,
			VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT,
		};

		for (auto HandleType : CandidateTypes)
		{
			VkPhysicalDeviceExternalImageFormatInfo ExternalImageFormatInfo = {};
			ExternalImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO;
			ExternalImageFormatInfo.handleType = HandleType;

			VkPhysicalDeviceImageFormatInfo2 ImageFormatInfo = {};
			ImageFormatInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
			ImageFormatInfo.pNext = &ExternalImageFormatInfo;
			ImageFormatInfo.format = Format;
			ImageFormatInfo.type = ImageType;
			ImageFormatInfo.tiling = Tiling;
			ImageFormatInfo.usage = Usage;
			ImageFormatInfo.flags = CreateFlags;

			VkExternalImageFormatProperties ExternalImageFormatProps = {};
			ExternalImageFormatProps.sType = VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES;

			VkImageFormatProperties2 ImageFormatProps = {};
			ImageFormatProps.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
			ImageFormatProps.pNext = &ExternalImageFormatProps;

			VkResult Result = vkGetPhysicalDeviceImageFormatProperties2(
				InDevice->GetPhyDevice()->GetPhysicalDeviceHandle(),
				&ImageFormatInfo,
				&ImageFormatProps);

			if (Result == VK_SUCCESS)
			{
				const auto& ExtFeatures = ExternalImageFormatProps.externalMemoryProperties;
				if (ExtFeatures.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT)
				{
					LOG_INFO("VulkanTexture: Using external memory handle type: 0x{:x}", (uint32)HandleType);
					return HandleType;
				}
			}
		}

		LOG_ERROR("VulkanTexture: No exportable external memory handle type found");
		return (VkExternalMemoryHandleTypeFlagBits)0;
#else
		return VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif
	}

	void* VulkanTexture::GetSharedMemoryHandle() const
	{
		if (mSharedHandle != nullptr)
			return mSharedHandle;

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		if (mExternalHandleType == (VkExternalMemoryHandleTypeFlagBits)0)
		{
			LOG_ERROR("VulkanTexture::GetSharedMemoryHandle - No external handle type configured");
			return nullptr;
		}

		VkMemoryGetWin32HandleInfoKHR HandleInfo = {};
		HandleInfo.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
		HandleInfo.memory = mAllocation.GetDeviceMemoryHandle(mDevice);
		HandleInfo.handleType = mExternalHandleType;

		VkResult Result = vkGetMemoryWin32HandleKHR(mDevice->GetDevice(), &HandleInfo, &mSharedHandle);
		if (Result != VK_SUCCESS)
		{
			LOG_ERROR("VulkanTexture::GetSharedMemoryHandle - Failed to get Win32 handle, VkResult: {}", (int)Result);
			mSharedHandle = nullptr;
		}
#endif
		return mSharedHandle;
	}

	VulkanTexture::~VulkanTexture()
	{
		DestroyReadbackResources();

		// 关闭共享 HANDLE
		if (mSharedHandle != nullptr)
		{
			CloseHandle(mSharedHandle);
			mSharedHandle = nullptr;
		}

		// 总是销毁 View（由 VulkanTexture 创建）
		mDefaultView.Destroy(*mDevice);
		
		// 如果是独立分配的 mPartialView，也需要销毁
		if (mPartialView != nullptr && mPartialView != &mDefaultView)
		{
			mPartialView->Destroy(*mDevice);
			delete mPartialView;
			mPartialView = nullptr;
		}
		
		// 仅当拥有 VkImage 所有权时销毁图像和内存
		if (mbOwnsImage && mHandle != VK_NULL_HANDLE)
		{
			mDevice->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::EType::Image, mHandle);
			mDevice->GetMemoryManager()->FreeVulkanAllocation(mAllocation);
			mHandle = VK_NULL_HANDLE;
		}
	}

	void VulkanTexture::InitReadbackResources()
	{
		if (mCpuReadbackBuffer == nullptr)
		{
			mCpuReadbackBuffer = new VulkanCpuReadbackBuffer();
		}

		if (mCpuReadbackBuffer->bReady)
			return;

		uint32 Width = GetWidth();
		uint32 Height = GetHeight();
		uint32 BytesPerPixel = GPixelFormats[GetFormat()].BlockBytes;
		uint32 BufferSize = Width * Height * BytesPerPixel;

		// 通过 StagingBufferManager 创建 host-visible staging buffer
		VulkanStagingBuffer* StagingBuf = mDevice->GetStagingManager()->AcquireBuffer(
			BufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

		mCpuReadbackBuffer->Buffer = StagingBuf->GetHandle();
		//mCpuReadbackBuffer->Allocation = StagingBuf->mAllocation;
		mCpuReadbackBuffer->MappedPointer = StagingBuf->GetMappingPointer();
		mCpuReadbackBuffer->TotalSize = BufferSize;
		mCpuReadbackBuffer->MipOffsets[0] = 0;
		mCpuReadbackBuffer->MipSize[0] = BufferSize;
		mCpuReadbackBuffer->bReady = true;

		// 注意：StagingBuffer 对象的生命周期由 StagingManager 管理，
		// 但我们持有其 VkBuffer handle 和 Allocation。
		// 不要通过 ReleaseBuffer 释放它，因为我们需要持久持有。
	}

	void VulkanTexture::CopyToReadbackBuffer()
	{
		if (mCpuReadbackBuffer == nullptr || !mCpuReadbackBuffer->bReady)
			return;

		if (mHandle == VK_NULL_HANDLE || mCpuReadbackBuffer->Buffer == VK_NULL_HANDLE)
			return;

		// 获取当前活跃的 command buffer（渲染线程）
		VulkanCommandContext* VkCtx = static_cast<VulkanCommandContext*>(
			RenderSystem::instance()->GetRHICommandContext());
		VulkanCommandBuffer* CmdBuffer = VkCtx->GetCommandBufferManager()->GetActiveCmdBuffer();

		if (!CmdBuffer || !CmdBuffer->HasBegun())
			return;

		uint32 Width = GetWidth();
		uint32 Height = GetHeight();

		// 保存当前布局
		VkImageLayout OldLayout = GetImageLayout();

		// Transition to TRANSFER_SRC_OPTIMAL
		if (OldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			VkImageSubresourceRange SubRange = VulkanPipelineBarrier::MakeSubresourceRange(
				GetFullAspectMask(), 0, 1, 0, 1);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), this,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, SubRange);
		}

		// vkCmdCopyImageToBuffer
		VkBufferImageCopy Region = {};
		Region.bufferOffset = 0;
		Region.bufferRowLength = 0;   // tightly packed
		Region.bufferImageHeight = 0; // tightly packed
		Region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		Region.imageSubresource.mipLevel = 0;
		Region.imageSubresource.baseArrayLayer = 0;
		Region.imageSubresource.layerCount = 1;
		Region.imageOffset = { 0, 0, 0 };
		Region.imageExtent = { Width, Height, 1 };

		vkCmdCopyImageToBuffer(CmdBuffer->GetHandle(),
			mHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			mCpuReadbackBuffer->Buffer, 1, &Region);

		// Transition back to original layout
		if (OldLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			VkImageSubresourceRange SubRange = VulkanPipelineBarrier::MakeSubresourceRange(
				GetFullAspectMask(), 0, 1, 0, 1);
			VulkanSetImageLayout(CmdBuffer->GetHandle(), this,
				OldLayout, SubRange);
		}
	}

	bool VulkanTexture::ReadbackPixels(void* OutData, uint32& OutRowPitch)
	{
		if (mCpuReadbackBuffer == nullptr || !mCpuReadbackBuffer->bReady)
			return false;

		if (mCpuReadbackBuffer->MappedPointer == nullptr)
			return false;

		// Invalidate for CPU read (ensures GPU writes are visible)
		//mCpuReadbackBuffer->Allocation.InvalidateMappedMemory(mDevice);

		uint32 BytesPerPixel = GPixelFormats[GetFormat()].BlockBytes;
		OutRowPitch = GetWidth() * BytesPerPixel;
		Memory::MemoryCopy(OutData, mCpuReadbackBuffer->MappedPointer, mCpuReadbackBuffer->TotalSize);
		return true;
	}

	bool VulkanTexture::IsReadbackReady() const
	{
		return mCpuReadbackBuffer != nullptr && mCpuReadbackBuffer->bReady;
	}

	void VulkanTexture::DestroyReadbackResources()
	{
		if (mCpuReadbackBuffer != nullptr)
		{
			if (mCpuReadbackBuffer->Buffer != VK_NULL_HANDLE && mCpuReadbackBuffer->bReady)
			{
				// 注意：VkBuffer 和 Allocation 由 StagingBufferManager 创建，
				// 但我们没有释放回 manager。手动销毁。
				vkDestroyBuffer(mDevice->GetDevice(), mCpuReadbackBuffer->Buffer, VULKAN_CPU_ALLOCATOR);
				//mDevice->GetMemoryManager()->FreeVulkanAllocation(mCpuReadbackBuffer->Allocation);
				mCpuReadbackBuffer->Buffer = VK_NULL_HANDLE;
				mCpuReadbackBuffer->MappedPointer = nullptr;
				mCpuReadbackBuffer->bReady = false;
			}
			delete mCpuReadbackBuffer;
			mCpuReadbackBuffer = nullptr;
		}
	}

	void VulkanTexture::GenerateImageCreateInfo(VulkanImageCreateInfo& OutInfo, VulkanDevice* InDevice, const RHITextureDesc& InDesc, VkFormat* OutStorageFormat, VkFormat* OutViewFormat, bool bForceLinearTexture)
	{
		const VkPhysicalDeviceProperties& DeviceProps = InDevice->GetPhyDevice()->GetDeviceProperties();
		//const PixelFormatInfo& FormatInfo = 
		VkFormat TextureFormat = EngineToVkTextureFormat(InDesc.mFormat, InDesc.isSRGB);
		const TextureCreateFlags Flags = InDesc.mFlags;
		if (EnumHasAnyFlags(Flags, TextureCreateFlags::CPUReadback))
		{
			bForceLinearTexture = true;
		}

		VkImageCreateInfo& ImageCreateInfo = OutInfo.mImageCreateInfo;
		Memory::MemoryZero(ImageCreateInfo);
		ImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		const VkImageViewType ResourceType = TextureDimensionToVkImageViewType(InDesc.mDimension);

		switch (ResourceType)
		{
		case VK_IMAGE_VIEW_TYPE_1D:
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_1D;
			break;
		case VK_IMAGE_VIEW_TYPE_CUBE:
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case VK_IMAGE_VIEW_TYPE_2D:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case VK_IMAGE_VIEW_TYPE_3D:
			ImageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
			break;
		default:
			break;
		}
		VkFormat srgbFormat = EngineToVkTextureFormat(InDesc.mFormat, !EnumHasAllFlags(Flags, TextureCreateFlags::UAV));
		VkFormat nonSrgbFormat = EngineToVkTextureFormat(InDesc.mFormat, false);
		ImageCreateInfo.format = EnumHasAnyFlags(Flags, TextureCreateFlags::UAV) ? nonSrgbFormat : srgbFormat;
		if (OutViewFormat)
		{
			*OutViewFormat = srgbFormat;
			if (EnumHasAnyFlags(Flags, TextureCreateFlags::External))
			{
				*OutViewFormat = nonSrgbFormat;
			}
		}
		if (OutStorageFormat)
		{
			*OutStorageFormat = nonSrgbFormat;
		}

		ImageCreateInfo.extent.width = InDesc.mExtent.x;
		ImageCreateInfo.extent.height = InDesc.mExtent.y;
		ImageCreateInfo.extent.depth = ResourceType == VK_IMAGE_VIEW_TYPE_3D ? InDesc.mDepth : 1;
		ImageCreateInfo.mipLevels = InDesc.mNumMips;
		uint32 LayerCount = (ResourceType == VK_IMAGE_VIEW_TYPE_CUBE || ResourceType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) ? 6 : 1;
		ImageCreateInfo.arrayLayers = InDesc.mArraySize * LayerCount;
		ImageCreateInfo.flags = (ResourceType == VK_IMAGE_VIEW_TYPE_CUBE || ResourceType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

		if (EnumHasAllFlags(Flags, TextureCreateFlags::SRGB))
		{
			if (InDevice->GetOptionalExtensions().HasKHRImageFormatList)
			{
				VkImageFormatListCreateInfoKHR& ImageFormatListCreateInfo = OutInfo.mImageFormatListCreateInfo;
				Memory::MemoryZero(ImageFormatListCreateInfo);

				ImageFormatListCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
				ImageFormatListCreateInfo.pNext = ImageCreateInfo.pNext;
				ImageCreateInfo.pNext = &ImageFormatListCreateInfo;
				ImageFormatListCreateInfo.viewFormatCount = 2;
				ImageFormatListCreateInfo.pViewFormats = OutInfo.mFormatsUsed;
				OutInfo.mFormatsUsed[0] = nonSrgbFormat;
				OutInfo.mFormatsUsed[1] = srgbFormat;
			}

			ImageCreateInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}
		if (InDevice->GetOptionalExtensions().HasKHRMaintenance1 && ImageCreateInfo.imageType == VK_IMAGE_TYPE_3D)
		{
			ImageCreateInfo.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT_KHR;
		}

		ImageCreateInfo.tiling = bForceLinearTexture ? VK_IMAGE_TILING_LINEAR : GVulkanViewTypeTilingMode[ResourceType];

		ImageCreateInfo.usage = 0;
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		//@TODO: should everything be created with the source bit?
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		ImageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

		if (EnumHasAnyFlags(Flags, TextureCreateFlags::Presentable))
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}
		else if (EnumHasAnyFlags(Flags, (TextureCreateFlags)((uint64)TextureCreateFlags::RenderTargetable | (uint64)TextureCreateFlags::DepthStencilTargetable)))
		{
			if (EnumHasAllFlags(Flags, TextureCreateFlags::InputAttachmentRead))
			{
				ImageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
			}
			ImageCreateInfo.usage |= (EnumHasAnyFlags(Flags, TextureCreateFlags::RenderTargetable) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			if (EnumHasAllFlags(Flags, TextureCreateFlags::Memoryless) && InDevice->GetDeviceMemoryManager()->SupportsMemoryless())
			{
				ImageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
				// Remove the transfer and sampled bits, as they are incompatible with the transient bit.
				ImageCreateInfo.usage &= ~(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
			}
		}
		else if (EnumHasAnyFlags(Flags, TextureCreateFlags::DepthStencilResolveTarget))
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		}
		else if (EnumHasAnyFlags(Flags, TextureCreateFlags::ResolveTargetable))
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			ImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		}

		if (EnumHasAnyFlags(Flags, TextureCreateFlags::UAV))
		{
			ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
		}

#if VULKAN_SUPPORTS_EXTERNAL_MEMORY
		if (EnumHasAnyFlags(Flags, TextureCreateFlags::External))
		{
			// 先查询驱动支持的 handle 类型（用于设置 ImageCreateInfo）
			VkExternalMemoryHandleTypeFlagBits SupportedHandleType = QuerySupportedExternalHandleType(
				InDevice,
				nonSrgbFormat,
				ImageCreateInfo.imageType,
				ImageCreateInfo.tiling,
				ImageCreateInfo.usage,
				ImageCreateInfo.flags);

			VkExternalMemoryImageCreateInfoKHR& ExternalMemImageCreateInfo = OutInfo.mExternalMemImageCreateInfo;
			Memory::MemoryZero(ExternalMemImageCreateInfo);
			ExternalMemImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO_KHR;
			ExternalMemImageCreateInfo.handleTypes = SupportedHandleType;
			ExternalMemImageCreateInfo.pNext = ImageCreateInfo.pNext;
			ImageCreateInfo.pNext = &ExternalMemImageCreateInfo;
			ImageCreateInfo.format = nonSrgbFormat;
		}
#endif // VULKAN_SUPPORTS_EXTERNAL_MEMORY

		ImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		ImageCreateInfo.queueFamilyIndexCount = 0;
		ImageCreateInfo.pQueueFamilyIndices = nullptr;

		uint8 NumSamples = InDesc.mNumSamples;
		if (ImageCreateInfo.tiling == VK_IMAGE_TILING_LINEAR && NumSamples > 1)
		{
			NumSamples = 1;
		}

		switch (NumSamples)
		{
		case 1:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			break;
		case 2:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_2_BIT;
			break;
		case 4:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_4_BIT;
			break;
		case 8:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_8_BIT;
			break;
		case 16:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_16_BIT;
			break;
		case 32:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_32_BIT;
			break;
		case 64:
			ImageCreateInfo.samples = VK_SAMPLE_COUNT_64_BIT;
			break;
		default:
			break;
		}

		const VkFormatFeatureFlags FormatFlags = ImageCreateInfo.tiling == VK_IMAGE_TILING_LINEAR ?
			InDevice->GetFormatProperties()[ImageCreateInfo.format].linearTilingFeatures :
			InDevice->GetFormatProperties()[ImageCreateInfo.format].optimalTilingFeatures;

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
		{
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_SAMPLED_BIT;
		}

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
		{
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
		}

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
		{
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
		{
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT))
		{
			// this flag is used unconditionally, strip it without warnings 
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}

		if (!VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
		{
			// this flag is used unconditionally, strip it without warnings 
			ImageCreateInfo.usage &= ~VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		if (EnumHasAnyFlags(Flags, TextureCreateFlags::DepthStencilTargetable))
		{
			if (VKHasAnyFlags(FormatFlags, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
			{
				ImageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
			}
		}

		ImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	}

	void VulkanTextureView::Create(VulkanDevice& Device, VkImage InImage, VkImageViewType ViewType, VkImageAspectFlags AspectFlags,
		PixelFormat InFormat, VkFormat Format, uint32 FirstMip, uint32 NumMips, uint32 ArraySliceIndex,
		uint32 NumArraySlices, bool bUseIdentitySwizzle, VkImageUsageFlags ImageUsageFlags)
	{
		mView = VK_NULL_HANDLE;

		VkImageViewCreateInfo ViewInfo;
		Memory::MemoryZero(ViewInfo);
		ViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		ViewInfo.image = InImage;
		ViewInfo.viewType = ViewType;
		ViewInfo.format = Format;
		VkImageViewASTCDecodeModeEXT DecodeMode;
		if (Device.GetOptionalExtensions().HasEXTASTCDecodeMode && IsAstcLdrFormat(Format) && !IsAstcSrgbFormat(Format))
		{
			Memory::MemoryZero(DecodeMode);
			DecodeMode.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_ASTC_DECODE_MODE_EXT;
			DecodeMode.decodeMode = VK_FORMAT_R8G8B8A8_UNORM;
			DecodeMode.pNext = ViewInfo.pNext;
			ViewInfo.pNext = &DecodeMode;
		}

		if (bUseIdentitySwizzle)
		{
			ViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			ViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		}
		else
		{
			//todo
			//ViewInfo.components = Device.GetFormatComponentMapping(Format);
		}
		ViewInfo.subresourceRange.aspectMask = AspectFlags;
		ViewInfo.subresourceRange.baseMipLevel = FirstMip;
		ViewInfo.subresourceRange.levelCount = NumMips;
		ViewInfo.subresourceRange.baseArrayLayer = ArraySliceIndex;
		switch (ViewType)
		{
		case VK_IMAGE_VIEW_TYPE_3D:
			ViewInfo.subresourceRange.layerCount = 1;
			break;
		case VK_IMAGE_VIEW_TYPE_CUBE:
			ViewInfo.subresourceRange.layerCount = 6;
			break;
		case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
			ViewInfo.subresourceRange.layerCount = 6 * NumArraySlices;
			break;
		case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
		case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
			ViewInfo.subresourceRange.layerCount = NumArraySlices;
			break;
		default:
			ViewInfo.subresourceRange.layerCount = 1;
			break;
		}

		if (InFormat == PF_X24_G8)
		{
			ViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		}

		VkImageViewUsageCreateInfo ImageViewUsageCreateInfo;
		if ((ImageUsageFlags != 0) && Device.GetOptionalExtensions().HasKHRMaintenance2)
		{
			Memory::MemoryZero(ImageViewUsageCreateInfo);
			ImageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
			ImageViewUsageCreateInfo.usage = ImageUsageFlags;

			ImageViewUsageCreateInfo.pNext = (void*)ViewInfo.pNext;
			ViewInfo.pNext = &ImageViewUsageCreateInfo;
		}
		vkCreateImageView(Device.GetDevice(), &ViewInfo, VULKAN_CPU_ALLOCATOR, &mView);
		mImage = InImage;
	}

	void VulkanTextureView::Destroy(VulkanDevice& Device)
	{
		if (mView)
		{
			Device.GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::EType::ImageView, mView);
			mImage = VK_NULL_HANDLE;
			mView = VK_NULL_HANDLE;
			mViewId = 0;
		}
	}
}
