#include "ElainePrecompiledHeader.h"

namespace Elaine
{
	void MemswapGreaterThan8(void* Ptr1, void* Ptr2, size_t Size)
	{
		union PtrUnion
		{
			void* PtrVoid;
			uint8* Ptr8;
			uint16* Ptr16;
			uint32* Ptr32;
			uint64* Ptr64;
			uint64 PtrUint;
		};

		PtrUnion Union1 = { Ptr1 };
		PtrUnion Union2 = { Ptr2 };

		if (Union1.PtrUint & 1)
		{
			Valswap(*Union1.Ptr8++, *Union2.Ptr8++);
			Size -= 1;
		}
		if (Union1.PtrUint & 2)
		{
			Valswap(*Union1.Ptr16++, *Union2.Ptr16++);
			Size -= 2;
		}
		if (Union1.PtrUint & 4)
		{
			Valswap(*Union1.Ptr32++, *Union2.Ptr32++);
			Size -= 4;
		}

		uint32 CommonAlignment = Math::min(Math::CountTrailingZeros((uint32)(Union1.PtrUint - Union2.PtrUint)), 3u);
		switch (CommonAlignment)
		{
		default:
			for (; Size >= 8; Size -= 8)
			{
				Valswap(*Union1.Ptr64++, *Union2.Ptr64++);
			}

		case 2:
			for (; Size >= 4; Size -= 4)
			{
				Valswap(*Union1.Ptr32++, *Union2.Ptr32++);
			}

		case 1:
			for (; Size >= 2; Size -= 2)
			{
				Valswap(*Union1.Ptr16++, *Union2.Ptr16++);
			}

		case 0:
			for (; Size >= 1; Size -= 1)
			{
				Valswap(*Union1.Ptr8++, *Union2.Ptr8++);
			}
		}
	}

	void* Memory::MemoryZero(void* dest, size_t insize)
	{
		return memset(dest, 0, insize);
	}

	void* Memory::MemorySet(void* Dest, uint8 inChar, size_t count)
	{
		return memset(Dest, inChar, count);
	}

	void* Memory::MemoryCopy(void* Dest, const void* Src, size_t Count)
	{
		return memcpy(Dest, Src, Count);
	}

	void* Memory::MemoryMove(void* Dest, const void* Src, size_t Count)
	{
		return memmove(Dest, Src, Count);
	}

	void* Memory::MemorySwap(void* Ptr1, void* Ptr2, size_t Size)
	{
		switch (Size)
		{
		case 0:
			break;

		case 1:
			Valswap(*(uint8*)Ptr1, *(uint8*)Ptr2);
			break;

		case 2:
			Valswap(*(uint16*)Ptr1, *(uint16*)Ptr2);
			break;

		case 3:
			Valswap(*((uint16*&)Ptr1)++, *((uint16*&)Ptr2)++);
			Valswap(*(uint8*)Ptr1, *(uint8*)Ptr2);
			break;

		case 4:
			Valswap(*(uint32*)Ptr1, *(uint32*)Ptr2);
			break;

		case 5:
			Valswap(*((uint32*&)Ptr1)++, *((uint32*&)Ptr2)++);
			Valswap(*(uint8*)Ptr1, *(uint8*)Ptr2);
			break;

		case 6:
			Valswap(*((uint32*&)Ptr1)++, *((uint32*&)Ptr2)++);
			Valswap(*(uint16*)Ptr1, *(uint16*)Ptr2);
			break;

		case 7:
			Valswap(*((uint32*&)Ptr1)++, *((uint32*&)Ptr2)++);
			Valswap(*((uint16*&)Ptr1)++, *((uint16*&)Ptr2)++);
			Valswap(*(uint8*)Ptr1, *(uint8*)Ptr2);
			break;

		case 8:
			Valswap(*(uint64*)Ptr1, *(uint64*)Ptr2);
			break;

		case 16:
			Valswap(((uint64*)Ptr1)[0], ((uint64*)Ptr2)[0]);
			Valswap(((uint64*)Ptr1)[1], ((uint64*)Ptr2)[1]);
			break;

		default:
			MemswapGreaterThan8(Ptr1, Ptr2, Size);
			break;
		}
		return Ptr2;
	}
	__forceinline void* Memory::SystemMalloc(size_t Size)
	{
		return ::malloc(Size);
	}
	void Memory::SystemFree(void* Ptr)
	{
		::free(Ptr);
	}

}