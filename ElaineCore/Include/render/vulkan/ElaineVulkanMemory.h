#pragma once

namespace VulkanRHI
{

#define VULKAN_CPU_ALLOCATOR  nullptr

	class VulkanAllocation;
	class VulkanTexture;
	class VulkanCommandContext;
	class VulkanFenceManager;
	class VulkanCommandBuffer;

	enum VulkanAllocationType :uint8
	{
		VulkanAllocationTypeEmpty,
		VulkanAllocationPooledBuffer,
		VulkanAllocationBuffer,
		VulkanAllocationImage,
		VulkanAllocationImageDedicated,
		VulkanAllocationSize, // keep last
	};

	enum class VulkanMemoryType
	{
		Image,
		Buffer,
	};

	enum VulkanFreeFlags
	{
		VulkanFreeFlag_None = 0x0,
		VulkanFreeFlag_DontDefer = 0x1,
	};

	enum VulkanAllocationMetaType : uint8
	{
		VulkanAllocationMetaUnknown,
		VulkanAllocationMetaUniformBuffer,
		VulkanAllocationMetaMultiBuffer,
		VulkanAllocationMetaRingBuffer,
		VulkanAllocationMetaFrameTempBuffer,
		VulkanAllocationMetaImageRenderTarget,
		VulkanAllocationMetaImageOther,
		VulkanAllocationMetaBufferUAV,
		VulkanAllocationMetaBufferStaging,
		VulkanAllocationMetaBufferOther,
		VulkanAllocationMetaSize, // keep last
	};

	enum VulkanAllocationFlags
	{
		VulkanAllocationFlagsMapped = 0x1,
		VulkanAllocationFlagsCanEvict = 0x2,
	};

	class VulkanEvictable
	{
	public:
		virtual bool CanMove() const { return false; }
		virtual bool CanEvict() const { return false; }
		virtual void Evict(VulkanDevice& Device, VulkanCommandContext& Context) { ; }
		virtual void Move(VulkanDevice& Device, VulkanCommandContext& Context, VulkanAllocation& Allocation) { ; }
		virtual VulkanTexture* GetEvictableTexture() { return nullptr; }
	};

	struct Range
	{
		uint32	mOffset;
		uint32	mSize;

		inline bool operator<(const Range& In) const
		{
			return mOffset < In.mOffset;
		}

		static void JoinConsecutiveRanges(std::vector<Range>& Ranges);

		/** Tries to insert the item so it has index ProposedIndex, but may end up merging it with neighbors */
		static int32 InsertAndTryToMerge(std::vector<Range>& Ranges, const Range& Item, int32 ProposedIndex);

		/** Tries to append the item to the end but may end up merging it with the neighbor */
		static int32 AppendAndTryToMerge(std::vector<Range>& Ranges, const Range& Item);

		/** Attempts to allocate from an entry - can remove it if it was used up*/
		static void AllocateFromEntry(std::vector<Range>& Ranges, int32 Index, uint32 SizeToAllocate);

		/** Sanity checks an array of ranges */
		static void SanityCheck(std::vector<Range>& Ranges);

		/** Adds to the array while maintaing the sort. */
		static int32 Add(std::vector<Range>& Ranges, const Range& Item);
	};

	struct VulkanPageSizeBucket
	{
		uint64		mAllocationMax;
		uint32		mPageSize;
		enum
		{
			BUCKET_MASK_IMAGE = 0x1,
			BUCKET_MASK_BUFFER = 0x2,
		};
		uint32		mBucketMask;
	};

	struct VulkanAllocationInternal
	{
		enum
		{
			UNUSED,
			ALLOCATED,
			FREED,
			FREEPENDING,
			FREEDISCARDED, // if a defrag happens with a pending free, its put into this state, so we can ignore the deferred delete once it happens.
		};

		void Init(const VulkanAllocation& Alloc, VulkanEvictable* InAllocationOwner, uint32 AllocationOffset, uint32 AllocationSize, uint32 Alignment);

		VulkanAllocationType		mType = VulkanAllocationTypeEmpty;
		VulkanAllocationMetaType	mMetaType = VulkanAllocationMetaUnknown;	
		int							mState = UNUSED;
		uint32						mSize = 0;
		uint32						mAllocationSize = 0;
		uint32						mAllocationOffset = 0;
		VulkanEvictable*			mAllocationOwner = 0;
		uint32						mAlignment = 0;

