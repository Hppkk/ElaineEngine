#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanRHI.h"
#include "render/vulkan/ElaineVulkanInstance.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanQueue.h"
#include "render/vulkan/ElaineVulkanShader.h"
#include "render/vulkan/ElaineVulkanPipeline.h"
#include "render/vulkan/ElaineVulkanCommandBuffer.h"
#include "render/vulkan/ElaineVulkanRenderPassCache.h"

namespace VulkanRHI
{
	// This 'frame number' should only be used for the deletion queue
	uint32 GVulkanRHIDeletionFrameNumber = 0;
	const uint32 NUM_FRAMES_TO_WAIT_FOR_RESOURCE_DELETE = 2;

	DeferredDeletionQueue::DeferredDeletionQueue(VulkanDevice* InDevice)
		: mDevice(InDevice)
	{

	}

	DeferredDeletionQueue::~DeferredDeletionQueue()
	{

	}

	void DeferredDeletionQueue::EnqueueResourceAllocation(VulkanAllocation& Allocation)
    {
        std::lock_guard<std::recursive_mutex> Lock(mMtx);
        FEntry Entry{}; Entry.StructureType = EType::ResourceAllocation; Entry.Allocation = Allocation;
        mDevice->GetGraphicQueue()->GetLastSubmittedInfo(Entry.CmdBuffer, Entry.FenceCounter);
        mEntries.push_back(Entry);
    }

	void DeferredDeletionQueue::EnqueueDeviceAllocation(VulkanDeviceMemoryAllocation* DeviceMemoryAllocation)
    {
        std::lock_guard<std::recursive_mutex> Lock(mMtx);
        FEntry Entry{}; Entry.StructureType = EType::DeviceMemoryAllocation; Entry.DeviceMemoryAllocation = DeviceMemoryAllocation;
        mDevice->GetGraphicQueue()->GetLastSubmittedInfo(Entry.CmdBuffer, Entry.FenceCounter);
        mEntries.push_back(Entry);
    }

	void DeferredDeletionQueue::ReleaseResources(bool bDeleteImmediately)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		VkDevice DeviceHandle = mDevice->GetDevice();

