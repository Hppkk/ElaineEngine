#include "ElainePrecompiledHeader.h"
#include "render/vulkan/ElaineVulkanMemory.h"
#include "render/vulkan/ElaineVulkanDevice.h"
#include "render/vulkan/ElaineVulkanPhysicalDevice.h"
#include "render/vulkan/ElaineVulkanBuffer.h"
#include "render/vulkan/ElaineVulkanTexture.h"
#include "render/vulkan/ElaineVulkanRHI.h"

#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
#include <vulkan_win32.h>
#endif
namespace VulkanRHI
{
	using namespace Elaine;
	int32 GVulkanBudgetPercentageScale = 100;

	namespace
	{
		template<typename BitsType>
		constexpr bool VKHasAllFlags(VkFlags Flags, BitsType Contains)
		{
			return (Flags & Contains) == Contains;
		}

		template<typename BitsType>
		constexpr bool VKHasAnyFlags(VkFlags Flags, BitsType Contains)
		{
			return (Flags & Contains) != 0;
		}

		enum
		{
			GPU_ONLY_HEAP_PAGE_SIZE = 128 * 1024 * 1024,
			STAGING_HEAP_PAGE_SIZE = 32 * 1024 * 1024,
			ANDROID_MAX_HEAP_PAGE_SIZE = 16 * 1024 * 1024,
			ANDROID_MAX_HEAP_IMAGE_PAGE_SIZE = 16 * 1024 * 1024,
			ANDROID_MAX_HEAP_BUFFER_PAGE_SIZE = 4 * 1024 * 1024,
		};
	}

	void Range::JoinConsecutiveRanges(std::vector<Range>& Ranges)
	{
		if (Ranges.size() > 1)
		{

		}
	}

	const TCHAR* VulkanAllocationTypeToString(VulkanAllocationType Type)
	{
		switch (Type)
		{
		case VulkanAllocationTypeEmpty: return TEXT("Empty");
		case VulkanAllocationPooledBuffer: return TEXT("PooledBuffer");
		case VulkanAllocationBuffer: return TEXT("Buffer");
		case VulkanAllocationImage: return TEXT("Image");
		case VulkanAllocationImageDedicated: return TEXT("ImageDedicated");
		default:
			break;
		}
		return TEXT("");
	}

	const TCHAR* VulkanAllocationMetaTypeToString(VulkanAllocationMetaType MetaType)
	{
		switch (MetaType)
		{
		case VulkanAllocationMetaUnknown: return TEXT("Unknown");
		case VulkanAllocationMetaUniformBuffer: return TEXT("UBO");
		case VulkanAllocationMetaMultiBuffer: return TEXT("MultiBuf");
		case VulkanAllocationMetaRingBuffer: return TEXT("RingBuf");
		case VulkanAllocationMetaFrameTempBuffer: return TEXT("FrameTemp");
		case VulkanAllocationMetaImageRenderTarget: return TEXT("ImageRT");
		case VulkanAllocationMetaImageOther: return TEXT("Image");
		case VulkanAllocationMetaBufferUAV: return TEXT("BufferUAV");
		case VulkanAllocationMetaBufferStaging: return TEXT("BufferStg");
		case VulkanAllocationMetaBufferOther: return TEXT("BufOthr");
		default:
			break;
		}
		return TEXT("");
	}

	/** Tries to insert the item so it has index ProposedIndex, but may end up merging it with neighbors */
	int32 Range::InsertAndTryToMerge(std::vector<Range>& Ranges, const Range& Item, int32 ProposedIndex)
	{
		int32 Ret = ProposedIndex;
		if (!!(ProposedIndex == 0))
		{
			// only cases 1 and 3 apply
			Range& NextRange = Ranges[Ret];

			if (!!(NextRange.mOffset == Item.mOffset + Item.mSize))
			{
				NextRange.mOffset = Item.mOffset;
				NextRange.mSize += Item.mSize;
			}
			else
			{
				Ranges.insert(Ranges.begin() + ProposedIndex, Item);
				Ret = ProposedIndex;
			}
		}
		else
		{
			// all cases apply
			Range& NextRange = Ranges[ProposedIndex];
			Range& PrevRange = Ranges[ProposedIndex - 1];

			// see if we can merge with previous
			if (!!(PrevRange.mOffset + PrevRange.mSize == Item.mOffset))
			{
				// case 2, can still end up being case 4
				PrevRange.mSize += Item.mSize;

				if (!!(PrevRange.mOffset + PrevRange.mSize == NextRange.mOffset))
				{
					// case 4
					PrevRange.mSize += NextRange.mSize;
					Ranges.erase(Ranges.begin() + ProposedIndex);
					Ret = ProposedIndex - 1;
				}
			}
			else if (!!(Item.mOffset + Item.mSize == NextRange.mOffset))
			{
				// case 3
				NextRange.mOffset = Item.mOffset;
				NextRange.mSize += Item.mSize;
			}
			else
			{
				// case 1 - the new range is disjoint with both
				Ranges.insert(Ranges.begin() + ProposedIndex, Item);
				Ret = ProposedIndex;	// this can invalidate NextRange/PrevRange references, don't touch them after this
			}
		}
		return Ret;
	}

	/** Tries to append the item to the end but may end up merging it with the neighbor */
	int32 Range::AppendAndTryToMerge(std::vector<Range>& Ranges, const Range& Item)
	{
		int32 Ret = Ranges.size() - 1;

		Range& PrevRange = Ranges[Ret];
		if (!!(PrevRange.mOffset + PrevRange.mSize == Item.mOffset))
		{
			PrevRange.mSize += Item.mSize;
		}
		else
		{
			Ranges.push_back(Item);
			Ret = Ranges.size() - 1;
		}
		return Ret;
	}

	/** Attempts to allocate from an entry - can remove it if it was used up*/
	void Range::AllocateFromEntry(std::vector<Range>& Ranges, int32 Index, uint32 SizeToAllocate)
	{
		Range& Entry = Ranges[Index];
		if (SizeToAllocate < Entry.mSize)
		{
			// Modify current free entry in-place.
			Entry.mSize -= SizeToAllocate;
			Entry.mOffset += SizeToAllocate;
		}
		else
		{
			// Remove this free entry.
			Ranges.erase(Ranges.begin() + Index);
		}
	}

	/** Sanity checks an array of ranges */
	void Range::SanityCheck(std::vector<Range>& Ranges)
	{

	}

	/** Adds to the array while maintaing the sort. */
	int32 Range::Add(std::vector<Range>& Ranges, const Range& Item)
	{
		// find the right place to add
		int32 NumRanges = Ranges.size();
		if (!!(NumRanges <= 0))
		{
			Ranges.push_back(Item);
			return 0u;
		}

		Range* Data = Ranges.data();
		for (int32 Index = 0; Index < NumRanges; ++Index)
		{
			if (!!(Data[Index].mOffset > Item.mOffset))
			{
				return InsertAndTryToMerge(Ranges, Item, Index);
			}
		}

		// if we got this far and still haven't inserted, we're a new element
		return AppendAndTryToMerge(Ranges, Item);
	}

	void VulkanAllocationInternal::Init(const VulkanAllocation& Alloc, VulkanEvictable* InAllocationOwner, uint32 AllocationOffset, uint32 AllocationSize, uint32 Alignment)
	{
		mState = ALLOCATED;
		mType = Alloc.GetType();
		mMetaType = Alloc.mMetaType;

		mSize = Alloc.mSize;
		mAllocationSize = AllocationSize;
		mAllocationOffset = AllocationOffset;
		mAllocationOwner = InAllocationOwner;
		mAlignment = Alignment;
	}
	
	VulkanFence::VulkanFence(VulkanDevice* InDevice, VulkanFenceManager* InMgr, bool bCreateSignaled)
		: mOwner(InMgr)
		, mState(bCreateSignaled ? VulkanFence::EState::Signaled : VulkanFence::EState::NotReady)
	{
		VkFenceCreateInfo info;
		Memory::MemoryZero(info);
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = bCreateSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
		vkCreateFence(InDevice->GetDevice(), &info, VULKAN_CPU_ALLOCATOR, &mHandle);
	}

	VulkanFence::~VulkanFence()
	{

	}

	VulkanFenceManager::VulkanFenceManager()
		: mDevice(nullptr)
	{

	}

	VulkanFenceManager::~VulkanFenceManager()
	{

	}

	void VulkanFenceManager::Initilize(VulkanDevice* InDevice)
	{
		mDevice = InDevice;
	}

	void VulkanFenceManager::DeInit()
	{

		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (!mUsedFence.empty())
		{
			LOG_ERROR("No all fences are done!");
		}

		for (VulkanFence* Fence : mFreeFence)
		{
			DestroyFence(Fence);
		}
	}

	VulkanFence* VulkanFenceManager::AllocateFence(bool bCreateSignaled)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (mFreeFence.size() != 0)
		{
			VulkanFence* Fence = *mFreeFence.begin();
			mFreeFence.erase(Fence);
			mUsedFence.emplace(Fence);
			if (bCreateSignaled)
			{
				Fence->mState = VulkanFence::EState::Signaled;
			}
			return Fence;
		}