		int32						mNextFree = -1;
	};

	class ElaineCoreExport VulkanFence : public RHIGPUFence
	{
	public:
		VulkanFence(VulkanDevice* InDevice, VulkanFenceManager* InMgr, bool bCreateSignaled);

		~VulkanFence();

		inline VkFence GetHandle() const
		{
			return mHandle;
		}

		inline bool IsSignaled() const
		{
			return mState == EState::Signaled;
		}

		VulkanFenceManager* GetOwner() const
		{
			return mOwner;
		}

	private:
		enum class EState
		{
			// Initial state
			NotReady,

			// After GPU processed it
			Signaled,
		};

		VkFence				mHandle;
		VulkanFenceManager* mOwner;
		EState				mState;
		friend class VulkanFenceManager;
	};

	class ElaineCoreExport VulkanFenceManager
	{
	public:
		VulkanFenceManager();
		~VulkanFenceManager();
		void Initilize(VulkanDevice* InDevice);
		void DeInit();
		VulkanFence* AllocateFence(bool bCreateSignaled = false);
		inline bool IsFenceSignaled(VulkanFence* Fence)
		{
			if (Fence->IsSignaled())
			{
				return true;
			}

			return CheckFenceState(Fence);
		}

		// Returns false if it timed out
		bool WaitForFence(VulkanFence* Fence, uint64 TimeInNanoseconds);

		void ResetFence(VulkanFence* Fence);

		// Sets it to nullptr
		void ReleaseFence(VulkanFence*& Fence);

		// Sets it to nullptr
		void WaitAndReleaseFence(VulkanFence*& Fence, uint64 TimeInNanoseconds);
	private:
		// Returns true if signaled
		bool CheckFenceState(VulkanFence* Fence);

		void DestroyFence(VulkanFence* Fence);
	private:
		VulkanDevice*				mDevice;
		std::set<VulkanFence*>		mFreeFence;
		std::set<VulkanFence*>		mUsedFence;
		std::recursive_mutex		mMtx;
	};

	class GPUEvent
	{
	public:
		GPUEvent(VulkanDevice* InDevice);
		~GPUEvent();

		inline VkEvent GetHandle() const
		{
			return mHandle;
		}

	protected:
		VkEvent				mHandle;
		VulkanDevice*		mDevice;
	};

	class ElaineCoreExport VulkanSemaphore
	{
	public:
		VulkanSemaphore(VulkanDevice* InDevice);
		VulkanSemaphore(VulkanDevice* InDevice, const VkSemaphore& InExternalSemaphore);
		~VulkanSemaphore();

		inline const VkSemaphore& GetHandle() const
		{
			return mSemaphoreHandle;
		}

		inline bool IsExternallyOwned() const
		{
			return bExternallyOwned;
		}

	private:
		VulkanDevice*	mDevice;
		VkSemaphore		mSemaphoreHandle;
		bool			bExternallyOwned;
	};



	class ElaineCoreExport VulkanDeviceMemoryAllocation
	{
	public:
		VulkanDeviceMemoryAllocation()
			: mSize(0)
			, mDeviceHandle(VK_NULL_HANDLE)
			, mHandle(VK_NULL_HANDLE)
			, mMappedPointer(nullptr)
			, mMemoryTypeIndex(0)
			, mbCanBeMapped(0)
			, mbIsCoherent(0)
			, mbIsCached(0)
			, mbFreedBySystem(false)
			, mbDedicatedMemory(0)
		{

		}
		~VulkanDeviceMemoryAllocation();

		void* Map(VkDeviceSize Size, VkDeviceSize Offset);
		void Unmap();

		void FlushMappedMemory(VkDeviceSize InOffset, VkDeviceSize InSize);
		void InvalidateMappedMemory(VkDeviceSize InOffset, VkDeviceSize InSize);

		inline bool CanBeMapped() const
		{
			return mbCanBeMapped != 0;
		}

		inline bool IsMapped() const
		{
			return !!mMappedPointer;
		}

		inline void* GetMappedPointer()
		{
			assert(IsMapped());
			return mMappedPointer;
		}

		inline bool IsCoherent() const
		{
			return mbIsCoherent != 0;
		}

		inline VkDeviceMemory GetHandle() const
		{
			return mHandle;
		}

		inline VkDeviceSize GetSize() const
		{
			return mSize;
		}

		inline uint32 GetMemoryTypeIndex() const
		{
			return mMemoryTypeIndex;
		}

