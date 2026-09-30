#include "ElainePrecompiledHeader.h"
#include "Common/ElaineMemory.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

namespace
{
    constexpr uint32_t kClassCount = 16;
    constexpr size_t kClassBase = 16;
    constexpr uint32_t kStatsCapacity = 4096;
    constexpr uint32_t kLocalCacheLimit = 32;
    constexpr uint64_t kHeaderMagic = 0xE1A1CEA110CAFE01ull;

    struct AllocationHeader
    {
        uint64_t Magic = kHeaderMagic;
        void* Raw = nullptr;
        size_t RequestedSize = 0;
        size_t Alignment = 0;
        uint32_t ClassIndex = 0;
        uint32_t TypeId = 0;
        uint16_t ResourceTypeId = 0;
        uint16_t CategoryId = 0;
        uint32_t OwnerThread = 0;
    };

    struct FreeNode
    {
        FreeNode* Next = nullptr;
    };

    struct Counter
    {
        std::atomic<uint64_t> CurrentBytes{0};
        std::atomic<uint64_t> PeakBytes{0};
        std::atomic<uint64_t> AllocationCount{0};
        std::atomic<uint64_t> FreeCount{0};
        std::atomic<uint64_t> CurrentBlocks{0};
    };

    struct StatsSlot
    {
        std::atomic<uint64_t> Key{0};
        Elaine::MemoryTag Tag;
        uint32_t SizeClass = 0;
        Counter Values;
    };

    struct ThreadCache
    {
        FreeNode* Heads[kClassCount]{};
        uint32_t Counts[kClassCount]{};
        ~ThreadCache();
    };

    struct CentralPool
    {
        FreeNode* Heads[kClassCount]{};
        std::mutex Mutex;
    };

    StatsSlot GStats[kStatsCapacity];
    CentralPool GCentral;
    thread_local ThreadCache GTlsCache;
    constexpr uint32_t kScopeStackCapacity = 32;
    thread_local Elaine::MemoryTag GTlsTagStack[kScopeStackCapacity]{};
    thread_local uint32_t GTlsScopeDepth = 0;
    thread_local bool GTlsInAllocator = false;

    uint32_t ThreadId()
    {
        static std::atomic<uint32_t> NextId{1};
        thread_local uint32_t Id = NextId.fetch_add(1, std::memory_order_relaxed);
        return Id;
    }

    uint32_t ClassIndexFor(size_t Size, size_t Alignment)
    {
        const size_t Required = Size + sizeof(AllocationHeader) + Alignment;
        size_t Capacity = kClassBase;
        for (uint32_t Index = 0; Index < kClassCount; ++Index, Capacity <<= 1)
        {
            if (Required <= Capacity)
                return Index;
        }
        return UINT32_MAX;
    }

    size_t ClassCapacity(uint32_t Index)
    {
        return kClassBase << Index;
    }

    uint64_t MakeKey(const Elaine::MemoryTag& Tag, uint32_t ClassIndex)
    {
        uint64_t Value = 1469598103934665603ull;
        Value ^= Tag.TypeId; Value *= 1099511628211ull;
        Value ^= Tag.ResourceTypeId; Value *= 1099511628211ull;
        Value ^= Tag.CategoryId; Value *= 1099511628211ull;
        Value ^= ClassIndex; Value *= 1099511628211ull;
        return Value ? Value : 1;
    }

    StatsSlot* FindSlot(const Elaine::MemoryTag& Tag, uint32_t ClassIndex, bool Create)
    {
        const uint64_t Key = MakeKey(Tag, ClassIndex);
        const uint32_t Start = static_cast<uint32_t>(Key % kStatsCapacity);
        for (uint32_t Offset = 0; Offset < kStatsCapacity; ++Offset)
        {
            StatsSlot& Slot = GStats[(Start + Offset) % kStatsCapacity];
            uint64_t Existing = Slot.Key.load(std::memory_order_acquire);
            if (Existing == Key)
                return &Slot;
            if (Existing == UINT64_MAX)
                continue;
            if (Existing == 0 && Create)
            {
                uint64_t Expected = 0;
                if (Slot.Key.compare_exchange_strong(Expected, UINT64_MAX, std::memory_order_acq_rel))
                {
                    Slot.Tag = Tag;
                    Slot.SizeClass = ClassIndex;
                    Slot.Key.store(Key, std::memory_order_release);
                    return &Slot;
                }
            }
        }
        return nullptr;
    }