        do {
		std::vector<FEntry> Batch;
        Batch.swap(mEntries);
        for (const auto& enter : Batch)
		{
            if (!bDeleteImmediately && enter.CmdBuffer) enter.CmdBuffer->RefreshFenceStatus();
			if (bDeleteImmediately ||
				(!enter.CmdBuffer || enter.CmdBuffer->GetCompletedFenceCounter() > enter.FenceCounter)
				)
			{
				switch (enter.StructureType)
				{
#define VKSWITCH(Type, ...)	case EType::Type: __VA_ARGS__; vkDestroy##Type(DeviceHandle, (Vk##Type)enter.Handle, VULKAN_CPU_ALLOCATOR); break
					VKSWITCH(RenderPass);
					VKSWITCH(Buffer);
					VKSWITCH(BufferView);
					VKSWITCH(Image);
					VKSWITCH(ImageView);
					VKSWITCH(Pipeline);
					VKSWITCH(PipelineLayout);
					VKSWITCH(Framebuffer);
					VKSWITCH(DescriptorSetLayout);
					VKSWITCH(Sampler);
					VKSWITCH(Semaphore);
					VKSWITCH(ShaderModule);
					VKSWITCH(Event);
#undef VKSWITCH
				case EType::ResourceAllocation:
				{
					VulkanAllocation Allocation = enter.Allocation;
					Allocation.Own();
					mDevice->GetMemoryManager()->FreeVulkanAllocation(Allocation, VulkanFreeFlag_DontDefer);
					break;
				}
				case EType::DeviceMemoryAllocation:
				{
					VulkanDeviceMemoryAllocation* CurrentAllocation = enter.DeviceMemoryAllocation;
					mDevice->GetDeviceMemoryManager()->Free(CurrentAllocation);
					break;
				}
				case EType::AccelerationStructure:
				{
					//vkDestroyAccelerationStructureKHR(DeviceHandle, (VkAccelerationStructureKHR)enter.Handle, VULKAN_CPU_ALLOCATOR);
					break;
				}
				default:
					break;
				}

			}
            else mEntries.push_back(enter);
		}
        } while (bDeleteImmediately && !mEntries.empty());
	}
	

	void DeferredDeletionQueue::OnCmdBufferDeleted(VulkanCommandBuffer* CmdBuffer)
    {
        // Command pools are destroyed only after waiting for the device.
        std::lock_guard<std::recursive_mutex> Lock(mMtx);
        for (auto& Entry : mEntries) if (Entry.CmdBuffer == CmdBuffer) Entry.CmdBuffer = nullptr;
    }

	void DeferredDeletionQueue::EnqueueGenericResource(EType Type, uint64 Handle)
	{
		VulkanQueue* Queue = mDevice->GetGraphicQueue();
		FEntry Entry;
		Entry.StructureType = Type;
		Queue->GetLastSubmittedInfo(Entry.CmdBuffer, Entry.FenceCounter);
		Entry.FrameNumber = GVulkanRHIDeletionFrameNumber;
		Entry.DeviceMemoryAllocation = 0;
		Entry.Handle = Handle;

		{
			std::lock_guard<std::recursive_mutex> lock(mMtx);
			mEntries.push_back(Entry);
		}
	}

	VulkanDevice::VulkanDevice(VulkanPhysicalDevice* InPhyDevice)
		: mHandle(nullptr)
		, mComputeQueue(nullptr)
		, mTransferQueue(nullptr)
		, mPresentQueue(nullptr)
		, mGraphicQueue(nullptr)
		, mVulkanPhyDevice(InPhyDevice)
		, mFenceMgr(nullptr)
		, mDefDelQueue(this)
		, mStagingMgr(nullptr)
		, mMemoryManager(nullptr)
		, mDeviceMemoryMgr(nullptr)
		, mTimestampValidBitsMask(0u)
	{
		
	}

    void VulkanDevice::InvalidateImageView(VkImageView View)
    { for (auto* Cache : mFramebufferCaches) Cache->InvalidateView(View); }
    VulkanDevice::~VulkanDevice()
    {
        vkDeviceWaitIdle(mHandle);
        SAFE_DELETE(mPipelineManager);
        SAFE_DELETE(mShaderManager);
        mStagingMgr->DeInit(); SAFE_DELETE(mStagingMgr);
        mDefDelQueue.ReleaseResources(true);
        mMemoryManager->Deinit();
        mDefDelQueue.ReleaseResources(true);
        SAFE_DELETE(mMemoryManager);
        mDeviceMemoryMgr->Deinit(); SAFE_DELETE(mDeviceMemoryMgr);
        mFenceMgr->DeInit(); SAFE_DELETE(mFenceMgr);
        std::set<VulkanQueue*> Queues{mGraphicQueue, mComputeQueue, mTransferQueue, mPresentQueue};
        for (auto* Queue : Queues) delete Queue;
        vkDestroyDevice(mHandle, VULKAN_CPU_ALLOCATOR); mHandle = VK_NULL_HANDLE;
    }

	void VulkanDevice::Initialize()
	{
		for (uint32 Index = 0; Index < VK_FORMAT_ASTC_12x12_SRGB_BLOCK - VK_FORMAT_UNDEFINED + 1; ++Index)
		{
			const VkFormat Format = (VkFormat)Index;
			Memory::MemoryZero(mFormatProperties[Index]);
			vkGetPhysicalDeviceFormatProperties(GetPhyDevice()->GetPhysicalDeviceHandle(), Format, &mFormatProperties[Index]);
		}

		mVendorId = RHIConvertToGpuVendorId(mVulkanPhyDevice->GetDeviceProperties().vendorID);

		VkDeviceCreateInfo DeviceCreateInfo;
		Memory::MemoryZero(DeviceCreateInfo);
		DeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		for (auto& Extension : mVulkanPhyDevice->GetDeviceExtensions())
		{
			std::string ExtName(Extension.extensionName);
			if (ExtName == VK_KHR_SWAPCHAIN_EXTENSION_NAME)
				mDeviceExtensions.push_back(Extension.extensionName);
			else if (ExtName == VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME)
				mDeviceExtensions.push_back(Extension.extensionName);
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
			else if (ExtName == VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME)
				mDeviceExtensions.push_back(Extension.extensionName);
#endif
		}

		LOG_INFO("VulkanDevice: Enabled Device Extensions:");
		for (const auto& extension : mDeviceExtensions)
		{
			LOG_INFO("  - {}", extension);
		}

		DeviceCreateInfo.enabledExtensionCount = mDeviceExtensions.size();
		DeviceCreateInfo.ppEnabledExtensionNames = mDeviceExtensions.data();

		//don't care, vulkan the current version has been moved the validation layer to the when the instance creating. 
		if (Root::instance()->IsEnableVulkanValidationLayer())
		{
			DeviceCreateInfo.enabledLayerCount = mVulkanPhyDevice->GetVulkanInstance()->GetLayers().size();
			DeviceCreateInfo.ppEnabledLayerNames = mVulkanPhyDevice->GetVulkanInstance()->GetLayers().data();
		}

		std::vector<VkDeviceQueueCreateInfo> QueueFamilyInfos;
		int32 GfxQueueFamilyIndex = -1;
		int32 ComputeQueueFamilyIndex = -1;
		int32 TransferQueueFamilyIndex = -1;

		uint32 NumPriorities = 0;

		auto& TmpQueueFamilyProps = mVulkanPhyDevice->GetQueueFamilyProps();

		for (int32 FamilyIndex = 0; FamilyIndex < TmpQueueFamilyProps.size(); ++FamilyIndex)
		{
			const VkQueueFamilyProperties& CurrProps = TmpQueueFamilyProps[FamilyIndex];

			bool bIsValidQueue = false;
			if ((CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)
			{
				if (GfxQueueFamilyIndex == -1)
				{
					GfxQueueFamilyIndex = FamilyIndex;
					bIsValidQueue = true;
				}
			}

			if ((CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) == VK_QUEUE_COMPUTE_BIT)
			{
				if (ComputeQueueFamilyIndex == -1 && GfxQueueFamilyIndex != FamilyIndex)
				{
					ComputeQueueFamilyIndex = FamilyIndex;
					bIsValidQueue = true;
				}
			}

			if ((CurrProps.queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT)
			{
				if (TransferQueueFamilyIndex == -1 && (CurrProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) != VK_QUEUE_GRAPHICS_BIT && (CurrProps.queueFlags & VK_QUEUE_COMPUTE_BIT) != VK_QUEUE_COMPUTE_BIT)
				{
					TransferQueueFamilyIndex = FamilyIndex;
					bIsValidQueue = true;
				}
			}

			uint32 QueueIndex = QueueFamilyInfos.size();
			QueueFamilyInfos.push_back({});
			VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[QueueIndex];
			CurrQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			CurrQueue.queueFamilyIndex = FamilyIndex;
			CurrQueue.queueCount = CurrProps.queueCount;
			NumPriorities += CurrProps.queueCount;
		}

		std::vector<float> QueuePriorities(NumPriorities);
		float* CurrentPriority = QueuePriorities.data();
		for (int32 Index = 0; Index < QueueFamilyInfos.size(); ++Index)
		{
			VkDeviceQueueCreateInfo& CurrQueue = QueueFamilyInfos[Index];
			CurrQueue.pQueuePriorities = CurrentPriority;

			const VkQueueFamilyProperties& CurrProps = TmpQueueFamilyProps[CurrQueue.queueFamilyIndex];
			for (int32 QueueIndex = 0; QueueIndex < (int32)CurrProps.queueCount; ++QueueIndex)
			{
				*CurrentPriority++ = 1.0f;
			}
		}

		DeviceCreateInfo.queueCreateInfoCount = QueueFamilyInfos.size();
		DeviceCreateInfo.pQueueCreateInfos = QueueFamilyInfos.data();

		DeviceCreateInfo.pEnabledFeatures = &mVulkanPhyDevice->GetPhysicalFeatures();

		VkResult Result = vkCreateDevice(mVulkanPhyDevice->GetPhysicalDeviceHandle(), &DeviceCreateInfo, VULKAN_CPU_ALLOCATOR, &mHandle);
		if (Result != VK_SUCCESS)
		{
			LOG_ERROR("VulkanRHI: Cannot create a Vulkan device. Error Code: {}", Result);
		}
#ifdef USE_VOLK
		volkLoadDevice(mHandle);
#endif

		mGraphicQueue = new VulkanQueue(this, GfxQueueFamilyIndex);
		mGraphicQueue->Initialize();
		if (ComputeQueueFamilyIndex == -1)
		{
			// If we didn't find a dedicated Queue, use the default one
			ComputeQueueFamilyIndex = GfxQueueFamilyIndex;
			mComputeQueue = mGraphicQueue;
		}
		else
		{
			// Dedicated queue// async
			//if ()
			//{
			//	bAsyncComputeQueue = true;
			//}
			mComputeQueue = new VulkanQueue(this, ComputeQueueFamilyIndex);
			mComputeQueue->Initialize();
		}
		
		if (TransferQueueFamilyIndex == -1)
		{
			// If we didn't find a dedicated Queue, use the default one
			TransferQueueFamilyIndex = ComputeQueueFamilyIndex;
			mTransferQueue = mComputeQueue;
		}
		else
		{
			mTransferQueue = new VulkanQueue(this, TransferQueueFamilyIndex);
			mTransferQueue->Initialize();
		}

		uint64 NumBits = TmpQueueFamilyProps[GfxQueueFamilyIndex].timestampValidBits;
		if (NumBits > 0)
		{

			if (NumBits == 64)
			{
				// Undefined behavior trying to 1 << 64 on uint64
				mTimestampValidBitsMask = UINT64_MAX;
			}
			else
			{
				mTimestampValidBitsMask = ((uint64)1 << (uint64)NumBits) - (uint64)1;
			}
		}

		mFenceMgr = new VulkanFenceManager();
		mFenceMgr->Initialize(this);
		mDeviceMemoryMgr = new VulkanDeviceMemoryManager();
		mDeviceMemoryMgr->Init(this);
		mMemoryManager = new VulkanMemoryManager(this);
		mMemoryManager->Init();
		mStagingMgr = new VulkanStagingBufferManager();
		mStagingMgr->Init(this);
		mShaderManager = new VulkanShaderManager(this);
		mPipelineManager = new VulkanPiplineManager(this);

	}
}