	private:
		VkDeviceSize	mSize;
		VkDevice		mDeviceHandle;
		VkDeviceMemory	mHandle;
		void*			mMappedPointer;
		uint32			mMemoryTypeIndex : 8;
		uint32			mbCanBeMapped : 1;
		uint32			mbIsCoherent : 1;
		uint32			mbIsCached : 1;
		uint32			mbFreedBySystem : 1;
		uint32			mbDedicatedMemory : 1;
		uint32			: 0;
		friend class VulkanDeviceMemoryManager;
	};

	struct DeviceMemoryBlockKey
	{
		uint32 MemoryTypeIndex;
		VkDeviceSize BlockSize;

		bool operator==(const DeviceMemoryBlockKey& Other) const
		{
			return MemoryTypeIndex == Other.MemoryTypeIndex && BlockSize == Other.BlockSize;
		}

		bool operator< (const DeviceMemoryBlockKey& Other) const
		{
			return Elaine::HashCombine(MemoryTypeIndex, BlockSize) < Elaine::HashCombine(Other.MemoryTypeIndex, Other.BlockSize);
		}
	};

	//bool operator< (const DeviceMemoryBlockKey& Key1, const DeviceMemoryBlockKey& Key2)
	//{
	//	return Elaine::HashCombine(Key1.MemoryTypeIndex, Key1.BlockSize) < Elaine::HashCombine(Key2.MemoryTypeIndex, Key2.BlockSize);
	//}

	struct DeviceMemoryBlock
	{
		DeviceMemoryBlockKey Key;
		struct FreeBlock
		{
			VulkanDeviceMemoryAllocation* Allocation;
			uint32 FrameFreed;
		};
		std::vector<FreeBlock> Allocations;

	};

	class ElaineCoreExport VulkanDeviceMemoryManager
	{
		friend class VulkanMemoryManager;
	public:
		VulkanDeviceMemoryManager();
		~VulkanDeviceMemoryManager();
		void Init(VulkanDevice* InDevice);
		void Deinit();
		uint32 GetEvictedMemoryProperties();
		inline bool HasUnifiedMemory() const
		{
			return mbHasUnifiedMemory;
		}

		inline bool SupportsMemoryless() const
		{
			return mbSupportsMemoryless;
		}

		inline uint32 GetNumMemoryTypes() const
		{
			return mMemoryProperties.memoryTypeCount;
		}

		bool SupportsMemoryType(VkMemoryPropertyFlags Properties) const;
		void GetPrimaryHeapStatus(uint64& OutAllocated, uint64& OutLimit);

		VkResult GetMemoryTypeFromProperties(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32* OutTypeIndex);
		VkResult GetMemoryTypeFromPropertiesExcluding(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32 ExcludeTypeIndex, uint32* OutTypeIndex);
		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const;
		// bCanFail means an allocation failing is not a fatal error, just returns nullptr
		VulkanDeviceMemoryAllocation* Alloc(bool bCanFail, VkDeviceSize AllocationSize, uint32 MemoryTypeIndex, void* DedicatedAllocateInfo, float Priority, bool bExternal, const char* File, uint32 Line);
		VulkanDeviceMemoryAllocation* Alloc(bool bCanFail, VkDeviceSize AllocationSize, uint32 MemoryTypeBits, VkMemoryPropertyFlags MemoryPropertyFlags, void* DedicatedAllocateInfo, float Priority, bool bExternal, const char* File, uint32 Line);

		// Sets the Allocation to nullptr
		void Free(VulkanDeviceMemoryAllocation*& Allocation);

		uint64 GetTotalMemory(bool bGPU) const;
		VkDeviceSize GetBaseHeapSize(uint32 HeapIndex) const;
		uint32 GetHeapIndex(uint32 MemoryTypeIndex);

	private:
		void UpdateMemoryProperties();
		void FreeInternal(VulkanDeviceMemoryAllocation* Allocation);
		void TrimMemory(bool bFullTrim);
	private:
		struct HeapInfo
		{
			HeapInfo() 
				: mUsedSize(0)
				, mPeakSize(0)
			{
			}

			VkDeviceSize mUsedSize;
			VkDeviceSize mPeakSize;
			std::vector<VulkanDeviceMemoryAllocation*> Allocations;
		};