    void AddCounter(Counter& CounterValue, size_t Size)
    {
        const uint64_t Current = CounterValue.CurrentBytes.fetch_add(Size, std::memory_order_relaxed) + Size;
        CounterValue.AllocationCount.fetch_add(1, std::memory_order_relaxed);
        CounterValue.CurrentBlocks.fetch_add(1, std::memory_order_relaxed);
        uint64_t Peak = CounterValue.PeakBytes.load(std::memory_order_relaxed);
        while (Current > Peak && !CounterValue.PeakBytes.compare_exchange_weak(Peak, Current, std::memory_order_relaxed)) {}
    }

    void RemoveCounter(Counter& CounterValue, size_t Size)
    {
        CounterValue.CurrentBytes.fetch_sub(Size, std::memory_order_relaxed);
        CounterValue.FreeCount.fetch_add(1, std::memory_order_relaxed);
        CounterValue.CurrentBlocks.fetch_sub(1, std::memory_order_relaxed);
    }

    void PushCentral(uint32_t ClassIndex, void* Raw)
    {
        auto* Node = static_cast<FreeNode*>(Raw);
        std::lock_guard<std::mutex> Lock(GCentral.Mutex);
        Node->Next = GCentral.Heads[ClassIndex];
        GCentral.Heads[ClassIndex] = Node;
    }

    void* PopCentral(uint32_t ClassIndex)
    {
        std::lock_guard<std::mutex> Lock(GCentral.Mutex);
        FreeNode* Node = GCentral.Heads[ClassIndex];
        if (Node)
            GCentral.Heads[ClassIndex] = Node->Next;
        return Node;
    }

    void* AcquireRaw(uint32_t ClassIndex)
    {
        if (GTlsCache.Heads[ClassIndex])
        {
            FreeNode* Node = GTlsCache.Heads[ClassIndex];
            GTlsCache.Heads[ClassIndex] = Node->Next;
            --GTlsCache.Counts[ClassIndex];
            return Node;
        }
        if (void* Central = PopCentral(ClassIndex))
            return Central;
        return std::malloc(ClassCapacity(ClassIndex));
    }

    void ReleaseRaw(uint32_t ClassIndex, void* Raw, uint32_t OwnerThread)
    {
        if (OwnerThread == ThreadId() && GTlsCache.Counts[ClassIndex] < kLocalCacheLimit)
        {
            auto* Node = static_cast<FreeNode*>(Raw);
            Node->Next = GTlsCache.Heads[ClassIndex];
            GTlsCache.Heads[ClassIndex] = Node;
            ++GTlsCache.Counts[ClassIndex];
        }
        else
        {
            PushCentral(ClassIndex, Raw);
        }
    }

    AllocationHeader* HeaderFromUser(void* Ptr)
    {
        return reinterpret_cast<AllocationHeader*>(static_cast<unsigned char*>(Ptr) - sizeof(AllocationHeader));
    }

    void* AllocateInternal(size_t Size, size_t Alignment)
    {
        if (Size == 0) Size = 1;
        if (Alignment < alignof(void*)) Alignment = alignof(void*);
        if ((Alignment & (Alignment - 1)) != 0) return nullptr;

        const uint32_t ClassIndex = Alignment <= alignof(std::max_align_t) ? ClassIndexFor(Size, Alignment) : UINT32_MAX;
        const Elaine::MemoryTag Tag = GTlsScopeDepth ? GTlsTagStack[GTlsScopeDepth - 1] : Elaine::MemoryTag{};
        void* Raw = ClassIndex == UINT32_MAX ? std::malloc(Size + sizeof(AllocationHeader) + Alignment) : AcquireRaw(ClassIndex);
        if (!Raw) return nullptr;

        uintptr_t Begin = reinterpret_cast<uintptr_t>(Raw) + sizeof(AllocationHeader);
        uintptr_t UserAddress = (Begin + Alignment - 1) & ~(static_cast<uintptr_t>(Alignment) - 1);
        auto* Header = reinterpret_cast<AllocationHeader*>(UserAddress - sizeof(AllocationHeader));
        Header->Raw = Raw;
        Header->RequestedSize = Size;
        Header->Alignment = Alignment;
        Header->ClassIndex = ClassIndex;
        Header->TypeId = Tag.TypeId;
        Header->ResourceTypeId = Tag.ResourceTypeId;
        Header->CategoryId = Tag.CategoryId;
        Header->OwnerThread = ThreadId();

        if (StatsSlot* Slot = FindSlot(Tag, ClassIndex, true))
            AddCounter(Slot->Values, Size);
        return reinterpret_cast<void*>(UserAddress);
    }

