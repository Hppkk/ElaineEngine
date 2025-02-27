#pragma once
#include "ElaineCorePrerequirements.h"
#include "vulkan_core.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "vulkan/ElaineVulkanCommandBuffer.h"
#include "render/common/ElaineRHIProtocol.h"

namespace VulkanRHI
{

	class VulkanQueue;
	class VulkanPhysicalDevice;
	class VulkanInstance;
	class VulkanShaderManager;
	class VulkanPiplineManager;

	struct OptionalVulkanDeviceExtensions
	{
		union
		{
			struct
			{
				// Optional Extensions
				uint64 HasEXTValidationCache : 1;
				uint64 HasMemoryPriority : 1;
				uint64 HasMemoryBudget : 1;
				uint64 HasEXTASTCDecodeMode : 1;
				uint64 HasEXTFragmentDensityMap : 1;
				uint64 HasEXTFragmentDensityMap2 : 1;
				uint64 HasKHRFragmentShadingRate : 1;
				uint64 HasEXTFullscreenExclusive : 1;
				uint64 HasImageAtomicInt64 : 1;
				uint64 HasAccelerationStructure : 1;
				uint64 HasRayTracingPipeline : 1;
				uint64 HasRayQuery : 1;
				uint64 HasDeferredHostOperations : 1;

				// Vendor specific
				uint64 HasAMDBufferMarker : 1;
				uint64 HasNVDiagnosticCheckpoints : 1;
				uint64 HasNVDeviceDiagnosticConfig : 1;
				uint64 HasQcomRenderPassTransform : 1;

				// Promoted to 1.1
				uint64 HasKHRMaintenance1 : 1;
				uint64 HasKHRMaintenance2 : 1;
				uint64 HasKHRDedicatedAllocation : 1;
				uint64 HasKHRMultiview : 1;

				// Promoted to 1.2
				uint64 HasKHRRenderPass2 : 1;
				uint64 HasKHRImageFormatList : 1;
				uint64 HasKHRShaderAtomicInt64 : 1;
				uint64 HasEXTScalarBlockLayout : 1;
				uint64 HasBufferDeviceAddress : 1;
				uint64 HasSPIRV_14 : 1;
				uint64 HasShaderFloatControls : 1;
				uint64 HasEXTDescriptorIndexing : 1;
				uint64 HasEXTShaderViewportIndexLayer : 1;
				uint64 HasSeparateDepthStencilLayouts : 1;
				uint64 HasEXTHostQueryReset : 1;

				// Promoted to 1.3
				uint64 HasEXTTextureCompressionASTCHDR : 1;
				uint64 HasKHRMaintenance4 : 1;
			};
			uint64 Packed;
		};
	};

	class ElaineCoreExport DeferredDeletionQueue
	{
	public:
		DeferredDeletionQueue(VulkanDevice* InDevice);
		~DeferredDeletionQueue();

		enum class EType
		{
			RenderPass,
			Buffer,
			BufferView,
			Image,
			ImageView,
			Pipeline,
			PipelineLayout,
			Framebuffer,
			DescriptorSetLayout,
			Sampler,
			Semaphore,
			ShaderModule,
			Event,
			ResourceAllocation,
			DeviceMemoryAllocation,
			BufferSuballocation,
			AccelerationStructure,
		};

		template <typename T>
		inline void EnqueueResource(EType Type, T Handle)
		{
			static_assert(sizeof(T) <= sizeof(uint64), "Vulkan resource handle type size too large.");
			EnqueueGenericResource(Type, (uint64)Handle);
		}

		void EnqueueResourceAllocation(VulkanAllocation& Allocation);
		void EnqueueDeviceAllocation(VulkanDeviceMemoryAllocation* DeviceMemoryAllocation);

		void ReleaseResources(bool bDeleteImmediately = false);

		inline void Clear()
		{
			ReleaseResources(true);
		}

