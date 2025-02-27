#pragma once
#include "ElaineCorePrerequirements.h"
#include "ElaineMemory.h"
#include "ElaineMath.h"

namespace Elaine
{
#define OBJECT_MAX_MALLOC_ 32

	struct BlockInfo
	{
		uint8 mIndex = 0u;
		bool mbUsed = false;
		uint8 mChunkIndex = 0u;
	};

	template<class Ty>
	struct ObjectChunk
	{
		ObjectChunk()
		{
			size_t MallocSize = (sizeof(Ty) + sizeof(BlockInfo)) * OBJECT_MAX_MALLOC_;
			mMemoryBlock = (char*)Memory::SystemMalloc(MallocSize);
			Memory::MemoryZero(mMemoryBlock, MallocSize);
		}

		~ObjectChunk()
		{
			Memory::SystemFree(mMemoryBlock);
		}

		Ty* AllocateMemory(uint8 InIndex)
		{
			size_t ResultIndex = (sizeof(Ty) + sizeof(BlockInfo)) * mFreeInedx + sizeof(Ty);
			new(&(mMemoryBlock[ResultIndex]))BlockInfo{ mFreeInedx, true ,InIndex };
			uint8 TempFreeIndex = mFreeInedx;
			mFreeInedx = Next();
			return (Ty*)(&(mMemoryBlock[ResultIndex - sizeof(Ty)]));
		}

		void DeallocateMemory(Ty* InMemory)
		{
			BlockInfo* CurrentBlock = reinterpret_cast<BlockInfo*>(&InMemory[1]);
			CurrentBlock->mbUsed = false;
			size_t CurrentIndex = CurrentBlock->mIndex;
			mFreeInedx = Math::min((size_t)mFreeInedx, CurrentIndex);
		}

		bool IsFull()
		{
			return mFreeInedx == mMaxCapacity;
		}

		uint8 Next()
		{
			if (IsFull())
				return mMaxCapacity;

			for (size_t Index = mFreeInedx + 1; Index < mMaxCapacity; ++Index)
			{
				size_t ResultIndex = (sizeof(Ty) + sizeof(BlockInfo)) * Index + sizeof(Ty);
				BlockInfo* CurrentBlock = reinterpret_cast<BlockInfo*>(&mMemoryBlock[ResultIndex]);
				if (!CurrentBlock->mbUsed)
					return Index;
			}
			return mMaxCapacity;
		}

		char* mMemoryBlock = nullptr;
		uint8 mFreeInedx = 0u;
		uint8 mMaxCapacity = OBJECT_MAX_MALLOC_;
	};

	template<typename Ty>
	class RHIObjectPool
	{
	public:
		RHIObjectPool()
		{
			mStacks.reserve(4);
			mStacks.push_back({ });
			mFreeStack = mStacks[0];
		}

		~RHIObjectPool()
		{

		}

		template<class ...ValTy>
		Ty* CreateObject(ValTy&& ...InVals)
		{
			if (mFreeStack.IsFull())
			{
				mStacks.push_back({ });
				mFreeStack = mStacks.back();
				mFreeIndex = mStacks.size() - 1;
			}

			return new(mFreeStack.AllocateMemory(mFreeIndex))Ty(std::forward<ValTy>(InVals)...);
		}

		void ReleaseObject(Ty* InObject)
		{
			if (InObject == nullptr)
				return;

			InObject->~Ty();
			mFreeIndex = reinterpret_cast<BlockInfo*>((void*)(&InObject[1]))->mChunkIndex;
			auto&& CurrentFree = mStacks[mFreeIndex];
			CurrentFree.DeallocateMemory(InObject);
			mFreeStack = CurrentFree;
		}

	private:
		std::vector<ObjectChunk<Ty>> mStacks;
		ObjectChunk<Ty> mFreeStack;
		size_t mFreeIndex = 0u;

		friend Ty;
	};

}