		double										mMemoryUpdateTime;
		VkPhysicalDeviceMemoryBudgetPropertiesEXT	mMemoryBudget;
		VkPhysicalDeviceMemoryProperties			mMemoryProperties;
		VkDevice									mDeviceHandle;
		VulkanDevice*								mDevice;
		uint32										mNumAllocations;
		uint32										mPeakNumAllocations;
		bool										mbHasUnifiedMemory;
		bool										mbSupportsMemoryless;
		int32										mPrimaryHeapIndex;
		std::recursive_mutex						mMtx;
		std::map<DeviceMemoryBlockKey, DeviceMemoryBlock> mAllocations;

		std::vector<HeapInfo>						mHeapInfos;
	};

	class ElaineCoreExport VulkanSubResourceAllocator
	{
		friend class VulkanMemoryManager;
		friend class VulkanResourceHeap;
	public:
		VulkanSubResourceAllocator(VulkanAllocationType InType, VulkanMemoryManager* InOwner, uint8 InSubResourceAllocatorFlags, VulkanDeviceMemoryAllocation* InDeviceMemoryAllocation,
			uint32 InMemoryTypeIndex, VkMemoryPropertyFlags InMemoryPropertyFlags,
			uint32 InAlignment, VkBuffer InBuffer, uint32 InBufferSize, uint32 InBufferId, VkBufferUsageFlags InBufferUsageFlags, int32 InPoolSizeIndex);
		VulkanSubResourceAllocator(VulkanAllocationType InType, VulkanMemoryManager* InOwner, uint8 InSubResourceAllocatorFlags, VulkanDeviceMemoryAllocation* InDeviceMemoryAllocation,
			uint32 InMemoryTypeIndex, uint32 BufferId = 0xffffffff);
		~VulkanSubResourceAllocator();

		void Destroy(VulkanDevice* Device);
		bool TryAllocate2(VulkanAllocation& OutAllocation, VulkanEvictable* Owner, uint32 InSize, uint32 InAlignment, VulkanAllocationMetaType InMetaType, const char* File, uint32 Line);
		void Free(VulkanAllocation& Allocation);

		inline uint32 GetAlignment() const
		{
			return mAlignment;
		}

		inline void* GetMappedPointer()
		{
			return mMemoryAllocation->GetMappedPointer();
		}

		void Flush(VkDeviceSize Offset, VkDeviceSize AllocationSize);
		void Invalidate(VkDeviceSize Offset, VkDeviceSize AllocationSize);
		uint32 GetMaxSize() const { return mMaxSize; }
		uint32 GetUsedSize() const { return mUsedSize; }

		inline uint32 GetHandleId() const
		{
			return mBufferId;
		}
		VulkanDeviceMemoryAllocation* GetMemoryAllocation()
		{
			return mMemoryAllocation;
		}
		VulkanAllocationType GetType() { return mType; }

		std::vector<uint32> GetMemoryUsed();
		uint32 GetNumSubAllocations();

		uint8 GetSubresourceAllocatorFlags()
		{
			return mSubresourceAllocatorFlags;
		}
		uint32 GetId()
		{
			return mBufferId;
		}
		bool GetIsDefragging() { return mbIsDefragging; }

	protected:
		void SetIsDefragging(bool bInIsDefragging) { mbIsDefragging = bInIsDefragging; }
		int32 DefragTick(VulkanDevice& Device, VulkanCommandContext& Context, VulkanResourceHeap* Heap, uint32 Count);
		bool CanDefrag();
		uint64 EvictToHost(VulkanDevice& Device, VulkanCommandContext& Context);
		void SetFreePending(VulkanAllocation& Allocation);
		void FreeInternalData(int32 Index);
		int32 AllocateInternalData();
		bool JoinFreeBlocks();

		uint32									mMemoryUsed[VulkanAllocationMetaSize];

		VulkanAllocationType					mType;
		VulkanMemoryManager* mOwner;
		uint32									mMemoryTypeIndex;
		VkMemoryPropertyFlags					mMemoryPropertyFlags;
		VulkanDeviceMemoryAllocation* mMemoryAllocation;
		uint32									mMaxSize;
		uint32									mAlignment;
		uint32									mFrameFreed;
		uint32									mLastDefragFrame = 0;
		int64									mUsedSize;
		VkBufferUsageFlags						mBufferUsageFlags;
		VkBuffer								mBuffer;
		uint32									mBufferId;
		int32									mPoolSizeIndex;
		uint32									mAllocatorIndex;
		uint8									mSubresourceAllocatorFlags;
		uint8									mBucketId;
		bool									mbIsEvicting = false;
		bool									mbLocked = false;
		bool									mbIsDefragging = false;
		uint32									mNumSubAllocations = 0;
		uint32									mAllocCalls = 0;
		uint32									mFreeCalls = 0;