    void DeallocateInternal(void* Ptr) noexcept
    {
        if (!Ptr) return;
        AllocationHeader* Header = HeaderFromUser(Ptr);
        if (Header->Magic != kHeaderMagic)
        {
            LOG_WARN("Memory release rejected: invalid or duplicate allocation header.");
            return;
        }
        Elaine::MemoryTag Tag{Header->TypeId, Header->ResourceTypeId, Header->CategoryId};
        if (StatsSlot* Slot = FindSlot(Tag, Header->ClassIndex, false))
            RemoveCounter(Slot->Values, Header->RequestedSize);
        void* Raw = Header->Raw;
        const uint32_t ClassIndex = Header->ClassIndex;
        const uint32_t OwnerThread = Header->OwnerThread;
        Header->Magic = 0;
        if (ClassIndex == UINT32_MAX)
            std::free(Raw);
        else
            ReleaseRaw(ClassIndex, Raw, OwnerThread);
    }

    ThreadCache::~ThreadCache()
    {
        for (uint32_t Index = 0; Index < kClassCount; ++Index)
        {
            FreeNode* Node = Heads[Index];
            while (Node)
            {
                FreeNode* Next = Node->Next;
                std::free(Node);
                Node = Next;
            }
        }
    }
}

namespace Elaine
{
    MemoryScope::MemoryScope(const MemoryTag& InTag)
    {
        if (GTlsScopeDepth < 32)
        {
            GTlsTagStack[GTlsScopeDepth] = InTag;
            ++GTlsScopeDepth;
            mActive = true;
        }
    }

    MemoryScope::~MemoryScope()
    {
        if (!mActive)
            return;
        if (GTlsScopeDepth > 0)
            --GTlsScopeDepth;
        if (GTlsScopeDepth < kScopeStackCapacity)
            GTlsTagStack[GTlsScopeDepth] = MemoryTag{};
    }

    void* Memory::Allocate(size_t Size, size_t Alignment)
    {
        if (GTlsInAllocator) return std::malloc(Size ? Size : 1);
        GTlsInAllocator = true;
        void* Result = AllocateInternal(Size, Alignment);
        GTlsInAllocator = false;
        return Result;
    }

    void Memory::Deallocate(void* Ptr, size_t) noexcept
    {
        if (GTlsInAllocator) { std::free(Ptr); return; }
        GTlsInAllocator = true;
        DeallocateInternal(Ptr);
        GTlsInAllocator = false;
    }

    void* Memory::Reallocate(void* Ptr, size_t NewSize, size_t Alignment)
    {
        if (!Ptr) return Allocate(NewSize, Alignment);
        if (NewSize == 0) { Deallocate(Ptr); return nullptr; }
        AllocationHeader* Header = HeaderFromUser(Ptr);
        const size_t OldSize = Header->Magic == kHeaderMagic ? Header->RequestedSize : 0;
        void* NewPtr = Allocate(NewSize, Alignment ? Alignment : (Header->Magic == kHeaderMagic ? Header->Alignment : alignof(std::max_align_t)));
        if (!NewPtr) return nullptr;
        if (OldSize) std::memcpy(NewPtr, Ptr, std::min(OldSize, NewSize));
        Deallocate(Ptr);
        return NewPtr;
    }

    MemoryTag Memory::GetCurrentTag() { return GTlsScopeDepth ? GTlsTagStack[GTlsScopeDepth - 1] : MemoryTag{}; }

    uint32_t Memory::TypeId(const char* TypeName)
    {
        uint32_t Hash = 2166136261u;
        if (TypeName)
            while (*TypeName) { Hash ^= static_cast<unsigned char>(*TypeName++); Hash *= 16777619u; }
        return Hash;
    }

    MemoryStats Memory::GetMemoryStats()
    {
        MemoryStats Result;
        for (auto& Slot : GStats)
        {
            if (Slot.Key.load(std::memory_order_acquire) == 0) continue;
            Result.CurrentBytes += Slot.Values.CurrentBytes.load();
            Result.PeakBytes = std::max(Result.PeakBytes, Slot.Values.PeakBytes.load());
            Result.AllocationCount += Slot.Values.AllocationCount.load();
            Result.FreeCount += Slot.Values.FreeCount.load();
            Result.CurrentBlocks += Slot.Values.CurrentBlocks.load();
        }
        return Result;
    }

