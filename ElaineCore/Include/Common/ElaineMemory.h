#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <new>
#include <vector>

namespace Elaine
{
	struct MemoryTag
	{
		uint32_t TypeId = 0;
		uint16_t ResourceTypeId = 0;
		uint16_t CategoryId = 0;
	};

	struct MemoryStats
	{
		uint64_t CurrentBytes = 0;
		uint64_t PeakBytes = 0;
		uint64_t AllocationCount = 0;
		uint64_t FreeCount = 0;
		uint64_t CurrentBlocks = 0;
	};

	struct MemoryStatsEntry
	{
		MemoryTag Tag;
		uint32_t SizeClass = 0;
		MemoryStats Stats;
	};

	class ElaineCoreExport MemoryScope
	{
	public:
		explicit MemoryScope(const MemoryTag& InTag);
		~MemoryScope();
		MemoryScope(const MemoryScope&) = delete;
		MemoryScope& operator=(const MemoryScope&) = delete;
	private:
		bool mActive = false;
	};
	template <typename T>
	struct TIsPointer
	{
		enum { Value = false };
	};

	template <typename T> struct TIsPointer<T*> { enum { Value = true }; };

	template <typename T> struct TIsPointer<const          T> { enum { Value = TIsPointer<T>::Value }; };
	template <typename T> struct TIsPointer<      volatile T> { enum { Value = TIsPointer<T>::Value }; };
	template <typename T> struct TIsPointer<const volatile T> { enum { Value = TIsPointer<T>::Value }; };

	template <typename T>
	struct TIsIntegral
	{
		enum { Value = false };
	};

	template <> struct TIsIntegral<         bool> { enum { Value = true }; };
	template <> struct TIsIntegral<         char> { enum { Value = true }; };
	template <> struct TIsIntegral<signed   char> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned char> { enum { Value = true }; };
	template <> struct TIsIntegral<         char16_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         char32_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         wchar_t> { enum { Value = true }; };
	template <> struct TIsIntegral<         short> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned short> { enum { Value = true }; };
	template <> struct TIsIntegral<         int> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned int> { enum { Value = true }; };
	template <> struct TIsIntegral<         long> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned long> { enum { Value = true }; };
	template <> struct TIsIntegral<         long long> { enum { Value = true }; };
	template <> struct TIsIntegral<unsigned long long> { enum { Value = true }; };

	template <typename T> struct TIsIntegral<const          T> { enum { Value = TIsIntegral<T>::Value }; };
	template <typename T> struct TIsIntegral<      volatile T> { enum { Value = TIsIntegral<T>::Value }; };
	template <typename T> struct TIsIntegral<const volatile T> { enum { Value = TIsIntegral<T>::Value }; };


	template <typename T>
	__forceinline constexpr T Align(T Val, uint64 Alignment)
	{
		static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "Align expects an integer or pointer type");

		return (T)(((uint64)Val + Alignment - 1) & ~(Alignment - 1));
	}

	template <typename T>
	__forceinline constexpr T AlignArbitrary(T Val, uint64 Alignment)
	{
		static_assert(TIsIntegral<T>::Value || TIsPointer<T>::Value, "AlignArbitrary expects an integer or pointer type");

		return (T)((((uint64)Val + Alignment - 1) / Alignment) * Alignment);
	}

	template <typename T>
	__forceinline void Valswap(T& A, T& B)
	{
		T Tmp = A;
		A = B;
		B = Tmp;
	}

	static void MemswapGreaterThan8(void* Ptr1, void* Ptr2, size_t Size);

	class ElaineCoreExport Memory
	{
	public:
		static void* Allocate(size_t Size, size_t Alignment = alignof(std::max_align_t));
		static void Deallocate(void* Ptr, size_t Alignment = 0) noexcept;
		static void* Reallocate(void* Ptr, size_t NewSize, size_t Alignment = 0);
		static MemoryTag GetCurrentTag();
		static uint32_t TypeId(const char* TypeName);
		static MemoryStats GetMemoryStats();
		static MemoryStats GetMemoryStats(uint32_t TypeId, uint16_t ResourceTypeId);
		static std::vector<MemoryStatsEntry> GetMemoryStatsSnapshot();
		static void ResetMemoryStats();
		static void DumpMemoryLeaks();
		static void* MemorySet(void* Dest, uint8 inChar, size_t Count);
		static void* MemoryZero(void* Dest, size_t insize);
		static void* MemoryCopy(void* Dest, const void* Src, size_t Count);
		static void* MemoryMove(void* Dest, const void* Src, size_t Count);
		static void* MemorySwap(void* Ptr1, void* Ptr2, size_t Size);
		static void* SystemMalloc(size_t Size);
		static void  SystemFree(void* Ptr);



		template<class T>
		static __forceinline void* MemoryZero(T& Src)
		{
			static_assert(!TIsPointer<T>::Value, "For pointers use the two parameters function");
			return MemoryZero(&Src, sizeof(T));
		}

		template<class T>
		static __forceinline void* MemoryCopy(T& Dest, const T& Src)
		{
			return MemoryCopy(&Dest, &Src, sizeof(T));
		}
	};
}

void* operator new(std::size_t Size);
void* operator new[](std::size_t Size);
void* operator new(std::size_t Size, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t Size, const std::nothrow_t&) noexcept;
void* operator new(std::size_t Size, std::align_val_t Alignment);
void* operator new[](std::size_t Size, std::align_val_t Alignment);
void* operator new(std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept;
void* operator new[](std::size_t Size, std::align_val_t Alignment, const std::nothrow_t&) noexcept;
void operator delete(void* Ptr) noexcept;
void operator delete[](void* Ptr) noexcept;
void operator delete(void* Ptr, std::size_t Size) noexcept;
void operator delete[](void* Ptr, std::size_t Size) noexcept;
void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;
void operator delete[](void* Ptr, std::align_val_t Alignment) noexcept;
void operator delete(void* Ptr, std::size_t Size, std::align_val_t Alignment) noexcept;
void operator delete[](void* Ptr, std::size_t Size, std::align_val_t Alignment) noexcept;
void operator delete(void* Ptr, const std::nothrow_t&) noexcept;
void operator delete[](void* Ptr, const std::nothrow_t&) noexcept;
void operator delete(void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept;
void operator delete[](void* Ptr, std::align_val_t Alignment, const std::nothrow_t&) noexcept;