		// List of free ranges
		std::vector<Range>						mFreeList;
		std::vector<VulkanAllocationInternal>	mInternalData;
		int32									mInternalFreeList = -1;
		std::recursive_mutex					mMtx;
		uint32 GetAllocatorIndex()
		{
			return mAllocatorIndex;
		}
	};

	class VulkanAllocation
	{
	public:
		VulkanAllocation();
		~VulkanAllocation();
		void Init(VulkanAllocationType Type, VulkanAllocationMetaType MetaType, uint64 Handle, uint32 InSize, uint32 AlignedOffset, uint32 AllocatorIndex, uint32 AllocationIndex, uint32 BufferId);
		void Free(VulkanDevice& Device);
		void Swap(VulkanAllocation& Other);
		void Reference(const VulkanAllocation& Other); //point to other, but don't take ownership
		bool HasAllocation();

		void Disown(); //disown & own should be used if ownership is transferred.
		void Own();
		bool IsValid() const;

		uint64						mVulkanHandle = 0;
		uint32						mHandleId = 0;
		uint32						mSize = 0;
		uint32						mOffset = 0;
		uint32						mAllocationIndex = 0;
		uint16						mAllocatorIndex = 0;
		VulkanAllocationMetaType	mMetaType = VulkanAllocationMetaUnknown;
		uint8						mType : 7;
		uint8						mbHasOwnership : 1;

		inline VulkanAllocationType GetType() const
		{
			return (VulkanAllocationType)mType;
		}

		inline void SetType(VulkanAllocationType InType)
		{
			mType = (uint8)InType;
		}

		//helper functions
		void* GetMappedPointer(VulkanDevice* Device);
		void FlushMappedMemory(VulkanDevice* Device);
		void InvalidateMappedMemory(VulkanDevice* Device);
		VkBuffer GetBufferHandle() const;
		uint32 GetBufferAlignment(VulkanDevice* Device) const;
		VkDeviceMemory GetDeviceMemoryHandle(VulkanDevice* Device) const;
		VulkanSubResourceAllocator* GetSubresourceAllocator(VulkanDevice* Device) const;
		void BindBuffer(VulkanDevice* Device, VkBuffer Buffer);
		void BindImage(VulkanDevice* Device, VkImage Image);
	};

	// A set of Device Allocations (Heap Pages) for a specific memory type. This handles pooling allocations inside memory pages to avoid
// doing allocations directly off the device's heaps
	class VulkanResourceHeap
	{
	public:
		VulkanResourceHeap(VulkanMemoryManager* InOwner, uint32 InMemoryTypeIndex, uint32 InOverridePageSize = 0);
		~VulkanResourceHeap();

		void FreePage(VulkanSubResourceAllocator* InPage);
		void ReleasePage(VulkanSubResourceAllocator* InPage);

		//void ReleaseFreedPages(bool bImmediately);

		inline VulkanMemoryManager* GetOwner()
		{
			return mOwner;
		}

		inline bool IsHostCachedSupported() const
		{
			return mbIsHostCachedSupported;
		}

		inline bool IsLazilyAllocatedSupported() const
		{
			return mbIsLazilyAllocatedSupported;
		}

		inline uint32 GetMemoryTypeIndex() const
		{
			return mMemoryTypeIndex;
		}

		uint64 EvictOne(VulkanDevice& Device, VulkanCommandContext& Context);
		void DefragTick(VulkanDevice& Device, VulkanCommandContext& Context, uint32 Count);

		void SetDefragging(VulkanSubResourceAllocator* Allocator);
		bool GetIsDefragging(VulkanSubResourceAllocator* Allocator);
		uint32 GetPageSizeBucket(VulkanPageSizeBucket& BucketOut, VulkanMemoryType Type, uint32 AllocationSize, bool bForceSingleAllocation);

	protected:
		enum
		{
			MAX_BUCKETS = 5,
		};
		std::vector<VulkanPageSizeBucket>	mPageSizeBuckets;
		uint32 GetPageSize();
		VulkanMemoryManager*				mOwner;
		uint16								mMemoryTypeIndex;
		const uint16						mHeapIndex;