    MemoryStats Memory::GetMemoryStats(uint32_t TypeIdValue, uint16_t ResourceTypeId)
    {
        MemoryStats Result;
        MemoryTag Tag{TypeIdValue, ResourceTypeId, 0};
        for (auto& Slot : GStats)
        {
            if (Slot.Key.load(std::memory_order_acquire) == 0 || Slot.Tag.TypeId != Tag.TypeId || Slot.Tag.ResourceTypeId != Tag.ResourceTypeId) continue;
            Result.CurrentBytes += Slot.Values.CurrentBytes.load();
            Result.PeakBytes = std::max(Result.PeakBytes, Slot.Values.PeakBytes.load());
            Result.AllocationCount += Slot.Values.AllocationCount.load();
            Result.FreeCount += Slot.Values.FreeCount.load();
            Result.CurrentBlocks += Slot.Values.CurrentBlocks.load();
        }
        return Result;
    }

    std::vector<MemoryStatsEntry> Memory::GetMemoryStatsSnapshot()
    {
        std::vector<MemoryStatsEntry> Result;
        for (auto& Slot : GStats)
        {
            if (Slot.Key.load(std::memory_order_acquire) == 0) continue;
            MemoryStatsEntry Entry;
            Entry.Tag = Slot.Tag;
            Entry.SizeClass = Slot.SizeClass;
            Entry.Stats.CurrentBytes = Slot.Values.CurrentBytes.load();
            Entry.Stats.PeakBytes = Slot.Values.PeakBytes.load();
            Entry.Stats.AllocationCount = Slot.Values.AllocationCount.load();
            Entry.Stats.FreeCount = Slot.Values.FreeCount.load();
            Entry.Stats.CurrentBlocks = Slot.Values.CurrentBlocks.load();
            Result.push_back(Entry);
        }
        return Result;
    }

    void Memory::ResetMemoryStats()
    {
        for (auto& Slot : GStats)
        {
            if (Slot.Key.load(std::memory_order_acquire) == 0) continue;
            const uint64_t Current = Slot.Values.CurrentBytes.load();
            Slot.Values.PeakBytes.store(Current);
            Slot.Values.AllocationCount.store(0);
            Slot.Values.FreeCount.store(0);
        }
    }

    void Memory::DumpMemoryLeaks()
    {
        for (auto& Slot : GStats)
        {
            if (Slot.Key.load(std::memory_order_acquire) == 0 || Slot.Values.CurrentBlocks.load() == 0) continue;
            LOG_WARN("Memory leak: type={} resource={} class={} bytes={} blocks={}", Slot.Tag.TypeId, Slot.Tag.ResourceTypeId, Slot.SizeClass, Slot.Values.CurrentBytes.load(), Slot.Values.CurrentBlocks.load());
        }
    }

    void* Memory::SystemMalloc(size_t Size) { return Allocate(Size); }
    void Memory::SystemFree(void* Ptr) { Deallocate(Ptr); }
}

void* operator new(std::size_t Size) { if (void* Ptr = Elaine::Memory::Allocate(Size)) return Ptr; throw std::bad_alloc(); }
void* operator new[](std::size_t Size) { if (void* Ptr = Elaine::Memory::Allocate(Size)) return Ptr; throw std::bad_alloc(); }
void* operator new(std::size_t Size, const std::nothrow_t&) noexcept { return Elaine::Memory::Allocate(Size); }
void* operator new[](std::size_t Size, const std::nothrow_t&) noexcept { return Elaine::Memory::Allocate(Size); }
void* operator new(std::size_t Size, std::align_val_t Alignment) { if (void* Ptr = Elaine::Memory::Allocate(Size, static_cast<size_t>(Alignment))) return Ptr; throw std::bad_alloc(); }
void* operator new[](std::size_t Size, std::align_val_t Alignment) { if (void* Ptr = Elaine::Memory::Allocate(Size, static_cast<size_t>(Alignment))) return Ptr; throw std::bad_alloc(); }
void* operator new(std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept { return Elaine::Memory::Allocate(Size, static_cast<size_t>(Alignment)); }
void* operator new[](std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept { return Elaine::Memory::Allocate(Size, static_cast<size_t>(Alignment)); }
void operator delete(void* Ptr) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete[](void* Ptr) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete(void* Ptr, std::size_t) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete[](void* Ptr, std::size_t) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete(void* Ptr, std::align_val_t Alignment) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
void operator delete[](void* Ptr, std::align_val_t Alignment) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
void operator delete(void* Ptr, std::size_t, std::align_val_t Alignment) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
void operator delete[](void* Ptr, std::size_t, std::align_val_t Alignment) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
void operator delete(void* Ptr, const std::nothrow_t&) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete[](void* Ptr, const std::nothrow_t&) noexcept { Elaine::Memory::Deallocate(Ptr); }
void operator delete(void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
void operator delete[](void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept { Elaine::Memory::Deallocate(Ptr, static_cast<size_t>(Alignment)); }