		VulkanFence* newFence = new VulkanFence(mDevice, this, bCreateSignaled);
		mUsedFence.emplace(newFence);
		return newFence;
	}
	// Returns false if it timed out
	bool VulkanFenceManager::WaitForFence(VulkanFence* Fence, uint64 TimeInNanoseconds)
	{
		auto iter = mUsedFence.find(Fence);
		if (iter == mUsedFence.end())
			return false;

		VkResult Result = vkWaitForFences(mDevice->GetDevice() ,1 , &Fence->mHandle, true, TimeInNanoseconds);
		switch (Result)
		{
		case VK_SUCCESS:
			Fence->mState = VulkanFence::EState::Signaled;
			return true;
		case VK_TIMEOUT:
			break;
		default:
			break;
		}

		return false;
	}

	void VulkanFenceManager::ResetFence(VulkanFence* Fence)
	{
		if (Fence->mState != VulkanFence::EState::NotReady)
		{
			vkResetFences(mDevice->GetDevice(), 1, &Fence->mHandle);
			Fence->mState = VulkanFence::EState::NotReady;
		}
	}

	// Sets it to nullptr
	void VulkanFenceManager::ReleaseFence(VulkanFence*& Fence)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		ResetFence(Fence);
		mUsedFence.erase(Fence);
		mFreeFence.emplace(Fence);
		Fence = nullptr;
	}

	// Sets it to nullptr
	void VulkanFenceManager::WaitAndReleaseFence(VulkanFence*& Fence, uint64 TimeInNanoseconds)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (!Fence->IsSignaled())
		{
			WaitForFence(Fence, TimeInNanoseconds);
		}

		ResetFence(Fence);
		mUsedFence.erase(Fence);
		mFreeFence.emplace(Fence);
		Fence = nullptr;
	}
	// Returns true if signaled
	bool VulkanFenceManager::CheckFenceState(VulkanFence* Fence)
	{
		auto iter = mUsedFence.find(Fence);
		if (iter == mUsedFence.end())
			return false;

		VkResult Result = vkGetFenceStatus(mDevice->GetDevice(), Fence->mHandle);
		switch (Result)
		{
		case VK_SUCCESS:
			Fence->mState = VulkanFence::EState::Signaled;
			return true;

		case VK_NOT_READY:
			break;

		default:
			break;
		}

		return false;
	}

	void VulkanFenceManager::DestroyFence(VulkanFence* Fence)
	{
		vkDestroyFence(mDevice->GetDevice(), Fence->GetHandle(), VULKAN_CPU_ALLOCATOR);
		Fence->mHandle = nullptr;
		delete Fence;
	}

	GPUEvent::GPUEvent(VulkanDevice* InDevice)
		:mDevice(InDevice)
	{
		VkEventCreateInfo info;
		Memory::MemoryZero(info);
		info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
		vkCreateEvent(InDevice->GetDevice(), &info, VULKAN_CPU_ALLOCATOR, &mHandle);
	}
	GPUEvent::~GPUEvent()
	{
		mDevice->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::EType::Event, mHandle);
	}

	VulkanSemaphore::VulkanSemaphore(VulkanDevice* InDevice)
		: mDevice(InDevice)
		, bExternallyOwned(false)
	{
		VkSemaphoreCreateInfo CreateInfo;
		Memory::MemoryZero(CreateInfo);
		CreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		vkCreateSemaphore(mDevice->GetDevice(), &CreateInfo, VULKAN_CPU_ALLOCATOR, &mSemaphoreHandle);
	}

	VulkanSemaphore::VulkanSemaphore(VulkanDevice* InDevice, const VkSemaphore& InExternalSemaphore)
		: mDevice(InDevice)
		, mSemaphoreHandle(InExternalSemaphore)
		, bExternallyOwned(true)
	{

	}

	VulkanSemaphore::~VulkanSemaphore()
	{
		if (!bExternallyOwned)
		{
			mDevice->GetDeferredDeletionQueue().EnqueueResource(DeferredDeletionQueue::EType::Semaphore, mSemaphoreHandle);
		}
		mSemaphoreHandle = VK_NULL_HANDLE;
	}

	VulkanStagingBuffer::VulkanStagingBuffer(VulkanDevice* InDevice)
		: mDevice(InDevice)
		, mBuffer(VK_NULL_HANDLE)
		, mMemoryReadFlags(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		, mBufferSize(0)
	{

	}

	VulkanStagingBuffer::~VulkanStagingBuffer()
	{
		mDevice->GetMemoryManager()->FreeVulkanAllocation(mAllocation);
	}

	VkBuffer VulkanStagingBuffer::GetHandle() const
	{
		return mBuffer;
	}

	void* VulkanStagingBuffer::GetMappingPointer()
	{
		return mAllocation.GetMappedPointer(mDevice);
	}

	uint32 VulkanStagingBuffer::GetSize()
	{
		return mBufferSize;
	}

	VkDeviceMemory VulkanStagingBuffer::GetDeviceMemoryHandle() const
	{
		return mAllocation.GetDeviceMemoryHandle(mDevice);
	}

	void VulkanStagingBuffer::FlushMappingMemory()
	{
		mAllocation.FlushMappedMemory(mDevice);
	}

	void VulkanStagingBuffer::InvalidateMappedMemory()
	{
		mAllocation.InvalidateMappedMemory(mDevice);
	}

	void VulkanStagingBuffer::Destroy()
	{
		vkDestroyBuffer(mDevice->GetDevice(), mBuffer, VULKAN_CPU_ALLOCATOR);
		mBuffer = VK_NULL_HANDLE;
		mDevice->GetMemoryManager()->FreeVulkanAllocation(mAllocation);
	}

	void VulkanStagingBuffer::UnMap()
	{
		mAllocation.GetSubresourceAllocator(mDevice)->GetMemoryAllocation()->Unmap();
	}

	VulkanStagingBufferManager::VulkanStagingBufferManager()
	{

	}

	VulkanStagingBufferManager::~VulkanStagingBufferManager()
	{

	}

	void VulkanStagingBufferManager::Init(VulkanDevice* InDevice)
	{
		mDevice = InDevice;
	}

	void VulkanStagingBufferManager::DeInit()
	{
		ProcessPendingFree(true, true);
	}

	VulkanStagingBuffer* VulkanStagingBufferManager::AcquireBuffer(uint32 Size, VkBufferUsageFlags InUsageFlags, VkMemoryPropertyFlagBits InMemoryReadFlags)
	{
		if (InMemoryReadFlags == VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		{
			uint64 NonCoherentAtomSize = (uint64)mDevice->GetPhyDevice()->GetLimits().nonCoherentAtomSize;
			Size = AlignArbitrary(Size, NonCoherentAtomSize);
		}

		if ((InUsageFlags & (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)) != 0)
		{
			InUsageFlags |= (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		}
		{
			std::lock_guard<std::recursive_mutex> Lock(mMtx);
			for (int32 Index = 0; Index < mFreeStagingBuffers.size(); ++Index)
			{
				FreeEntry& FreeBuffer = mFreeStagingBuffers[Index];
				if (FreeBuffer.mStagingBuffer->GetSize() == Size && FreeBuffer.mStagingBuffer->mMemoryReadFlags == InMemoryReadFlags)
				{
					VulkanStagingBuffer* Buffer = FreeBuffer.mStagingBuffer;
					mFreeStagingBuffers.erase(mFreeStagingBuffers.begin() + Index);
					mUsedStagingBuffers.push_back(Buffer);
					return Buffer;
				}
			}
		}

		VulkanStagingBuffer* StagingBuffer = new VulkanStagingBuffer(mDevice);

		VkBufferCreateInfo StagingBufferCreateInfo;
		Memory::MemoryZero(StagingBufferCreateInfo);
		StagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		StagingBufferCreateInfo.size = Size;
		StagingBufferCreateInfo.usage = InUsageFlags;

		VkDevice VulkanDevice = mDevice->GetDevice();

		vkCreateBuffer(VulkanDevice, &StagingBufferCreateInfo, VULKAN_CPU_ALLOCATOR, &StagingBuffer->mBuffer);

		VkMemoryRequirements MemReqs;
		vkGetBufferMemoryRequirements(VulkanDevice, StagingBuffer->mBuffer, &MemReqs);

		// Set minimum alignment to 16 bytes, as some buffers are used with CPU SIMD instructions
		MemReqs.alignment = Math::max<VkDeviceSize>(16, MemReqs.alignment);
		static const bool bIsAmd = mDevice->GetPhyDevice()->GetDeviceProperties().vendorID == 0x1002;
		if (InMemoryReadFlags == VK_MEMORY_PROPERTY_HOST_CACHED_BIT || bIsAmd)
		{
			uint64 NonCoherentAtomSize = (uint64)mDevice->GetPhyDevice()->GetLimits().nonCoherentAtomSize;
			MemReqs.alignment = AlignArbitrary(MemReqs.alignment, NonCoherentAtomSize);
		}

		VkMemoryPropertyFlags readTypeFlags = InMemoryReadFlags;
		if (!mDevice->GetMemoryManager()->AllocateBufferMemory(StagingBuffer->mAllocation, nullptr, MemReqs, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | readTypeFlags, VulkanAllocationMetaBufferStaging, false, __FILE__, __LINE__))
		{
			
		}
		StagingBuffer->mMemoryReadFlags = InMemoryReadFlags;
		StagingBuffer->mBufferSize = Size;
		StagingBuffer->mAllocation.BindBuffer(mDevice, StagingBuffer->mBuffer);
		StagingBuffer->mBufferOffset = StagingBuffer->mAllocation.mOffset;
		//StagingBuffer->ResourceAllocation->BindBuffer(Device, StagingBuffer->Buffer);

		{
			std::lock_guard<std::recursive_mutex> Lock(mMtx);
			mUsedStagingBuffers.push_back(StagingBuffer);
			mUsedMemory += StagingBuffer->GetSize();
			mPeakUsedMemory = Math::max(mUsedMemory, mPeakUsedMemory);
		}
		return StagingBuffer;

	}

	// Sets pointer to nullptr
	void VulkanStagingBufferManager::ReleaseBuffer(VulkanCommandBuffer* CmdBuffer, VulkanStagingBuffer*& StagingBuffer)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		auto iter = std::find(mUsedStagingBuffers.begin(), mUsedStagingBuffers.end(), StagingBuffer);
		if (iter != mUsedStagingBuffers.end())
			mUsedStagingBuffers.erase(iter);

		if (CmdBuffer)
		{
			//PendingItemsPerCmdBuffer* ItemsForCmdBuffer = FindOrAdd(CmdBuffer);
			//PendingItemsPerCmdBuffer::PendingItems* ItemsForFence = ItemsForCmdBuffer->FindOrAddItemsForFence(CmdBuffer->GetFenceSignaledCounterA());
			//ItemsForFence->mResources.push_back(StagingBuffer);
		}
		else
		{
			mFreeStagingBuffers.push_back({ StagingBuffer, 1 });
		}
		StagingBuffer = nullptr;
	}

	inline VulkanStagingBufferManager::PendingItemsPerCmdBuffer::PendingItems* VulkanStagingBufferManager::PendingItemsPerCmdBuffer::FindOrAddItemsForFence(uint64 Fence)
	{
		for (int32 Index = 0; Index < mPendingItems.size(); ++Index)
		{
			if (mPendingItems[Index].mFenceCounter == Fence)
			{ 
				return &mPendingItems[Index];
			}
		}
		mPendingItems.push_back({});
		PendingItems* New = new(&mPendingItems.back()) PendingItems;
		New->mFenceCounter = Fence;
		return New;
	}

	void VulkanStagingBufferManager::ProcessPendingFree(bool bImmediately, bool bFreeToOS)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		ProcessPendingFreeNoLock(bImmediately, bFreeToOS);
	}

	VulkanStagingBufferManager::PendingItemsPerCmdBuffer* VulkanStagingBufferManager::FindOrAdd(VulkanCommandBuffer* CmdBuffer)
	{
		return nullptr;
	}

	void VulkanStagingBufferManager::ProcessPendingFreeNoLock(bool bImmediately, bool bFreeToOS)
	{

	}

	VulkanResourceHeap::VulkanResourceHeap(VulkanMemoryManager* InOwner, uint32 InMemoryTypeIndex, uint32 InOverridePageSize)
		: mOwner(InOwner)
		, mMemoryTypeIndex((uint16)InMemoryTypeIndex)
		, mHeapIndex((uint16)InOwner->mDeivce->GetDeviceMemoryManager()->GetHeapIndex(InMemoryTypeIndex))
		, mbIsHostCachedSupported(false)
		, mbIsLazilyAllocatedSupported(false)
		, mOverridePageSize(InOverridePageSize)
		, mPeakPageSize(0)
		, mUsedMemory(0)
		, mPageIDCounter(0)
	{

	}

	VulkanResourceHeap::~VulkanResourceHeap()
	{
		auto DeletePages = [&](std::vector<VulkanSubResourceAllocator*>& UsedPages, const TCHAR* Name)
		{
			bool bLeak = false;
			for (uint32 Index = UsedPages.size() - 1; Index >= 0; --Index)
			{
				VulkanSubResourceAllocator* Page = UsedPages[Index];
				bLeak |= !Page->JoinFreeBlocks();
				mOwner->mDeivce->GetDeviceMemoryManager()->Free(Page->mMemoryAllocation);
				delete Page;
			}
			UsedPages.clear();
			return bLeak;
		};

		for (std::vector<VulkanSubResourceAllocator*>& Pages : mActivePages)
		{
			DeletePages(Pages, TEXT("Pages"));
		}
	}

	void VulkanResourceHeap::FreePage(VulkanSubResourceAllocator* InPage)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		assert(InPage->JoinFreeBlocks());

		InPage->mFrameFreed = 1;

		if (InPage->GetType() == VulkanAllocationImageDedicated)
		{
			auto iter = std::find(mUsedDedicatedImagePages.begin(), mUsedDedicatedImagePages.end(), InPage);
			if (iter!= mUsedDedicatedImagePages.end())
			{
				mUsedDedicatedImagePages.erase(iter);
			}
		}
		else
		{
			uint8 BucketId = InPage->mBucketId;

			std::vector<VulkanSubResourceAllocator*>& Pages = mActivePages[BucketId];
			auto iter = std::find(Pages.begin(), Pages.end(), InPage);
			if (iter != Pages.end())
			{
				Pages.erase(iter);
			}
		}

		ReleasePage(InPage);
	}

	void VulkanResourceHeap::ReleasePage(VulkanSubResourceAllocator* InPage)
	{
		mOwner->UnregisterSubresourceAllocator(InPage);
		VulkanDeviceMemoryAllocation* Allocation = InPage->mMemoryAllocation;
		InPage->mMemoryAllocation = 0;
		mOwner->mDeivce->GetDeviceMemoryManager()->Free(Allocation);
		mUsedMemory -= InPage->mMaxSize;
		delete InPage;
	}

	uint64 VulkanResourceHeap::EvictOne(VulkanDevice& Device, VulkanCommandContext& Context)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		for (int32 Index = MAX_BUCKETS - 2; Index >= 0; Index--)
		{
			std::vector<VulkanSubResourceAllocator*>& Pages = mActivePages[Index];
			for (int32 Index2 = 0; Index2 < Pages.size(); ++Index2)
			{
				VulkanSubResourceAllocator* Allocator = Pages[Index2];
				if (!Allocator->mbIsEvicting && Allocator->GetSubresourceAllocatorFlags() & VulkanAllocationFlagsCanEvict)
				{
					return Allocator->EvictToHost(Device, Context);
				}
			}
		}
		return 0;
	}

	float GVulkanDefragSizeFactor = 1.3f;
	float GVulkanDefragSizeFraction = .7f;
	int32 GVulkanDefragAgeDelay = 100;

	void VulkanResourceHeap::DefragTick(VulkanDevice& Device, VulkanCommandContext& Context, uint32 Count)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		VulkanSubResourceAllocator* CurrentDefragTarget = 0;
		//Continue if defrag currently in progress.
		for (std::vector<VulkanSubResourceAllocator*>& Pages : mActivePages)
		{
			for (int32 Index = 0; Index < Pages.size(); ++Index)
			{
				VulkanSubResourceAllocator* Allocator = Pages[Index];
				if (GetIsDefragging(Allocator))
				{
					CurrentDefragTarget = Allocator;
				}
			}
		}


		if (!CurrentDefragTarget)
		{
			//    If no defragger is currently in progress, search for a new candidate.
			//    search for a candiate that
			// 1) is defraggable
			// 2) has not recently been defragged
			struct PotentialDefragMove
			{
				uint64 Key;
				VulkanSubResourceAllocator* Allocator;
			};
			const uint32 FrameNumber = 1;
			std::vector<PotentialDefragMove> PotentialDefrag;
			for (std::vector<VulkanSubResourceAllocator*>& Pages : mActivePages)
			{
				uint32 FreeSpace = 0;
				for (int32 Index = 0; Index < Pages.size(); ++Index)
				{
					VulkanSubResourceAllocator* Allocator = Pages[Index];
					if (0 == (Allocator->GetSubresourceAllocatorFlags() & VulkanAllocationFlagsCanEvict))
					{
						uint32 MaxSize = Allocator->GetMaxSize();
						uint32 UsedSize = Allocator->GetUsedSize();
						FreeSpace += MaxSize - UsedSize;
					}
				}

				for (int32 Index = 0; Index < Pages.size(); ++Index)
				{
					VulkanSubResourceAllocator* Allocator = Pages[Index];
					if (0 == (Allocator->GetSubresourceAllocatorFlags() & VulkanAllocationFlagsCanEvict) && Allocator->CanDefrag())
					{
						uint32 MaxSize = Allocator->GetMaxSize();
						uint32 UsedSize = Allocator->GetUsedSize();
						uint32 Age = FrameNumber - Allocator->mLastDefragFrame;
						if (FreeSpace > UsedSize
							&& UsedSize * GVulkanDefragSizeFactor < FreeSpace - UsedSize
							&& UsedSize < MaxSize * GVulkanDefragSizeFraction
							&& Age >(uint32)GVulkanDefragAgeDelay)
						{
							float FillPrc = 1.f - (float(UsedSize) / MaxSize);
							uint32 SortLow = uint16(FillPrc * 0xffff);
							uint64 SortKey = (uint64(Age) << 32) | SortLow;
							PotentialDefragMove Potential = { SortKey, Allocator };
							PotentialDefrag.push_back(Potential);
						}
					}
				}
			}
			if (PotentialDefrag.size())
			{
				std::sort(PotentialDefrag.begin(),PotentialDefrag.end(),([](const PotentialDefragMove& LHS, const PotentialDefragMove& RHS)
					{
						return LHS.Key < RHS.Key;
					}));
				CurrentDefragTarget = PotentialDefrag[0].Allocator;
				SetDefragging(CurrentDefragTarget);
				mDefragCountDown = 3;
			}
		}


		if (CurrentDefragTarget)
		{
			uint32 MaxSize = CurrentDefragTarget->GetMaxSize();
			uint32 UsedSize = CurrentDefragTarget->GetUsedSize();
			uint32 FreeSpace = MaxSize - UsedSize;

			if (CurrentDefragTarget->DefragTick(Device, Context, this, Count) > 0)
			{
				mDefragCountDown = 3;
			}
			else
			{
				if (0 == --mDefragCountDown)
				{
					CurrentDefragTarget->SetIsDefragging(false);
				}
			}
		}
	}

	void VulkanResourceHeap::SetDefragging(VulkanSubResourceAllocator* Allocator)
	{
		uint32 Flags = Allocator->GetSubresourceAllocatorFlags();
		int32 CurrentIndex = -1;
		int32 LastIndex = -1;
		uint32 BucketId = Allocator->mBucketId;
		std::vector<VulkanSubResourceAllocator*>& Pages = mActivePages[BucketId];
		for (int32 Index = 0; Index < Pages.size(); ++Index)
		{
			VulkanSubResourceAllocator* Used = Pages[Index];
			if (Flags == (Allocator->GetSubresourceAllocatorFlags()))
			{
				if (Used == Allocator)
				{
					CurrentIndex = Index;
					Used->SetIsDefragging(true);
				}
				else
				{
					LastIndex = Index;
					Used->SetIsDefragging(false);
				}
			}
		}

		//make sure heap being defragged is last, so we don't allocate from it unless its the only option
		if (LastIndex > CurrentIndex)
		{
			Pages[CurrentIndex] = Pages[LastIndex];
			Pages[LastIndex] = Allocator;
		}
	}

	bool VulkanResourceHeap::GetIsDefragging(VulkanSubResourceAllocator* Allocator)
	{
		return Allocator->GetIsDefragging();
	}

	uint32 VulkanResourceHeap::GetPageSizeBucket(VulkanPageSizeBucket& BucketOut, VulkanMemoryType Type, uint32 AllocationSize, bool bForceSingleAllocation)
	{
		if (bForceSingleAllocation)
		{
			uint32 Bucket = mPageSizeBuckets.size() - 1;
			BucketOut = mPageSizeBuckets[Bucket];
			return Bucket;
		}
		uint32 Mask = 0;
		Mask |= (Type == VulkanMemoryType::Image) ? VulkanPageSizeBucket::BUCKET_MASK_IMAGE : 0;
		Mask |= (Type == VulkanMemoryType::Buffer) ? VulkanPageSizeBucket::BUCKET_MASK_BUFFER : 0;
		for (VulkanPageSizeBucket& B : mPageSizeBuckets)
		{
			if (Mask == (B.mBucketMask & Mask) && AllocationSize <= B.mAllocationMax)
			{
				BucketOut = B;
				return &B - &mPageSizeBuckets[0];
			}
		}
		return 0xffffffff;
	}

	uint32 VulkanResourceHeap::GetPageSize()
	{
		return 0;
	}

	bool MetaTypeCanEvict(VulkanAllocationMetaType MetaType)
	{
		switch (MetaType)
		{
		case VulkanAllocationMetaImageOther: return true;
		default: return false;
		}
	}

	bool VulkanResourceHeap::TryRealloc(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VulkanMemoryType Type, uint32 Size, uint32 Alignment, VulkanAllocationMetaType MetaType)
	{
		VulkanDeviceMemoryManager* DeviceMemoryManager = mOwner->mDeivce->GetDeviceMemoryManager();
		VulkanPageSizeBucket MemoryBucket;
		uint32 BucketId = GetPageSizeBucket(MemoryBucket, Type, Size, false);

		bool bHasUnifiedMemory = DeviceMemoryManager->HasUnifiedMemory();

		std::vector<VulkanSubResourceAllocator*>& UsedPages = mActivePages[BucketId];
		VulkanAllocationType AllocationType = (Type == VulkanMemoryType::Image) ? VulkanAllocationImage : VulkanAllocationBuffer;
		uint8 AllocationFlags = (!bHasUnifiedMemory && MetaTypeCanEvict(MetaType)) ? VulkanAllocationFlagsCanEvict : 0;

		{
			// Check Used pages to see if we can fit this in
			for (int32 Index = 0; Index < UsedPages.size(); ++Index)
			{
				VulkanSubResourceAllocator* Page = UsedPages[Index];
				if (!Page->mbLocked)
				{
					if (Page->GetSubresourceAllocatorFlags() == AllocationFlags)
					{
						if (Page->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, __FILE__, __LINE__))
						{
							return true;
						}
					}
				}
			}
		}
		return false;
	}

	bool VulkanResourceHeap::AllocateResource(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VulkanMemoryType Type, uint32 Size, uint32 Alignment, bool bMapAllocation, bool bForceSeparateAllocation, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		bForceSeparateAllocation = bForceSeparateAllocation;
		VulkanDeviceMemoryManager* DeviceMemoryManager = mOwner->mDeivce->GetDeviceMemoryManager();
		VulkanPageSizeBucket MemoryBucket;
		uint32 BucketId = GetPageSizeBucket(MemoryBucket, Type, Size, bForceSeparateAllocation);
		bool bHasUnifiedMemory = DeviceMemoryManager->HasUnifiedMemory();
		std::vector<VulkanSubResourceAllocator*>& UsedPages = mActivePages[BucketId];
		VulkanAllocationType AllocationType = (Type == VulkanMemoryType::Image) ? VulkanAllocationImage : VulkanAllocationBuffer;
		uint8 AllocationFlags = (!bHasUnifiedMemory && MetaTypeCanEvict(MetaType)) ? VulkanAllocationFlagsCanEvict : 0;
		if (bMapAllocation)
		{
			AllocationFlags |= VulkanAllocationFlagsMapped;
		}
		uint32 AllocationSize;

		if (!bForceSeparateAllocation)
		{
			if (Size < MemoryBucket.mPageSize) // Last bucket, for dedicated allocations has max size set to 0, preventing reuse
			{
				// Check Used pages to see if we can fit this in
				for (int32 Index = 0; Index < UsedPages.size(); ++Index)
				{
					VulkanSubResourceAllocator* Page = UsedPages[Index];
					if (Page->GetSubresourceAllocatorFlags() == AllocationFlags)
					{
						assert(Page->mMemoryAllocation->IsMapped() == bMapAllocation);
						if (Page->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line))
						{
							return true;
						}
					}
				}
			}
			AllocationSize = Math::max(Size, MemoryBucket.mPageSize); // for allocations above max, which are forced to be seperate allocations
		}
		else
		{
			// We get here when bForceSeparateAllocation is true, which is used for lazy allocations, since pooling those doesn't make sense.
			AllocationSize = Size;
		}

		VulkanDeviceMemoryAllocation* DeviceMemoryAllocation = DeviceMemoryManager->Alloc(true, AllocationSize, mMemoryTypeIndex, nullptr, 1.f, bExternal, File, Line);
		if (!DeviceMemoryAllocation && Size != AllocationSize)
		{
			// Retry with a smaller size
			DeviceMemoryAllocation = DeviceMemoryManager->Alloc(true, Size, mMemoryTypeIndex, nullptr, 1.f, bExternal, File, Line);
			if (!DeviceMemoryAllocation)
			{
				return false;
			}
		}

		if (!DeviceMemoryAllocation)
		{
			LOG_FATAL("LogVulkanRHI: Out of memory on Vulkan");
		}
		if (bMapAllocation)
		{
			DeviceMemoryAllocation->Map(AllocationSize, 0);
		}

		uint32 BufferId = 0;
		
		++mPageIDCounter;
		VulkanSubResourceAllocator* Page = new VulkanSubResourceAllocator(AllocationType, mOwner, AllocationFlags, DeviceMemoryAllocation, mMemoryTypeIndex, BufferId);
		mOwner->RegisterSubresourceAllocator(Page);
		Page->mBucketId = BucketId;
		mActivePages[BucketId].push_back(Page);

		mUsedMemory += AllocationSize;

		mPeakPageSize = Math::max(mPeakPageSize, AllocationSize);

		bool bOk = Page->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line);

		return bOk;

	}

	bool VulkanResourceHeap::AllocateDedicatedImage(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VkImage Image, uint32 Size, uint32 Alignment, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		uint32 AllocationSize = Size;
		VkMemoryDedicatedAllocateInfoKHR DedicatedAllocInfo;
		Memory::MemoryZero(DedicatedAllocInfo);
		DedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
		DedicatedAllocInfo.image = Image;
		VulkanDeviceMemoryAllocation* DeviceMemoryAllocation = mOwner->mDeivce->GetDeviceMemoryManager()->Alloc(true, AllocationSize, mMemoryTypeIndex, &DedicatedAllocInfo, 1.f, bExternal, File, Line);
		if (!DeviceMemoryAllocation)
		{
			return false;
		}

		uint32 BufferId = 0;
		++mPageIDCounter;

		VulkanSubResourceAllocator* NewPage = new VulkanSubResourceAllocator(VulkanAllocationImageDedicated, mOwner, 0, DeviceMemoryAllocation, mMemoryTypeIndex, BufferId);
		mOwner->RegisterSubresourceAllocator(NewPage);
		NewPage->mBucketId = 0xff;
		mUsedDedicatedImagePages.push_back(NewPage);

		mUsedMemory += AllocationSize;

		mPeakPageSize = Math::max(mPeakPageSize, AllocationSize);
		return NewPage->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line);
	}

	VulkanAllocation::VulkanAllocation()
		: mbHasOwnership(0)
	{
		SetType(VulkanAllocationTypeEmpty);
	}

	VulkanAllocation::~VulkanAllocation()
	{

	}

	void VulkanAllocation::Init(VulkanAllocationType Type, VulkanAllocationMetaType MetaType, uint64 Handle, uint32 InSize, uint32 AlignedOffset, uint32 AllocatorIndex, uint32 AllocationIndex, uint32 BufferId)
	{
		mbHasOwnership = 1;
		SetType(Type);
		mMetaType = MetaType;
		mSize = InSize;
		mOffset = AlignedOffset;
		mAllocatorIndex = AllocatorIndex;
		mAllocationIndex = AllocationIndex;
		mVulkanHandle = Handle;
		mHandleId = BufferId;
	}

	void VulkanAllocation::Free(VulkanDevice& Device)
	{
		if (HasAllocation())
		{
			Device.GetMemoryManager()->FreeVulkanAllocation(*this);
		}
	}

	void VulkanAllocation::Swap(VulkanAllocation& Other)
	{
		Memory::MemorySwap(this, &Other, sizeof(*this));
	}

	void VulkanAllocation::Reference(const VulkanAllocation& Other)
	{
		Memory::MemoryCopy(*this, Other);
		mbHasOwnership = 0;
	}

	bool VulkanAllocation::HasAllocation()
	{
		return mType != VulkanAllocationTypeEmpty && mbHasOwnership;
	}

	void VulkanAllocation::Disown()
	{
		mbHasOwnership = 0;
	}

	void VulkanAllocation::Own()
	{
		mbHasOwnership = 1;
	}

	bool VulkanAllocation::IsValid() const
	{
		return mSize != 0;
	}

	void* VulkanAllocation::GetMappedPointer(VulkanDevice* Device)
	{
		VulkanSubResourceAllocator* Allocator = GetSubresourceAllocator(Device);
		uint8* pMappedPointer = (uint8*)Allocator->GetMappedPointer();
		return mOffset + pMappedPointer;
	}

	void VulkanAllocation::FlushMappedMemory(VulkanDevice* Device)
	{
		VulkanSubResourceAllocator* Allocator = GetSubresourceAllocator(Device);
		Allocator->Flush(mOffset, mSize);
	}

	void VulkanAllocation::InvalidateMappedMemory(VulkanDevice* Device)
	{
		VulkanSubResourceAllocator* Allocator = GetSubresourceAllocator(Device);
		Allocator->Invalidate(mOffset, mSize);
	}

	VkBuffer VulkanAllocation::GetBufferHandle() const
	{
		return (VkBuffer)mVulkanHandle;
	}

	uint32 VulkanAllocation::GetBufferAlignment(VulkanDevice* Device) const
	{
		VulkanSubResourceAllocator* Allocator = GetSubresourceAllocator(Device);
		return Allocator->GetAlignment();
	}

	VkDeviceMemory VulkanAllocation::GetDeviceMemoryHandle(VulkanDevice* Device) const
	{
		VulkanSubResourceAllocator* Allocator = GetSubresourceAllocator(Device);
		return Allocator->GetMemoryAllocation()->GetHandle();
	}

	VulkanSubResourceAllocator* VulkanAllocation::GetSubresourceAllocator(VulkanDevice* Device) const
	{
		switch (mType)
		{
		case VulkanAllocationTypeEmpty:
			return 0;
			break;
		case VulkanAllocationPooledBuffer:
		case VulkanAllocationBuffer:
		case VulkanAllocationImage:
		case VulkanAllocationImageDedicated:
			return Device->GetMemoryManager()->GetSubresourceAllocator(mAllocatorIndex);
			break;
		default:
			break;
		}
		return 0;
	}

	void VulkanAllocation::BindBuffer(VulkanDevice* Device, VkBuffer Buffer)
	{
		VkDeviceMemory MemoryHandle = GetDeviceMemoryHandle(Device);
		VkResult Result = vkBindBufferMemory(Device->GetDevice(), Buffer, MemoryHandle, mOffset);

	}

	void VulkanAllocation::BindImage(VulkanDevice* Device, VkImage Image)
	{
		VkDeviceMemory MemoryHandle = GetDeviceMemoryHandle(Device);
		VkResult Result = vkBindImageMemory(Device->GetDevice(), Image, MemoryHandle, mOffset);

	}

	VulkanSubResourceAllocator::VulkanSubResourceAllocator(VulkanAllocationType InType, VulkanMemoryManager* InOwner, uint8 InSubResourceAllocatorFlags, VulkanDeviceMemoryAllocation* InDeviceMemoryAllocation,
		uint32 InMemoryTypeIndex, VkMemoryPropertyFlags InMemoryPropertyFlags,
		uint32 InAlignment, VkBuffer InBuffer, uint32 InBufferSize, uint32 InBufferId, VkBufferUsageFlags InBufferUsageFlags, int32 InPoolSizeIndex)
		: mType(InType)
		, mOwner(InOwner)
		, mMemoryTypeIndex(InMemoryTypeIndex)
		, mMemoryPropertyFlags(InMemoryPropertyFlags)
		, mMemoryAllocation(InDeviceMemoryAllocation)
		, mMaxSize(InBufferSize)
		, mAlignment(InAlignment)
		, mFrameFreed(0)
		, mUsedSize(0)
		, mBufferUsageFlags(InBufferUsageFlags)
		, mBuffer(InBuffer)
		, mBufferId(InBufferId)
		, mPoolSizeIndex(InPoolSizeIndex)
		, mAllocatorIndex(0xffffffff)
		, mSubresourceAllocatorFlags(InSubResourceAllocatorFlags)
	{
		Memory::MemoryZero(mMemoryUsed);

		if (InDeviceMemoryAllocation->IsMapped())
		{
			mSubresourceAllocatorFlags |= VulkanAllocationFlagsMapped;
		}
		else
		{
			mSubresourceAllocatorFlags &= ~VulkanAllocationFlagsMapped;
		}

		Range FullRange;
		FullRange.mOffset = 0;
		FullRange.mSize = mMaxSize;
		mFreeList.push_back(FullRange);
	}

	VulkanSubResourceAllocator::VulkanSubResourceAllocator(VulkanAllocationType InType, VulkanMemoryManager* InOwner, uint8 InSubResourceAllocatorFlags, VulkanDeviceMemoryAllocation* InDeviceMemoryAllocation,
		uint32 InMemoryTypeIndex, uint32 BufferId)
		: mType(InType)
		, mOwner(InOwner)
		, mMemoryTypeIndex(InMemoryTypeIndex)
		, mMemoryPropertyFlags(0)
		, mMemoryAllocation(InDeviceMemoryAllocation)
		, mAlignment(0)
		, mFrameFreed(0)
		, mUsedSize(0)
		, mBufferUsageFlags(0)
		, mBuffer(VK_NULL_HANDLE)
		, mBufferId(BufferId)
		, mPoolSizeIndex(0x7fffffff)
		, mAllocatorIndex(0xffffffff)
		, mSubresourceAllocatorFlags(InSubResourceAllocatorFlags)
	{
		Memory::MemoryZero(mMemoryUsed);
		mMaxSize = InDeviceMemoryAllocation->GetSize();

		if (InDeviceMemoryAllocation->IsMapped())
		{
			mSubresourceAllocatorFlags |= VulkanAllocationFlagsMapped;
		}
		else
		{
			mSubresourceAllocatorFlags &= ~VulkanAllocationFlagsMapped;
		}

		Range FullRange;
		FullRange.mOffset = 0;
		FullRange.mSize = mMaxSize;
		mFreeList.push_back(FullRange);
	}

	VulkanSubResourceAllocator::~VulkanSubResourceAllocator()
	{
		if (!JoinFreeBlocks())
		{
			LOG_WARN("LogVulkanRHI: FVulkanSubresourceAllocator  has unfreed  resources");
			uint32 LeakCount = 0;
			for (VulkanAllocationInternal& Data : mInternalData)
			{
				if (Data.mState == VulkanAllocationInternal::ALLOCATED)
				{
					LOG_WARN("LogVulkanRHI: **Memory Leak ");
				}
			}
		}
	}

	void VulkanSubResourceAllocator::Destroy(VulkanDevice* Device)
	{
		// Does not need to go in the deferred deletion queue
		if (mBuffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(Device->GetDevice(), mBuffer, VULKAN_CPU_ALLOCATOR);
			mBuffer = VK_NULL_HANDLE;
		}
	}

	bool VulkanSubResourceAllocator::TryAllocate2(VulkanAllocation& OutAllocation, VulkanEvictable* Owner, uint32 InSize, uint32 InAlignment, VulkanAllocationMetaType InMetaType, const char* File, uint32 Line)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		if (mbIsEvicting || mbLocked)
		{
			return false;
		}

		InAlignment = Math::max(InAlignment, mAlignment);
		for (int32 Index = 0; Index < mFreeList.size(); ++Index)
		{
			Range& Entry = mFreeList[Index];
			uint32 AllocatedOffset = Entry.mOffset;
			uint32 AlignedOffset = Align(Entry.mOffset, InAlignment);
			uint32 AlignmentAdjustment = AlignedOffset - Entry.mOffset;
			uint32 AllocatedSize = AlignmentAdjustment + InSize;
			if (AllocatedSize <= Entry.mSize)
			{
				Range::AllocateFromEntry(mFreeList, Index, AllocatedSize);

				mUsedSize += AllocatedSize;
				int32 ExtraOffset = AllocateInternalData();
				OutAllocation.Init(mType, InMetaType, (uint64)mBuffer, InSize, AlignedOffset, GetAllocatorIndex(), ExtraOffset, mBufferId);
				mMemoryUsed[InMetaType] += AllocatedSize;
				static uint32 UIDCounter = 0;
				UIDCounter++;
				mInternalData[ExtraOffset].Init(OutAllocation, Owner, AllocatedOffset, AllocatedSize, InAlignment);

				mAllocCalls++;
				mNumSubAllocations++;

				mbIsDefragging = false;
				return true;
			}
		}
		return false;
	}

	void VulkanSubResourceAllocator::Free(VulkanAllocation& Allocation)
	{
		bool bTryFree = false;
		{
			std::lock_guard<std::recursive_mutex> Lock(mMtx);
			mFreeCalls++;
			uint32 AllocationOffset;
			uint32 AllocationSize;
			{
				VulkanAllocationInternal& Data = mInternalData[Allocation.mAllocationIndex];
				bool bWasDiscarded = Data.mState == VulkanAllocationInternal::FREEDISCARDED;

				AllocationOffset = Data.mAllocationOffset;
				AllocationSize = Data.mAllocationSize;
				if (!bWasDiscarded)
				{
					mMemoryUsed[Allocation.mMetaType] -= AllocationSize;

				}
				Data.mState = VulkanAllocationInternal::FREED;
				FreeInternalData(Allocation.mAllocationIndex);
				Allocation.mAllocationIndex = -1;
				if (bWasDiscarded)
				{
					//this occurs if we do full defrag when there are pending frees. in that case the memory is just not moved to the new block.
					return;
				}
			}
			Range NewFree;
			NewFree.mOffset = AllocationOffset;
			NewFree.mSize = AllocationSize;

			Range::Add(mFreeList, NewFree);
			mUsedSize -= AllocationSize;
			mNumSubAllocations--;
			if (JoinFreeBlocks())
			{
				bTryFree = true; //cannot free here as it will cause incorrect lock ordering
			}
		}

		if (bTryFree)
		{
			mOwner->ReleaseSubresourceAllocator(this);
		}
	}

	void VulkanSubResourceAllocator::Flush(VkDeviceSize Offset, VkDeviceSize AllocationSize)
	{
		mMemoryAllocation->FlushMappedMemory(Offset, AllocationSize);
	}

	void VulkanSubResourceAllocator::Invalidate(VkDeviceSize Offset, VkDeviceSize AllocationSize)
	{
		mMemoryAllocation->InvalidateMappedMemory(Offset, AllocationSize);
	}


	std::vector<uint32> VulkanSubResourceAllocator::GetMemoryUsed()
	{
		std::vector<uint32> Result;
		for (int i = 0; i < VulkanAllocationMetaSize; ++i)
		{
			Result.push_back(mMemoryUsed[i]);
		}
		return Result;
	}

	uint32 VulkanSubResourceAllocator::GetNumSubAllocations()
	{
		return mNumSubAllocations;
	}

	int32 VulkanSubResourceAllocator::DefragTick(VulkanDevice& Device, VulkanCommandContext& Context, VulkanResourceHeap* Heap, uint32 Count_)
	{
		mLastDefragFrame = 1;
		int32 DefragCount = 0;
		int32 Count = (int32)Count_;
		mbLocked = true;

		auto PrintFreeList = [&](int32 id)
			{
				uint32 mmin = 0xffffffff;
				uint32 mmax = 0x0;
				for (Range& R : mFreeList)
				{
					mmin = Math::min(R.mSize, mmin);
					mmax = Math::max(R.mSize, mmax);
				}
			};

		std::lock_guard<std::recursive_mutex> Lock(mMtx);

		//Search for allocations to move to different pages.
		for (VulkanAllocationInternal& Alloc : mInternalData)
		{
			if (Alloc.mState == VulkanAllocationInternal::ALLOCATED)
			{
				VulkanEvictable* EvictableOwner = Alloc.mAllocationOwner;
				switch (Alloc.mMetaType)
				{
				case VulkanAllocationMetaImageRenderTarget: //only rendertargets can be defragged
				{
					VulkanAllocation Allocation;
					// The current SubAllocator is tagged as locked, this will never allocate in the current SubAllocator.
					if (Heap->TryRealloc(Allocation, EvictableOwner, VulkanMemoryType::Image, Alloc.mSize, Alloc.mAlignment, Alloc.mMetaType))
					{
						VulkanTexture* Texture = EvictableOwner->GetEvictableTexture();

						//Move the Rendertarget to the new allocation
						//Function swaps the old allocation into the Allocation object
						Texture->Move(Device, Context, Allocation);
						DefragCount++;
						Device.GetMemoryManager()->FreeVulkanAllocation(Allocation);
					}
					else
					{
						mbLocked = false;
						return DefragCount;
					}

				}
				break;
				default:
					break;
				}
				if (0 >= --Count)
				{
					break;
				}
			}
		}
		mbLocked = false;
		return DefragCount;
	}

	bool VulkanSubResourceAllocator::CanDefrag()
	{
		for (VulkanAllocationInternal& Alloc : mInternalData)
		{
			if (Alloc.mState == VulkanAllocationInternal::ALLOCATED)
			{
				VulkanEvictable* EvictableOwner = Alloc.mAllocationOwner;
				if ((EvictableOwner == nullptr) || !EvictableOwner->CanMove())
				{
					return false;
				}
			}
		}
		return true;
	}

	uint64 VulkanSubResourceAllocator::EvictToHost(VulkanDevice& Device, VulkanCommandContext& Context)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		mbIsEvicting = true;
		for (VulkanAllocationInternal& Alloc : mInternalData)
		{
			if (Alloc.mState == VulkanAllocationInternal::ALLOCATED)
			{
				switch (Alloc.mMetaType)
				{
				case VulkanAllocationMetaImageOther:
				{
					VulkanEvictable* Texture = Alloc.mAllocationOwner;
					Texture->Evict(Device, Context);
				}
				break;
				default:
					//right now only there is only support for evicting non-rt images
					break;
				}
			}

		}
		return mMemoryAllocation->GetSize();
	}

	void VulkanSubResourceAllocator::SetFreePending(VulkanAllocation& Allocation)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		VulkanAllocationInternal& Data = mInternalData[Allocation.mAllocationIndex];
		Data.mState = VulkanAllocationInternal::FREEPENDING;
	}

	void VulkanSubResourceAllocator::FreeInternalData(int32 Index)
	{
		mInternalData[Index].mNextFree = mInternalFreeList;
		mInternalFreeList = Index;
		mInternalData[Index].mState = VulkanAllocationInternal::UNUSED;
	}

	int32 VulkanSubResourceAllocator::AllocateInternalData()
	{
		int32 FreeListHead = mInternalFreeList;
		if (FreeListHead < 0)
		{
			mInternalData.push_back({});
			mInternalData[mInternalData.size() - 1].mNextFree = -1;
			return mInternalData.size() - 1;

		}
		else
		{
			mInternalFreeList = mInternalData[FreeListHead].mNextFree;
			mInternalData[FreeListHead].mNextFree = -1;
			return FreeListHead;
		}
	}

	bool VulkanSubResourceAllocator::JoinFreeBlocks()
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		if (mFreeList.size() == 1)
		{
			if (mNumSubAllocations == 0)
			{
				return true;
			}
		}
		return false;
	}

	static uint32 CalculateBufferAlignmentFromVKUsageFlags(VulkanDevice& InDevice, const VkBufferUsageFlags BufferUsageFlags)
	{
		const VkPhysicalDeviceLimits& Limits = InDevice.GetPhyDevice()->GetLimits();

		const bool bIsTexelBuffer = VKHasAnyFlags(BufferUsageFlags, (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT));
		const bool bIsStorageBuffer = VKHasAnyFlags(BufferUsageFlags, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		const bool bIsVertexOrIndexBuffer = VKHasAnyFlags(BufferUsageFlags, (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT));
		const bool bIsAccelerationStructureBuffer = VKHasAnyFlags(BufferUsageFlags, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);
		const bool bIsUniformBuffer = VKHasAnyFlags(BufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

		uint32 Alignment = 1;

		if (bIsTexelBuffer || bIsStorageBuffer)
		{
			Alignment = Math::max(Alignment, (uint32)Limits.minTexelBufferOffsetAlignment);
			Alignment = Math::max(Alignment, (uint32)Limits.minStorageBufferOffsetAlignment);
		}
		else if (bIsVertexOrIndexBuffer)
		{
			// No alignment restrictions on Vertex or Index buffers, leave it at 1
		}
		else if (bIsAccelerationStructureBuffer)
		{
			Alignment = Math::max(Alignment, 0u);
		}
		else if (bIsUniformBuffer)
		{
			Alignment = Math::max(Alignment, (uint32)Limits.minUniformBufferOffsetAlignment);
		}
		else
		{
			
		}

		return Alignment;
	}

	VulkanDeviceMemoryAllocation::~VulkanDeviceMemoryAllocation()
	{

	}

	void* VulkanDeviceMemoryAllocation::Map(VkDeviceSize Size, VkDeviceSize Offset)
	{
		assert(!IsMapped());
		if (!mMappedPointer)
		{
			vkMapMemory(mDeviceHandle, mHandle, Offset, Size, 0, &mMappedPointer);
		}
		return mMappedPointer;
	}

	void VulkanDeviceMemoryAllocation::Unmap()
	{
		if (mMappedPointer)
		{
			vkUnmapMemory(mDeviceHandle, mHandle);
			mMappedPointer = nullptr;
		}
	}

	static int32 GForceCoherent = 0;

	void VulkanDeviceMemoryAllocation::FlushMappedMemory(VkDeviceSize InOffset, VkDeviceSize InSize)
	{
		if (!IsCoherent() || GForceCoherent != 0)
		{
			assert(IsMapped());
			assert(InOffset + InSize <= mSize);
			VkMappedMemoryRange Range;
			Memory::MemoryZero(Range);
			Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			Range.memory = mHandle;
			Range.offset = InOffset;
			Range.size = InSize;
			vkFlushMappedMemoryRanges(mDeviceHandle, 1, &Range);
		}
	}

	void VulkanDeviceMemoryAllocation::InvalidateMappedMemory(VkDeviceSize InOffset, VkDeviceSize InSize)
	{
		if (!IsCoherent() || GForceCoherent != 0)
		{
			assert(IsMapped());
			assert(InOffset + InSize <= mSize);
			VkMappedMemoryRange Range;
			Memory::MemoryZero(Range);
			Range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			Range.memory = mHandle;
			Range.offset = InOffset;
			Range.size = InSize;
			vkInvalidateMappedMemoryRanges(mDeviceHandle, 1, &Range);
		}
	}

	VulkanDeviceMemoryManager::VulkanDeviceMemoryManager()
		: mMemoryUpdateTime(.0)
		, mDeviceHandle(nullptr)
		, mDevice(nullptr)
		, mNumAllocations(0)
		, mPeakNumAllocations(0)
		, mbHasUnifiedMemory(false)
		, mbSupportsMemoryless(false)
		, mPrimaryHeapIndex(-1)
	{
		Memory::MemoryZero(mMemoryBudget);
		Memory::MemoryZero(mMemoryProperties);

	}	
	
	VulkanDeviceMemoryManager::~VulkanDeviceMemoryManager()
	{
		Deinit();
	}	 

	void VulkanDeviceMemoryManager::Init(VulkanDevice* InDevice)
	{
		mDevice = InDevice;
		mNumAllocations = 0;
		mPeakNumAllocations = 0;
		mbHasUnifiedMemory = true;
		mDeviceHandle = mDevice->GetDevice();
		UpdateMemoryProperties();
		mPrimaryHeapIndex = -1;

		uint64 PrimaryHeapSize = 0;
		uint32 NonLocalHeaps = 0;

		for (uint32 i = 0; i < mMemoryProperties.memoryHeapCount; ++i)
		{

			if (VKHasAllFlags(mMemoryProperties.memoryHeaps[i].flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
			{
				if (mMemoryProperties.memoryHeaps[i].size > PrimaryHeapSize)
				{
					mPrimaryHeapIndex = i;
					PrimaryHeapSize = mMemoryProperties.memoryHeaps[i].size;
				}
			}
			else
			{
				NonLocalHeaps++;
			}
		}
		if (0 == NonLocalHeaps)
		{
			mPrimaryHeapIndex = -1; // if there are no non-local heaps, disable eviction and defragmentation
		}

		// Update bMemoryless support
		mbSupportsMemoryless = false;
		for (uint32 i = 0; i < mMemoryProperties.memoryTypeCount && !mbSupportsMemoryless; ++i)
		{
			mbSupportsMemoryless = VKHasAllFlags(mMemoryProperties.memoryTypes[i].propertyFlags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
		}

		//HeapInfos.AddDefaulted(mMemoryProperties.memoryHeapCount);

		//PrintMemInfo();
	}

	void VulkanDeviceMemoryManager::Deinit()
	{
		TrimMemory(true);

		mNumAllocations = 0;
	}

	uint32 VulkanDeviceMemoryManager::GetEvictedMemoryProperties()
	{
		if (mDevice->GetVendorId() == GpuVendorId::Amd)
		{
			return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		}
		else if (mDevice->GetVendorId() == GpuVendorId::Nvidia)
		{
			return 0;
		}
		else
		{
			return 0;
		}
	}

	bool VulkanDeviceMemoryManager::SupportsMemoryType(VkMemoryPropertyFlags Properties) const
	{
		for (uint32 Index = 0; Index < mMemoryProperties.memoryTypeCount; ++Index)
		{
			if (mMemoryProperties.memoryTypes[Index].propertyFlags == Properties)
			{
				return true;
			}
		}
		return false;
	}

	void VulkanDeviceMemoryManager::GetPrimaryHeapStatus(uint64& OutAllocated, uint64& OutLimit)
	{
		if (mPrimaryHeapIndex < 0)
		{
			OutAllocated = 0;
			OutLimit = 1;
		}
		else
		{
			OutAllocated = mHeapInfos[mPrimaryHeapIndex].mUsedSize;
			if (mDevice->GetOptionalExtensions().HasMemoryBudget && (Root::instance()->getTimer()->getSeconds() - mMemoryUpdateTime >= 1.0))
			{
				mMemoryUpdateTime = Root::instance()->getTimer()->getSeconds();
				UpdateMemoryProperties();
			}
			OutLimit = GetBaseHeapSize(mPrimaryHeapIndex);
		}
	}

	VkResult VulkanDeviceMemoryManager::GetMemoryTypeFromProperties(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32* OutTypeIndex)
	{
		for (uint32 i = 0; i < mMemoryProperties.memoryTypeCount && TypeBits; i++)
		{
			if ((TypeBits & 1) == 1)
			{
				if ((mMemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties)
				{
					*OutTypeIndex = i;
					return VK_SUCCESS;
				}
			}
			TypeBits >>= 1;
		}

		// No memory types matched, return failure
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}

	VkResult VulkanDeviceMemoryManager::GetMemoryTypeFromPropertiesExcluding(uint32 TypeBits, VkMemoryPropertyFlags Properties, uint32 ExcludeTypeIndex, uint32* OutTypeIndex)
	{
		for (uint32 i = 0; i < mMemoryProperties.memoryTypeCount && TypeBits; i++)
		{
			if ((TypeBits & 1) == 1)
			{
				// Type is available, does it match user properties?
				if ((mMemoryProperties.memoryTypes[i].propertyFlags & Properties) == Properties && ExcludeTypeIndex != i)
				{
					*OutTypeIndex = i;
					return VK_SUCCESS;
				}
			}
			TypeBits >>= 1;
		}

		// No memory types matched, return failure
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}

	const VkPhysicalDeviceMemoryProperties& VulkanDeviceMemoryManager::GetMemoryProperties() const
	{
		return mMemoryProperties;
	}

	// bCanFail means an allocation failing is not a fatal error, just returns nullptr
	VulkanDeviceMemoryAllocation* VulkanDeviceMemoryManager::Alloc(bool bCanFail, VkDeviceSize AllocationSize, uint32 MemoryTypeIndex, void* DedicatedAllocateInfo, float Priority, bool bExternal, const char* File, uint32 Line)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (!DedicatedAllocateInfo)
		{
			DeviceMemoryBlockKey Key = { MemoryTypeIndex, AllocationSize };
			auto iter = mAllocations.find(Key);
			if (iter == mAllocations.end())
				iter = mAllocations.emplace(Key, DeviceMemoryBlock()).first;

			DeviceMemoryBlock& Block = iter->second;

			if (Block.Allocations.size() > 0)
			{
				DeviceMemoryBlock::FreeBlock Alloc = *Block.Allocations.begin();
				Block.Allocations.erase(Block.Allocations.begin());

				return Alloc.Allocation;
			}
		}
		VkMemoryAllocateInfo Info;
		Memory::MemoryZero(Info);
		Info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		Info.allocationSize = AllocationSize;
		Info.memoryTypeIndex = MemoryTypeIndex;

		VkMemoryPriorityAllocateInfoEXT Prio;
		Memory::MemoryZero(Prio);
		Prio.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
		Prio.priority = Priority;
		Info.pNext = &Prio;

		if (DedicatedAllocateInfo)
		{
			((VkMemoryDedicatedAllocateInfoKHR*)DedicatedAllocateInfo)->pNext = Info.pNext;
			Info.pNext = DedicatedAllocateInfo;
		}

		VkExportMemoryAllocateInfoKHR VulkanExportMemoryAllocateInfoKHR = {};
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
		VkExportMemoryWin32HandleInfoKHR VulkanExportMemoryWin32HandleInfoKHR = {};
#endif
		if (bExternal)
		{
			Memory::MemoryZero(VulkanExportMemoryAllocateInfoKHR);
			VulkanExportMemoryAllocateInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR;
#if ELAINE_PLATFORM == ELAINE_PLATFORM_WINDOWS
			Memory::MemoryZero(VulkanExportMemoryWin32HandleInfoKHR);
			VulkanExportMemoryWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR;
			VulkanExportMemoryWin32HandleInfoKHR.pNext = Info.pNext;
			VulkanExportMemoryWin32HandleInfoKHR.pAttributes = NULL;
			VulkanExportMemoryWin32HandleInfoKHR.dwAccess = GENERIC_ALL;
			VulkanExportMemoryWin32HandleInfoKHR.name = (LPCWSTR)nullptr;
			VulkanExportMemoryAllocateInfoKHR.pNext =  nullptr;
			VulkanExportMemoryAllocateInfoKHR.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
#else
			VulkanExportMemoryAllocateInfoKHR.pNext = Info.pNext;
			VulkanExportMemoryAllocateInfoKHR.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif
			Info.pNext = &VulkanExportMemoryAllocateInfoKHR;
		}

		VkDeviceMemory Handle;
		VkResult Result;

		{
			Result = vkAllocateMemory(mDeviceHandle, &Info, VULKAN_CPU_ALLOCATOR, &Handle);
		}

		if (Result == VK_ERROR_OUT_OF_DEVICE_MEMORY || Result == VK_ERROR_OUT_OF_HOST_MEMORY)
		{
			if (bCanFail)
			{
				LOG_WARN("VulkanRHI: Failed to allocate Device Memory");
				return nullptr;
			}
			const TCHAR* MemoryType = TEXT("?");
			switch (Result)
			{
			case VK_ERROR_OUT_OF_HOST_MEMORY: MemoryType = TEXT("Host"); break;
			case VK_ERROR_OUT_OF_DEVICE_MEMORY: MemoryType = TEXT("Local"); break;
			}
			LOG_FATAL("VulkanRHI: Out of  Memory");
		}

		VulkanDeviceMemoryAllocation* NewAllocation = new VulkanDeviceMemoryAllocation;
		NewAllocation->mDeviceHandle = mDeviceHandle;
		NewAllocation->mHandle = Handle;
		NewAllocation->mSize = AllocationSize;
		NewAllocation->mMemoryTypeIndex = MemoryTypeIndex;
		NewAllocation->mbCanBeMapped = VKHasAllFlags(mMemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		NewAllocation->mbIsCoherent = VKHasAllFlags(mMemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		NewAllocation->mbIsCached = VKHasAllFlags(mMemoryProperties.memoryTypes[MemoryTypeIndex].propertyFlags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
		NewAllocation->mbDedicatedMemory = DedicatedAllocateInfo != 0;
		++mNumAllocations;
		mPeakNumAllocations = Math::max(mNumAllocations, mPeakNumAllocations);

		if (mNumAllocations == mDevice->GetPhyDevice()->GetLimits().maxMemoryAllocationCount)
		{
			LOG_WARN("VulkanRHI: Hit Maximum # of allocations  reported by device!");
		}

		uint32 HeapIndex = mMemoryProperties.memoryTypes[MemoryTypeIndex].heapIndex;
		//mHeapInfos[HeapIndex].Allocations.push_back(NewAllocation);
		//mHeapInfos[HeapIndex].mUsedSize += AllocationSize;
		//mHeapInfos[HeapIndex].mPeakSize = Math::max(mHeapInfos[HeapIndex].mPeakSize, mHeapInfos[HeapIndex].mUsedSize);

		return NewAllocation;
	}

	VulkanDeviceMemoryAllocation* VulkanDeviceMemoryManager::Alloc(bool bCanFail, VkDeviceSize AllocationSize, uint32 MemoryTypeBits, VkMemoryPropertyFlags MemoryPropertyFlags, void* DedicatedAllocateInfo, float Priority, bool bExternal, const char* File, uint32 Line)
	{
		uint32 MemoryTypeIndex = ~0;
		GetMemoryTypeFromProperties(MemoryTypeBits, MemoryPropertyFlags, &MemoryTypeIndex);
		return Alloc(bCanFail, AllocationSize, MemoryTypeIndex, DedicatedAllocateInfo, Priority, bExternal, File, Line);
	}

	// Sets the Allocation to nullptr
	void VulkanDeviceMemoryManager::Free(VulkanDeviceMemoryAllocation*& Allocation)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (!Allocation->mbDedicatedMemory)
		{
			VkDeviceSize AllocationSize = Allocation->mSize;
			DeviceMemoryBlockKey Key = { Allocation->mMemoryTypeIndex, AllocationSize };
			auto iter = mAllocations.find(Key);
			if (iter == mAllocations.end())
				iter = mAllocations.emplace(Key, DeviceMemoryBlock()).first;
			DeviceMemoryBlock& Block = iter->second;
			DeviceMemoryBlock::FreeBlock FreeBlock = { Allocation, 1 };
			Block.Allocations.push_back(FreeBlock);
			Allocation = nullptr;
			return;
		}

		FreeInternal(Allocation);
		Allocation = nullptr;
	}

	void VulkanDeviceMemoryManager::FreeInternal(VulkanDeviceMemoryAllocation* Allocation)
	{
		vkFreeMemory(mDeviceHandle, Allocation->mHandle, VULKAN_CPU_ALLOCATOR);
		--mNumAllocations;

		//uint32 HeapIndex = mMemoryProperties.memoryTypes[Allocation->mMemoryTypeIndex].heapIndex;

		//mHeapInfos[HeapIndex].mUsedSize -= Allocation->mSize;
		//auto iter = std::find(mHeapInfos[HeapIndex].Allocations.begin(), mHeapInfos[HeapIndex].Allocations.end(), Allocation);
		//mHeapInfos[HeapIndex].Allocations.erase(iter);
		Allocation->mbFreedBySystem = true;

		delete Allocation;
		Allocation = nullptr;
	}

	void VulkanDeviceMemoryManager::TrimMemory(bool bFullTrim)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);

		const uint32 FrameThresholdFull = 100;
		// After being held for FrameThresholdPartial frames, only LargeThresholdPartial/SmallThresholdPartial pages are kept reserved
		// SmallPageSize defines when Large/Small is used.
		const uint32 FrameThresholdPartial = 10;
		const uint32 LargeThresholdPartial = 5;
		const uint32 SmallThresholdPartial = 10;
		const uint64 SmallPageSize = (8llu << 20);
		uint32 Frame = 1;

		for (auto&& Pair : mAllocations)
		{
			DeviceMemoryBlock& Block = Pair.second;

			const uint32 ThresholdPartial = Block.Key.BlockSize <= SmallPageSize ? SmallThresholdPartial : LargeThresholdPartial;

			uint32 AbovePartialThreshold = 0;
			int32 Index = 0;
			while (Index < Block.Allocations.size())
			{
				DeviceMemoryBlock::FreeBlock& FreeBlock = Block.Allocations[Index];
				if (FreeBlock.FrameFreed + FrameThresholdFull < Frame || bFullTrim)
				{
					FreeInternal(FreeBlock.Allocation);
					Block.Allocations.erase(Block.Allocations.begin() + Index);
					continue;
				}
				else if (FreeBlock.FrameFreed + FrameThresholdPartial < Frame)
				{
					AbovePartialThreshold++;
				}
				Index++;
			}

			if (AbovePartialThreshold > ThresholdPartial)
			{
				uint32 BlocksToFree = AbovePartialThreshold - ThresholdPartial;
				Index = 0;
				while (Index < Block.Allocations.size() && BlocksToFree > 0)
				{
					DeviceMemoryBlock::FreeBlock& FreeBlock = Block.Allocations[Index];
					if (FreeBlock.FrameFreed + FrameThresholdPartial < Frame)
					{

						FreeInternal(FreeBlock.Allocation);
						Block.Allocations.erase(Block.Allocations.begin() + Index);
						BlocksToFree--;
						continue;
					}
					else
					{
						Index++;
					}
				}
			}
		}

	}

	uint64 VulkanDeviceMemoryManager::GetTotalMemory(bool bGPU) const
	{
		uint64 TotalMemory = 0;
		for (uint32 Index = 0; Index < mMemoryProperties.memoryHeapCount; ++Index)
		{
			const bool bIsGPUHeap = ((mMemoryProperties.memoryHeaps[Index].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);

			if (bIsGPUHeap == bGPU)
			{
				TotalMemory += mMemoryProperties.memoryHeaps[Index].size;
			}
		}
		return TotalMemory;
	}

	VkDeviceSize VulkanDeviceMemoryManager::GetBaseHeapSize(uint32 HeapIndex) const
	{
		VkDeviceSize HeapSize = mMemoryProperties.memoryHeaps[HeapIndex].size;
		return HeapSize;
	}

	uint32 VulkanDeviceMemoryManager::GetHeapIndex(uint32 MemoryTypeIndex)
	{
		return mMemoryProperties.memoryTypes[MemoryTypeIndex].heapIndex;
	}

	void VulkanDeviceMemoryManager::UpdateMemoryProperties()
	{
		if (mDevice->GetOptionalExtensions().HasMemoryBudget)
		{
			VkPhysicalDeviceMemoryProperties2 MemoryProperties2;
			Memory::MemoryZero(MemoryProperties2);
			Memory::MemoryZero(mMemoryBudget);
			mMemoryBudget.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
			MemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
			MemoryProperties2.pNext = &mMemoryBudget;

			vkGetPhysicalDeviceMemoryProperties2(mDevice->GetPhyDevice()->GetPhysicalDeviceHandle(), &MemoryProperties2);
			Memory::MemoryCopy(mMemoryProperties, MemoryProperties2.memoryProperties);

			for (uint32 Heap = 0; Heap < VK_MAX_MEMORY_HEAPS; ++Heap)
			{
				mMemoryBudget.heapBudget[Heap] = GVulkanBudgetPercentageScale * mMemoryBudget.heapBudget[Heap] / 100;
			}

			VkDeviceSize BudgetX = 0;
			for (uint32 Heap = 6; Heap < VK_MAX_MEMORY_HEAPS; ++Heap)
			{
				BudgetX += mMemoryBudget.heapBudget[Heap];
			}
		}
		else
		{
			vkGetPhysicalDeviceMemoryProperties(mDevice->GetPhyDevice()->GetPhysicalDeviceHandle(), &mMemoryProperties);
		}
	}

	VulkanMemoryManager::VulkanMemoryManager(VulkanDevice* InDeivce)
		: mDeviceMemoryManager(InDeivce->GetDeviceMemoryManager())
		, mDeivce(InDeivce)
	{

	}

	VulkanMemoryManager::~VulkanMemoryManager()
	{
		//Deinit();
	}

	void VulkanMemoryManager::Init()
	{
		const uint32 TypeBits = (1 << mDeviceMemoryManager->GetNumMemoryTypes()) - 1;
		const VkPhysicalDeviceMemoryProperties& MemoryProperties = mDeviceMemoryManager->GetMemoryProperties();
		mResourceTypeHeaps.resize(MemoryProperties.memoryTypeCount);

		// Upload heap. Spec requires this combination to exist.
		{
			uint32 TypeIndex = 0;
			mDeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &TypeIndex);
			uint64 HeapSize = MemoryProperties.memoryHeaps[MemoryProperties.memoryTypes[TypeIndex].heapIndex].size;
			mResourceTypeHeaps[TypeIndex] = new VulkanResourceHeap(this, TypeIndex, STAGING_HEAP_PAGE_SIZE);

			auto& PageSizeBuckets = mResourceTypeHeaps[TypeIndex]->mPageSizeBuckets;
			VulkanPageSizeBucket Bucket0 = { STAGING_HEAP_PAGE_SIZE, STAGING_HEAP_PAGE_SIZE, VulkanPageSizeBucket::BUCKET_MASK_IMAGE | VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
			VulkanPageSizeBucket Bucket1 = { UINT64_MAX, 0, VulkanPageSizeBucket::BUCKET_MASK_IMAGE | VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
			PageSizeBuckets.push_back(Bucket0);
			PageSizeBuckets.push_back(Bucket1);
		}

		// Download heap. Optional type per the spec.
		{
			uint32 TypeIndex = 0;
			{
				uint32 HostVisCachedIndex = 0;
				VkResult HostCachedResult = mDeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, &HostVisCachedIndex);
				uint32 HostVisIndex = 0;
				VkResult HostResult = mDeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &HostVisIndex);
				if (HostCachedResult == VK_SUCCESS)
				{
					TypeIndex = HostVisCachedIndex;
				}
				else if (HostResult == VK_SUCCESS)
				{
					TypeIndex = HostVisIndex;
				}
				else
				{
					// Redundant as it would have asserted above...
					LOG_FATAL("LogVulkanRHI: No Memory Type found supporting VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT!");
				}
			}
			uint64 HeapSize = MemoryProperties.memoryHeaps[MemoryProperties.memoryTypes[TypeIndex].heapIndex].size;
			mResourceTypeHeaps[TypeIndex] = new VulkanResourceHeap(this, TypeIndex, STAGING_HEAP_PAGE_SIZE);

			auto& PageSizeBuckets = mResourceTypeHeaps[TypeIndex]->mPageSizeBuckets;
			VulkanPageSizeBucket Bucket0 = { STAGING_HEAP_PAGE_SIZE, STAGING_HEAP_PAGE_SIZE, VulkanPageSizeBucket::BUCKET_MASK_IMAGE | VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
			VulkanPageSizeBucket Bucket1 = { UINT64_MAX, 0, VulkanPageSizeBucket::BUCKET_MASK_IMAGE | VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
			PageSizeBuckets.push_back(Bucket0);
			PageSizeBuckets.push_back(Bucket1);
		}

		// Setup main GPU heap
		{
			uint32 Index;
			mDeviceMemoryManager->GetMemoryTypeFromProperties(TypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &Index);

			for (Index = 0; Index < MemoryProperties.memoryTypeCount; ++Index)
			{
				const int32 HeapIndex = MemoryProperties.memoryTypes[Index].heapIndex;
				const VkDeviceSize HeapSize = MemoryProperties.memoryHeaps[HeapIndex].size;
				if (!mResourceTypeHeaps[Index])
				{
					mResourceTypeHeaps[Index] = new VulkanResourceHeap(this, Index);
					mResourceTypeHeaps[Index]->mbIsHostCachedSupported = VKHasAllFlags(MemoryProperties.memoryTypes[Index].propertyFlags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
					mResourceTypeHeaps[Index]->mbIsLazilyAllocatedSupported = VKHasAllFlags(MemoryProperties.memoryTypes[Index].propertyFlags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
					auto& PageSizeBuckets = mResourceTypeHeaps[Index]->mPageSizeBuckets;

#if ELAINE_PLATFORM == ELAINE_PLATFORM_ANDROID
					VulkanPageSizeBucket BucketImage = { UINT64_MAX, (uint32)ANDROID_MAX_HEAP_IMAGE_PAGE_SIZE, VulkanPageSizeBucket::BUCKET_MASK_IMAGE };
					VulkanPageSizeBucket BucketBuffer = { UINT64_MAX, (uint32)ANDROID_MAX_HEAP_BUFFER_PAGE_SIZE, VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
					mPageSizeBuckets.push_back(BucketImage);
					mPageSizeBuckets.push_back(BucketBuffer);
#else
					uint32 SmallAllocationThreshold = 2 << 20;
					uint32 LargeAllocationThreshold = 64llu << 20llu;
					VkDeviceSize SmallPageSize = 8llu << 20;
					VkDeviceSize LargePageSize = Math::min<VkDeviceSize>(HeapSize / 8, GPU_ONLY_HEAP_PAGE_SIZE);


					VulkanPageSizeBucket BucketSmallImage = { SmallAllocationThreshold, (uint32)SmallPageSize, VulkanPageSizeBucket::BUCKET_MASK_IMAGE };
					VulkanPageSizeBucket BucketLargeImage = { LargeAllocationThreshold, (uint32)LargePageSize, VulkanPageSizeBucket::BUCKET_MASK_IMAGE };
					VulkanPageSizeBucket BucketSmallBuffer = { SmallAllocationThreshold, (uint32)SmallPageSize, VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
					VulkanPageSizeBucket BucketLargeBuffer = { LargeAllocationThreshold, (uint32)LargePageSize, VulkanPageSizeBucket::BUCKET_MASK_BUFFER };
					VulkanPageSizeBucket BucketRemainder = { UINT64_MAX, 0, VulkanPageSizeBucket::BUCKET_MASK_BUFFER | VulkanPageSizeBucket::BUCKET_MASK_IMAGE };
					PageSizeBuckets.push_back(BucketSmallImage);
					PageSizeBuckets.push_back(BucketLargeImage);
					PageSizeBuckets.push_back(BucketSmallBuffer);
					PageSizeBuckets.push_back(BucketLargeBuffer);
					PageSizeBuckets.push_back(BucketRemainder);
#endif
				}
			}
		}
	}

	void VulkanMemoryManager::Deinit()
	{
		{
			ProcessPendingUBFreesNoLock(true);
		}

		DestroyResourceAllocations();

		for (int32 Index = 0; Index < mResourceTypeHeaps.size(); ++Index)
		{
			delete mResourceTypeHeaps[Index];
			mResourceTypeHeaps[Index] = nullptr;
		}
		mResourceTypeHeaps.clear();
	}

	uint32 VulkanMemoryManager::CalculateBufferAlignment(VulkanDevice& InDevice, BufferUsageFlags InUEUsage, bool bZeroSize)
	{
		const VkBufferUsageFlags VulkanBufferUsage = RHIToVKBufferUsageFlags(&InDevice, InUEUsage, bZeroSize);

		uint32 Alignment = CalculateBufferAlignmentFromVKUsageFlags(InDevice, VulkanBufferUsage);

		return Alignment;
	}

	float VulkanMemoryManager::CalculateBufferPriority(const VkBufferUsageFlags BufferUsageFlags)
	{
		float Priority = 0.5f;

		if (VKHasAnyFlags(BufferUsageFlags, (VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT)))
		{
			Priority = 1.0f;
		}
		else if (VKHasAnyFlags(BufferUsageFlags, (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)))
		{
			Priority = 0.5f;
		}
		else if (VKHasAnyFlags(BufferUsageFlags,
			(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
				VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
				VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)))
		{
			Priority = 0.5f;
		}
		else if (VKHasAnyFlags(BufferUsageFlags, (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)))
		{
			Priority = 0.0f;
		}
		else if (VKHasAnyFlags(BufferUsageFlags, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
		{
			Priority = 0.75f;
		}

		return Priority;
	}

	void VulkanMemoryManager::FreeVulkanAllocation(VulkanAllocation& Allocation, VulkanFreeFlags FreeFlags)
	{
		if (FreeFlags & VulkanFreeFlag_DontDefer)
		{
			switch (Allocation.mType)
			{
			case VulkanAllocationTypeEmpty:
				break;
			case VulkanAllocationPooledBuffer:
				FreeVulkanAllocationPooledBuffer(Allocation);
				break;
			case VulkanAllocationBuffer:
				FreeVulkanAllocationBuffer(Allocation);
				break;
			case VulkanAllocationImage:
				FreeVulkanAllocationImage(Allocation);
				break;
			case VulkanAllocationImageDedicated:
				FreeVulkanAllocationImageDedicated(Allocation);
				break;
			default:
				break;
			}
			Memory::MemoryZero(&Allocation, sizeof(Allocation));
			Allocation.mType = VulkanAllocationTypeEmpty;
		}
		else
		{
			GetSubresourceAllocator(Allocation.mAllocatorIndex)->SetFreePending(Allocation);
			mDeivce->GetDeferredDeletionQueue().EnqueueResourceAllocation(Allocation);
		}
	}

	bool VulkanMemoryManager::UpdateEvictThreshold(bool bLog)
	{
		uint64 PrimaryAllocated, PrimaryLimit;
		mDeviceMemoryManager->GetPrimaryHeapStatus(PrimaryAllocated, PrimaryLimit);
		double AllocatedPercentage = 100.0 * (PrimaryAllocated - mPendingEvictBytes) / PrimaryLimit;

		double EvictionLimit = 80.0f;
		double EvictionLimitLowered = EvictionLimit * (75.0f / 100.f);
		if (mbIsEvicting) //once eviction is started, further lower the limit, to avoid reclaiming memory we just free up
		{
			EvictionLimit = EvictionLimitLowered;
		}

		mbIsEvicting = AllocatedPercentage > EvictionLimit;
		return mbIsEvicting;
	}

	void VulkanMemoryManager::FreeVulkanAllocationPooledBuffer(VulkanAllocation& Allocation)
	{
		const uint32 Index = Allocation.mAllocatorIndex;
		GetSubresourceAllocator(Index)->Free(Allocation);
	}

	void VulkanMemoryManager::FreeVulkanAllocationBuffer(VulkanAllocation& Allocation)
	{
		const uint32 Index = Allocation.mAllocatorIndex;
		GetSubresourceAllocator(Index)->Free(Allocation);
	}

	void VulkanMemoryManager::FreeVulkanAllocationImage(VulkanAllocation& Allocation)
	{
		const uint32 Index = Allocation.mAllocatorIndex;
		GetSubresourceAllocator(Index)->Free(Allocation);
	}

	void VulkanMemoryManager::FreeVulkanAllocationImageDedicated(VulkanAllocation& Allocation)
	{
		const uint32 Index = Allocation.mAllocatorIndex;
		GetSubresourceAllocator(Index)->Free(Allocation);
	}

	bool VulkanMemoryManager::AllocateBufferPooled(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, uint32 Size, uint32 MinAlignment, VkBufferUsageFlags BufferUsageFlags, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, const char* File, uint32 Line)
	{
		uint32 Alignment = Math::max(MinAlignment, CalculateBufferAlignmentFromVKUsageFlags(*mDeivce, BufferUsageFlags));
		const float Priority = CalculateBufferPriority(BufferUsageFlags);

		const int32 PoolSize = (int32)GetPoolTypeForAlloc(Size, Alignment);

		if (PoolSize != (int32)MemBlockSize::MTCount)
		{
			Size = PoolSizes[PoolSize];
		}

		std::lock_guard<std::recursive_mutex> lock(mMtx);

		for (uint32 Index = 0; Index < mUsedBufferAllocations[PoolSize].size(); ++Index)
		{
			VulkanSubResourceAllocator* SubresourceAllocator = mUsedBufferAllocations[PoolSize][Index];
			if ((SubresourceAllocator->mBufferUsageFlags & BufferUsageFlags) == BufferUsageFlags &&
				(SubresourceAllocator->mMemoryPropertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)
			{

				if (SubresourceAllocator->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line))
				{
					return true;
				}
			}
		}

		for (int32 Index = 0; Index < mFreeBufferAllocations[PoolSize].size(); ++Index)
		{
			VulkanSubResourceAllocator* SubresourceAllocator = mFreeBufferAllocations[PoolSize][Index];
			if ((SubresourceAllocator->mBufferUsageFlags & BufferUsageFlags) == BufferUsageFlags &&
				(SubresourceAllocator->mMemoryPropertyFlags & MemoryPropertyFlags) == MemoryPropertyFlags)
			{
				if (SubresourceAllocator->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line))
				{
					mFreeBufferAllocations[PoolSize].erase(mFreeBufferAllocations->begin() + Index);
					mUsedBufferAllocations[PoolSize].push_back(SubresourceAllocator);
					return true;
				}
			}
		}

		// New Buffer
		const uint32 BufferSize = Math::max(Size, BufferSizes[PoolSize]);

		VkBuffer Buffer;
		VkBufferCreateInfo BufferCreateInfo;
		Memory::MemoryZero(BufferCreateInfo);
		BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.size = BufferSize;
		BufferCreateInfo.usage = BufferUsageFlags;
		vkCreateBuffer(mDeivce->GetDevice(), &BufferCreateInfo, VULKAN_CPU_ALLOCATOR, &Buffer);

		VkMemoryRequirements MemReqs;
		vkGetBufferMemoryRequirements(mDeivce->GetDevice(), Buffer, &MemReqs);
		Alignment = Math::max(uint32(MemReqs.alignment), Alignment);
		uint32 MemoryTypeIndex;
		mDeivce->GetDeviceMemoryManager()->GetMemoryTypeFromProperties(MemReqs.memoryTypeBits, MemoryPropertyFlags, &MemoryTypeIndex);
		bool bHasUnifiedMemory = mDeviceMemoryManager->HasUnifiedMemory();

		VulkanDeviceMemoryAllocation* DeviceMemoryAllocation = mDeviceMemoryManager->Alloc(true, MemReqs.size, MemoryTypeIndex, nullptr, Priority, false, File, Line);
		if (!DeviceMemoryAllocation)
		{
			MemoryPropertyFlags &= (~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			const uint32 ForbiddenMemoryTypeIndex = MemoryTypeIndex;
			if (VK_SUCCESS == mDeivce->GetDeviceMemoryManager()->GetMemoryTypeFromPropertiesExcluding(MemReqs.memoryTypeBits, MemoryPropertyFlags, ForbiddenMemoryTypeIndex, &MemoryTypeIndex))
			{
				DeviceMemoryAllocation = mDeviceMemoryManager->Alloc(false, MemReqs.size, MemoryTypeIndex, nullptr, Priority, false, File, Line);
			}
		}

		vkBindBufferMemory(mDeivce->GetDevice(), Buffer, DeviceMemoryAllocation->GetHandle(), 0);
		uint8 AllocationFlags = 0;
		if (!bHasUnifiedMemory && MetaTypeCanEvict(MetaType))
		{
			AllocationFlags |= VulkanAllocationFlagsCanEvict;
		}
		if (DeviceMemoryAllocation->CanBeMapped())
		{
			DeviceMemoryAllocation->Map(BufferSize, 0);
		}

		uint32 BufferId = 0;
		VulkanSubResourceAllocator* SubresourceAllocator = new VulkanSubResourceAllocator(VulkanAllocationPooledBuffer, this, AllocationFlags, DeviceMemoryAllocation, MemoryTypeIndex,
			MemoryPropertyFlags, MemReqs.alignment, Buffer, BufferSize, BufferId, BufferUsageFlags, PoolSize);
		RegisterSubresourceAllocator(SubresourceAllocator);

		mUsedBufferAllocations[PoolSize].push_back(SubresourceAllocator);
		if (SubresourceAllocator->TryAllocate2(OutAllocation, AllocationOwner, Size, Alignment, MetaType, File, Line))
		{
			return true;
		}

		return false;
	}

	bool VulkanMemoryManager::AllocateImageMemory(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line)
	{
		const bool bHasUnifiedMemory = mDeviceMemoryManager->HasUnifiedMemory();
		const bool bCanEvict = MetaTypeCanEvict(MetaType);
		if (!bHasUnifiedMemory && bCanEvict && MemoryPropertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && UpdateEvictThreshold(false))
		{
			MemoryPropertyFlags = mDeviceMemoryManager->GetEvictedMemoryProperties();
		}
		bool bMapped = VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uint32 TypeIndex = 0;

		if (mDeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, &TypeIndex) != VK_SUCCESS)
		{
			if (VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT))
			{
				// If lazy allocations are not supported, we can fall back to real allocations.
				MemoryPropertyFlags = MemoryPropertyFlags & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
				mDeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, &TypeIndex);
			}
			else
			{
				LOG_FATAL("VulkanRHI: Cannot find memory type for MemSize");
			}
		}
		if (!mResourceTypeHeaps[TypeIndex])
		{
			LOG_FATAL("VulkanRHI: Missing memory type index");
		}

		const bool bForceSeparateAllocation = bExternal || VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);

		if (!mResourceTypeHeaps[TypeIndex]->AllocateResource(OutAllocation, AllocationOwner, VulkanMemoryType::Image, MemoryReqs.size, MemoryReqs.alignment, bMapped, bForceSeparateAllocation, MetaType, bExternal, File, Line))
		{
			mDeviceMemoryManager->GetMemoryTypeFromPropertiesExcluding(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, TypeIndex, &TypeIndex);
			bMapped = VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			if (!mResourceTypeHeaps[TypeIndex]->AllocateResource(OutAllocation, AllocationOwner, VulkanMemoryType::Image, MemoryReqs.size, MemoryReqs.alignment, bMapped, bForceSeparateAllocation, MetaType, bExternal, File, Line))
			{
				LOG_FATAL("VulkanRHI: Out Of Memory, trying to allocate ");
				return false;
			}
		}
		return false;
	}

	bool VulkanMemoryManager::AllocateBufferMemory(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line)
	{
		uint32 TypeIndex = 0;
		VkResult Result = mDeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, &TypeIndex);
		bool bMapped = VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		if ((Result != VK_SUCCESS) || !mResourceTypeHeaps[TypeIndex])
		{
			if (VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
			{
				// Try non-cached flag
				MemoryPropertyFlags = MemoryPropertyFlags & ~VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			}

			if (VKHasAllFlags(MemoryPropertyFlags, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT))
			{
				// Try non-lazy flag
				MemoryPropertyFlags = MemoryPropertyFlags & ~VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			}

			// Try another heap type
			uint32 OriginalTypeIndex = TypeIndex;
			if (mDeviceMemoryManager->GetMemoryTypeFromPropertiesExcluding(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, (Result == VK_SUCCESS) ? TypeIndex : (uint32)-1, &TypeIndex) != VK_SUCCESS)
			{
				LOG_FATAL("VulkanRHI: Unable to find alternate type for index");
			}
			if (!mResourceTypeHeaps[TypeIndex])
			{
				LOG_FATAL("VulkanRHI: Missing memory type index ");
			}
		}

		if (!mResourceTypeHeaps[TypeIndex]->AllocateResource(OutAllocation, AllocationOwner, VulkanMemoryType::Buffer, MemoryReqs.size, MemoryReqs.alignment, bMapped, false, MetaType, bExternal, File, Line))
		{
			// Try another memory type if the allocation failed
			mDeviceMemoryManager->GetMemoryTypeFromPropertiesExcluding(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, TypeIndex, &TypeIndex);
			bMapped = (MemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			if (!mResourceTypeHeaps[TypeIndex])
			{
				LOG_FATAL("VulkanRHI: Missing memory type index");
			}
			if (!mResourceTypeHeaps[TypeIndex]->AllocateResource(OutAllocation, AllocationOwner, VulkanMemoryType::Buffer, MemoryReqs.size, MemoryReqs.alignment, bMapped, false, MetaType, bExternal, File, Line))
			{
				LOG_FATAL("VulkanRHI: Out Of Memory, trying to allocate");
				return false;
			}
		}
		return true;
	}

	bool VulkanMemoryManager::AllocateDedicatedImageMemory(VulkanAllocation& OutAllocation, VulkanEvictable* AllocationOwner, VkImage Image, const VkMemoryRequirements& MemoryReqs, VkMemoryPropertyFlags MemoryPropertyFlags, VulkanAllocationMetaType MetaType, bool bExternal, const char* File, uint32 Line)
	{
		VkImageMemoryRequirementsInfo2KHR ImageMemoryReqs2;
		Memory::MemoryZero(ImageMemoryReqs2);
		ImageMemoryReqs2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
		ImageMemoryReqs2.image = Image;

		VkMemoryDedicatedRequirementsKHR DedMemoryReqs;
		Memory::MemoryZero(DedMemoryReqs);
		DedMemoryReqs.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;


		VkMemoryRequirements2KHR MemoryReqs2;
		Memory::MemoryZero(MemoryReqs2);
		MemoryReqs2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;
		MemoryReqs2.pNext = &DedMemoryReqs;

		VulkanRHI::vkGetImageMemoryRequirements2KHR(mDeivce->GetDevice(), &ImageMemoryReqs2, &MemoryReqs2);

		bool bUseDedicated = DedMemoryReqs.prefersDedicatedAllocation != VK_FALSE || DedMemoryReqs.requiresDedicatedAllocation != VK_FALSE;
		if (bUseDedicated)
		{
			uint32 TypeIndex = 0;
			mDeviceMemoryManager->GetMemoryTypeFromProperties(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, &TypeIndex);

			if (!mResourceTypeHeaps[TypeIndex])
			{
				LOG_FATAL("VulkanRHI: Missing memory type index ");
			}
			if (!mResourceTypeHeaps[TypeIndex]->AllocateDedicatedImage(OutAllocation, AllocationOwner, Image, MemoryReqs.size, MemoryReqs.alignment, MetaType, bExternal, File, Line))
			{

				if (VK_SUCCESS == mDeviceMemoryManager->GetMemoryTypeFromPropertiesExcluding(MemoryReqs.memoryTypeBits, MemoryPropertyFlags, TypeIndex, &TypeIndex))
				{
					if (mResourceTypeHeaps[TypeIndex]->AllocateDedicatedImage(OutAllocation, AllocationOwner, Image, MemoryReqs.size, MemoryReqs.alignment, MetaType, bExternal, File, Line))
					{
						return true;
					}
				}
				return false;
			}
			return true;
		}
		else
		{
			return AllocateImageMemory(OutAllocation, AllocationOwner, MemoryReqs, MemoryPropertyFlags, MetaType, bExternal, File, Line);
		}
	}

	void VulkanMemoryManager::RegisterSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		if (mAllBufferAllocationsFreeListHead != (uint64)- 1)
		{
			const uint32 Index = mAllBufferAllocationsFreeListHead;
			mAllBufferAllocationsFreeListHead = (uint64)mAllBufferAllocations[Index];
			SubresourceAllocator->mAllocatorIndex = Index;
			mAllBufferAllocations[Index] = SubresourceAllocator;
		}
		else
		{
			SubresourceAllocator->mAllocatorIndex = mAllBufferAllocations.size();
			mAllBufferAllocations.push_back(SubresourceAllocator);
		}
	}

	void VulkanMemoryManager::UnregisterSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator)
	{
		if (SubresourceAllocator->mbIsEvicting)
		{
			mPendingEvictBytes -= SubresourceAllocator->GetMemoryAllocation()->GetSize();
		}
		std::lock_guard<std::recursive_mutex> lock(mMtx);
		const uint32 Index = SubresourceAllocator->mAllocatorIndex;
		mAllBufferAllocations[Index] = (VulkanSubResourceAllocator*)mAllBufferAllocationsFreeListHead;
		mAllBufferAllocationsFreeListHead = Index;

	}

	bool VulkanMemoryManager::ReleaseSubresourceAllocator(VulkanSubResourceAllocator* SubresourceAllocator)
	{
		std::lock_guard<std::recursive_mutex> lock(mMtx);

		if (SubresourceAllocator->JoinFreeBlocks())
		{
			if (SubresourceAllocator->mType == VulkanAllocationPooledBuffer)
			{
				auto iter = std::find(mUsedBufferAllocations[SubresourceAllocator->mPoolSizeIndex].begin(), mUsedBufferAllocations[SubresourceAllocator->mPoolSizeIndex].end(), SubresourceAllocator);

				mUsedBufferAllocations[SubresourceAllocator->mPoolSizeIndex].erase(iter);
				SubresourceAllocator->mFrameFreed = 1;
				mFreeBufferAllocations[SubresourceAllocator->mPoolSizeIndex].push_back(SubresourceAllocator);
			}
			else
			{
				VulkanResourceHeap* Heap = mResourceTypeHeaps[SubresourceAllocator->mMemoryTypeIndex];
				Heap->FreePage(SubresourceAllocator);
			}

			return true;
		}
		return false;
	}

	void VulkanMemoryManager::AllocUniformBuffer(VulkanAllocation& OutAllocation, uint32 Size, const void* Contents)
	{
		if (!AllocateBufferPooled(OutAllocation, nullptr, Size, 0, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VulkanAllocationMetaUniformBuffer, __FILE__, __LINE__))
		{

		}

		if (Contents)
		{
			Memory::MemoryCopy(OutAllocation.GetMappedPointer(mDeivce), Contents, Size);
			OutAllocation.FlushMappedMemory(mDeivce);
		}
	}

	void VulkanMemoryManager::FreeUniformBuffer(VulkanAllocation& InAllocation)
	{
		if (InAllocation.HasAllocation())
		{
			std::lock_guard<std::recursive_mutex> Lock(mUBAllocations.mMtx);
			ProcessPendingUBFreesNoLock(false);
			FUBPendingFree& Pending = mUBAllocations.mPendingFree.emplace_back(FUBPendingFree());

			Pending.mFrame = 1;
			Pending.mAllocation.Swap(InAllocation);
			mUBAllocations.mPeak = Math::max(mUBAllocations.mPeak, (uint32)mUBAllocations.mPendingFree.size());
		}
	}

	int32 GVulkanEvictOnePage = 0;

	void VulkanMemoryManager::ReleaseFreedPages(VulkanCommandContext& Context)
	{
		auto CanDefragHeap = [](VulkanResourceHeap* Heap)
			{

				for (std::vector<VulkanSubResourceAllocator*>& Pages : Heap->mActivePages)
				{
					for (VulkanSubResourceAllocator* Allocator : Pages)
					{
						if (0 == (Allocator->GetSubresourceAllocatorFlags() & VulkanAllocationFlagsCanEvict) && Allocator->CanDefrag())
						{
							return true;
						}
					}
				}
				return false;
			};

		auto CanEvictHeap = [](VulkanResourceHeap* Heap)
			{

				for (std::vector<VulkanSubResourceAllocator*>& Pages : Heap->mActivePages)
				{
					for (VulkanSubResourceAllocator* Allocator : Pages)
					{
						if (VulkanAllocationFlagsCanEvict == (Allocator->GetSubresourceAllocatorFlags() & VulkanAllocationFlagsCanEvict))
						{
							return true;
						}
					}
				}
				return false;
			};

		ReleaseFreedResources(false);

		const int32 PrimaryHeapIndex = mDeviceMemoryManager->mPrimaryHeapIndex;
		VulkanResourceHeap* BestEvictHeap = 0;
		uint64 BestEvictHeapSize = 0;

		VulkanResourceHeap* BestDefragHeap = 0;
		uint64 BestDefragHeapSize = 0;

		{
			for (VulkanResourceHeap* Heap : mResourceTypeHeaps)
			{
				if (Heap->mHeapIndex == PrimaryHeapIndex)
				{
					std::lock_guard<std::recursive_mutex> Lock(Heap->mMtx);

					uint64 UsedSize = Heap->mUsedMemory;
					if (CanDefragHeap(Heap) && BestDefragHeapSize < UsedSize)
					{
						BestDefragHeap = Heap;
						BestDefragHeapSize = UsedSize;
					}
					if (CanEvictHeap(Heap) && BestEvictHeapSize < UsedSize)
					{
						BestEvictHeap = Heap;
						BestEvictHeapSize = UsedSize;
					}
				}
			}
		}

		if (BestEvictHeap && ((GVulkanEvictOnePage || UpdateEvictThreshold(true)) && PrimaryHeapIndex >= 0))
		{
			GVulkanEvictOnePage = 0;
			mPendingEvictBytes += BestEvictHeap->EvictOne(*mDeivce, Context);
		}

	}

	void VulkanMemoryManager::ReleaseFreedResources(bool bImmediately)
	{
		std::vector<VulkanSubResourceAllocator*> BufferAllocationsToRelease;
		{
			std::lock_guard<std::recursive_mutex> Lock(mMtx);
			for (auto& FreeAllocations : mFreeBufferAllocations)
			{
				for (int32 Index = 0; Index < FreeAllocations.size(); ++Index)
				{
					VulkanSubResourceAllocator* BufferAllocation = FreeAllocations[Index];
					if (bImmediately || BufferAllocation->mFrameFreed + 3 < 1)
					{
						BufferAllocationsToRelease.push_back(BufferAllocation);
						Index = FreeAllocations.begin() - FreeAllocations.erase(FreeAllocations.begin() + Index) - 1;
						if (FreeAllocations.size() == 0)
							break;
					}
				}
			}
		}

		for (int32 Index = 0; Index < BufferAllocationsToRelease.size(); ++Index)
		{
			VulkanSubResourceAllocator* BufferAllocation = BufferAllocationsToRelease[Index];

			BufferAllocation->Destroy(mDeivce);

			VulkanResourceHeap* Heap = mResourceTypeHeaps[BufferAllocation->mMemoryTypeIndex];
			Heap->ReleasePage(BufferAllocation);
		}

	}

	void VulkanMemoryManager::DestroyResourceAllocations()
	{
		ReleaseFreedResources(true);

		for (auto& UsedAllocations : mUsedBufferAllocations)
		{
			for (int32 Index = UsedAllocations.size() - 1; Index >= 0; --Index)
			{
				VulkanSubResourceAllocator* BufferAllocation = UsedAllocations[Index];
				if (!BufferAllocation->JoinFreeBlocks())
				{
					LOG_WARN("VulkanRHI: Suballocation(s) for Buffer were not released.");
				}

				BufferAllocation->Destroy(mDeivce);
				mDeivce->GetDeviceMemoryManager()->Free(BufferAllocation->mMemoryAllocation);
				delete BufferAllocation;
			}
			UsedAllocations.clear();
		}

		for (auto& FreeAllocations : mFreeBufferAllocations)
		{
			for (int32 Index = 0; Index < FreeAllocations.size(); ++Index)
			{
				VulkanSubResourceAllocator* BufferAllocation = FreeAllocations[Index];
				BufferAllocation->Destroy(mDeivce);
				mDeivce->GetDeviceMemoryManager()->Free(BufferAllocation->mMemoryAllocation);
				delete BufferAllocation;
			}
			FreeAllocations.clear();
		}
	}

	void VulkanMemoryManager::ProcessPendingUBFreesNoLock(bool bForce)
	{
		static uint32 GFrameNumberRenderThread_WhenWeCanDelete = 0;
		if (!!bForce)
		{
			int NumAlloc = mUBAllocations.mPendingFree.size();
			for (int Index = 0; Index < NumAlloc; ++Index)
			{
				FUBPendingFree& Alloc = mUBAllocations.mPendingFree[Index];
				FreeVulkanAllocation(Alloc.mAllocation, VulkanFreeFlag_DontDefer);
			}
			mUBAllocations.mPendingFree.clear();

			GFrameNumberRenderThread_WhenWeCanDelete = 0;
		}
		else
		{
			if (!!(1 < GFrameNumberRenderThread_WhenWeCanDelete))
			{
				return;
			}

			// making use of the fact that we always add to the end of the array, so allocations are sorted by frame ascending
			int32 OldestFrameToKeep = -2;
			int32 NumAlloc = mUBAllocations.mPendingFree.size();
			int32 Index = 0;
			for (; Index < NumAlloc; ++Index)
			{
				FUBPendingFree& Alloc = mUBAllocations.mPendingFree[Index];
				if (!!(Alloc.mFrame < OldestFrameToKeep))
				{
					FreeVulkanAllocation(Alloc.mAllocation, VulkanFreeFlag_DontDefer);
				}
				else
				{
					// calculate when we will be able to delete the oldest allocation
					GFrameNumberRenderThread_WhenWeCanDelete = Alloc.mFrame + 4;
					break;
				}
			}

			int32 ElementsLeft = NumAlloc - Index;
			if (ElementsLeft > 0 && ElementsLeft != NumAlloc)
			{
				// FUBPendingFree is POD because it is stored in a TArray
				Memory::MemoryMove(mUBAllocations.mPendingFree.data(), mUBAllocations.mPendingFree.data() + Index, ElementsLeft * sizeof(FUBPendingFree));
				for (int32 EndIndex = ElementsLeft; EndIndex < mUBAllocations.mPendingFree.size(); ++EndIndex)
				{
					auto& E = mUBAllocations.mPendingFree[EndIndex];
					if (E.mAllocation.HasAllocation())
					{
						E.mAllocation.Disown();
					}
				}
			}
			mUBAllocations.mPendingFree.resize(NumAlloc - Index);
		}
	}

	void VulkanMemoryManager::ProcessPendingUBFrees(bool bForce)
	{
		std::lock_guard<std::recursive_mutex> Lock(mMtx);
		ProcessPendingUBFreesNoLock(bForce);
	}
}