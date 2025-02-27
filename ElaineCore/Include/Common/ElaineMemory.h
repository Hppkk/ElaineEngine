#pragma once

namespace Elaine
{
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