		bool								mbIsHostCachedSupported;
		bool								mbIsLazilyAllocatedSupported;
		uint8								mDefragCountDown = 0;

		uint32								mOverridePageSize;

		uint32								mPeakPageSize;
		uint64								mUsedMemory;
		uint32								mPageIDCounter;

		std::recursive_mutex				mMtx;
		std::vector<VulkanSubResourceAllocator*> mActivePages[MAX_BUCKETS];
		std::vector<VulkanSubResourceAllocator*> mUsedDedicatedImagePages;

		bool TryRealloc(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VulkanMemoryType Type, uint32 Size, uint32 Alignment, VulkanAllocationMetaType MetaType);
		bool AllocateResource(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VulkanMemoryType Type, uint32 Size, uint32 Alignment, bool bMapAllocation, bool bForceSeparateAllocation, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line);
		bool AllocateDedicatedImage(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VkImage Image, uint32 Size, uint32 Alignment, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line);

		friend class VulkanMemoryManager;
		friend class VulkanSubResourceAllocator;
	};

	class ElaineCoreExport VulkanStagingBuffer
	{
		friend class VulkanStagingBufferManager;
	public:
		VulkanStagingBuffer(VulkanDevice* InDevice);
		~VulkanStagingBuffer();
		VkBuffer GetHandle() const;
		void* GetMappingPointer();
		uint32 GetSize();
		VkDeviceMemory GetDeviceMemoryHandle() const;
		void FlushMappingMemory();
		void InvalidateMappedMemory();
		void Destroy();
		void UnMap();
		size_t GetBufferOffset() const { return mBufferOffset; }
	protected:
		VulkanDevice*				mDevice;
		VulkanAllocation			mAllocation;
		VkBuffer					mBuffer;
		VkMemoryPropertyFlagBits	mMemoryReadFlags;
		uint32						mBufferSize;
		size_t						mBufferOffset = 0;
	};

	class ElaineCoreExport VulkanStagingBufferManager
	{
		friend class VulkanMemoryManager;
	public:
		VulkanStagingBufferManager();
		~VulkanStagingBufferManager();
		void Init(VulkanDevice* InDevice);
		void DeInit();

		VulkanStagingBuffer* AcquireBuffer(uint32 Size, VkBufferUsageFlags InUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VkMemoryPropertyFlagBits InMemoryReadFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		// Sets pointer to nullptr
		void ReleaseBuffer(VulkanCommandBuffer* CmdBuffer, VulkanStagingBuffer*& StagingBuffer);

		void ProcessPendingFree(bool bImmediately, bool bFreeToOS);
	protected:
		struct PendingItemsPerCmdBuffer
		{
			VulkanCommandBuffer* mCmdBuffer;
			struct PendingItems
			{
				uint64 mFenceCounter;
				std::vector<VulkanStagingBuffer*> mResources;
			};


			inline PendingItems* FindOrAddItemsForFence(uint64 Fence);

			std::vector<PendingItems> mPendingItems;
		};

		struct FreeEntry
		{
			VulkanStagingBuffer*	mStagingBuffer;
			uint32					mFrameNumber;
		};

		std::vector<PendingItemsPerCmdBuffer>	mPendingFreeStagingBuffers;
		std::vector<VulkanStagingBuffer*>		mUsedStagingBuffers;
		std::vector<FreeEntry>					mFreeStagingBuffers;
		uint64									mPeakUsedMemory = 0;
		uint64									mUsedMemory = 0;
		VulkanDevice*							mDevice = VK_NULL_HANDLE;
		std::recursive_mutex					mMtx;

		PendingItemsPerCmdBuffer* FindOrAdd(VulkanCommandBuffer* CmdBuffer);

		void ProcessPendingFreeNoLock(bool bImmediately, bool bFreeToOS);

	};

	class VulkanMemoryManager;




	class ElaineCoreExport VulkanMemoryManager
	{
		friend class VulkanAllocation;
		friend class VulkanResourceHeap;
	public:
		enum class MemBlockSize
		{
			//MT64,
			MT128,
			MT256,
			MT512,
			MT1M,
			MT2M,
			MT8M,
			MTCount,
		};

		constexpr static uint32 PoolSizes[(int32)MemBlockSize::MTCount] = {
			128,
			256,
			512,
			1024,
			2048,
			8192,
		};