		void OnCmdBufferDeleted(VulkanCommandBuffer* CmdBuffer);
	private:
		void EnqueueGenericResource(EType Type, uint64 Handle);

		struct FEntry
		{
			EType StructureType;
			uint32 FrameNumber;
			uint64 FenceCounter;
			VulkanCommandBuffer* CmdBuffer;

			uint64 Handle;
			VulkanAllocation Allocation;
			VulkanDeviceMemoryAllocation* DeviceMemoryAllocation;
			bool operator<(const FEntry& InEntry) const
			{
				return Handle < InEntry.Handle;
			}
		};

	private:
		VulkanDevice*			mDevice;
		std::recursive_mutex	mMtx;
		std::set<FEntry>		mEntries;

	};


	class ElaineCoreExport VulkanDevice
	{
	public:
		VulkanDevice(VulkanPhysicalDevice* InPhyDevice);
		~VulkanDevice();
		void Initilize();

		VkDevice	GetDevice() const
		{
			return mHandle;
		}

		VulkanQueue* GetComputeQueue() const
		{
			return mComputeQueue;
		}

		VulkanQueue* GetTransferQueue() const
		{
			return mTransferQueue;
		}

		VulkanQueue* GetPresentQueue() const
		{
			return mPresentQueue;
		}

		VulkanQueue* GetGraphicQueue() const
		{
			return mGraphicQueue;
		}

		VulkanFenceManager* GetFenceManager() const
		{
			return mFenceMgr;
		}

		DeferredDeletionQueue& GetDeferredDeletionQueue()
		{
			return mDefDelQueue;
		}

		inline VulkanStagingBufferManager* GetStagingManager() const
		{
			return mStagingMgr;
		}

		inline GpuVendorId GetVendorId() const
		{
			return mVendorId;
		}

		inline VulkanPhysicalDevice* GetPhyDevice() const
		{
			return mVulkanPhyDevice;
		}

		inline VulkanMemoryManager* GetMemoryManager() const
		{
			return mMemoryManager;
		}

		inline VulkanDeviceMemoryManager* GetDeviceMemoryManager() const
		{
			return mDeviceMemoryMgr;
		}

		inline const OptionalVulkanDeviceExtensions& GetOptionalExtensions() const
		{
			return mOptionalDeviceExtensions;
		}

		inline const VkFormatProperties* GetFormatProperties() const
		{
			return mFormatProperties;
		}

		inline VulkanPiplineManager* GetPiplineManager() const
		{
			return mPipelineManager;
		}

		inline VulkanShaderManager* GetShaderManager() { return mShaderManager; }

	private:
		VkDevice						mHandle;
		VulkanQueue*					mComputeQueue;
		VulkanQueue*					mTransferQueue;
		VulkanQueue*					mPresentQueue;
		VulkanQueue*					mGraphicQueue;
		VulkanPhysicalDevice*			mVulkanPhyDevice;
		VulkanFenceManager*				mFenceMgr;
		DeferredDeletionQueue			mDefDelQueue;
		VulkanStagingBufferManager*		mStagingMgr;
		VulkanMemoryManager*			mMemoryManager;
		VulkanDeviceMemoryManager*		mDeviceMemoryMgr;
		VulkanShaderManager*			mShaderManager = nullptr;
		VulkanPiplineManager*			mPipelineManager = nullptr;
		GpuVendorId						mVendorId = GpuVendorId::NotQueried;
		OptionalVulkanDeviceExtensions	mOptionalDeviceExtensions;
		VkFormatProperties				mFormatProperties[VK_FORMAT_ASTC_12x12_SRGB_BLOCK - VK_FORMAT_UNDEFINED + 1];
		std::vector<const char*>		mDeviceExtensions;
		uint64							mTimestampValidBitsMask;
		bool							mbAsyncComputeQueue = false;
		bool							mbPresentOnComputeQueue = false;

	};
}