		constexpr static uint32 BufferSizes[(int32)MemBlockSize::MTCount + 1] =
		{
			128 * 1024,
			128 * 1024,
			256 * 1024,
			256 * 1024,
			512 * 1024,
			512 * 1024,
			1024 * 1024,
		};

	public:
		VulkanMemoryManager(VulkanDevice* InDeivce);
		~VulkanMemoryManager();
		void Init();
		void Deinit();
		static uint32 CalculateBufferAlignment(VulkanDevice& InDevice, Elaine::BufferUsageFlags InUEUsage, bool bZeroSize);
		static float CalculateBufferPriority(const VkBufferUsageFlags BufferUsageFlags);
		void FreeVulkanAllocation(VulkanAllocation& Allocation, VulkanFreeFlags FreeFlags = VulkanFreeFlag_None);

		void FreeVulkanAllocationPooledBuffer(VulkanAllocation& Allocation);
		void FreeVulkanAllocationBuffer(VulkanAllocation& Allocation);
		void FreeVulkanAllocationImage(VulkanAllocation& Allocation);
		void FreeVulkanAllocationImageDedicated(VulkanAllocation& Allocation);

		bool AllocateBufferPooled(VulkanAllocation& Allocation, VulkanEvictable* AllocationOwner, uint32 Size, uint32 MinAlignment, VkBufferUsageFlags BufferUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, const char* File, uint32 Line);
		bool AllocateImageMemory(VulkanAllocation& Allocation, VulkanEvictable* AllocationOwner, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line);
		bool AllocateBufferMemory(VulkanAllocation& Allocation, VulkanEvictable* AllocationOwner, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line);
		bool AllocateDedicatedImageMemory(VulkanAllocation& Allocation, VulkanEvictable* AllocationOwner, VkImage Image, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line);

		void RegisterSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator);
		void UnregisterSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator);
		bool ReleaseSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator);

		void AllocUniformBuffer(VulkanAllocation& OutAllocation, uint32 Size, const void* Contents);
		void FreeUniformBuffer(VulkanAllocation& InAllocation);

		void ReleaseFreedPages(VulkanCommandContext& Context);
		bool UpdateEvictThreshold(bool bLog);

	private:
		enum
		{
			BufferAllocationSize = 1 * 1024 * 1024,
			UniformBufferAllocationSize = 2 * 1024 * 1024,
		};

		MemBlockSize GetPoolTypeForAlloc(uint32 Size, uint32 Alignment)
		{
			MemBlockSize PoolSize = MemBlockSize::MTCount;
			{
				for (int32 i = 0; i < (int32)MemBlockSize::MTCount; ++i)
				{
					if (PoolSizes[i] >= Size)
					{
						PoolSize = (MemBlockSize)i;
						break;
					}
				}
			}
			return PoolSize;
		}

		inline VulkanSubResourceAllocator* GetSubresourceAllocator(const uint32 AllocatorIndex)
		{
			std::lock_guard<std::recursive_mutex> lock(mMtx);
			return mAllBufferAllocations[AllocatorIndex];
		}

		void ReleaseFreedResources(bool bImmediately);
		void DestroyResourceAllocations();
		void ProcessPendingUBFreesNoLock(bool bForce);
		void ProcessPendingUBFrees(bool bForce);

	private:
		struct FUBPendingFree
		{
			VulkanAllocation	mAllocation;
			uint64				mFrame = 0;
		};

		struct
		{
			std::recursive_mutex			mMtx;
			std::vector<FUBPendingFree>		mPendingFree;
			uint32							mPeak = 0;
		}												mUBAllocations;


		VulkanDeviceMemoryManager*						mDeviceMemoryManager;
		std::vector<VulkanResourceHeap*>				mResourceTypeHeaps;
		std::vector<VulkanSubResourceAllocator*>		mUsedBufferAllocations[(int32)MemBlockSize::MTCount + 1];
		std::vector<VulkanSubResourceAllocator*>		mFreeBufferAllocations[(int32)MemBlockSize::MTCount + 1];
		std::vector<VulkanSubResourceAllocator*>		mAllBufferAllocations;
		uint64											mAllBufferAllocationsFreeListHead = -1;
		uint64											mPendingEvictBytes = 0;
		bool											mbIsEvicting = false;
		bool											mbWantEviction = false;
		VulkanDevice*									mDeivce;
		std::recursive_mutex							mMtx;

	};
}