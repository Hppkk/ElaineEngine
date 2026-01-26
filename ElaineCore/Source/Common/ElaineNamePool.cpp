#include "ElainePrecompiledHeader.h"
#include "ElaineNamePool.h"

namespace Elaine
{
	namespace
	{
		static NamePool _Instance;
	}

	NamePool::~NamePool()
	{
		for (auto&& Na : mPool)
		{
			DellocName(Na.second);
		}
	}

	NamePool& NamePool::instance()
	{
		return _Instance;
	}

	const char* NamePool::Find(uint64_t InHash)
	{
		std::shared_lock lock(mMutex);
		auto Iter = mPool.find(InHash);
		if (Iter != mPool.end())
		{
			return Iter->second;
		}
		return nullptr;
	}

	const char* NamePool::GetName(const char* InName, uint64_t InHash)
	{
		{
			std::shared_lock lock(mMutex);
			auto Iter = mPool.find(InHash);
			if (Iter != mPool.end())
			{
				return Iter->second;
			}
		}

		{
			std::unique_lock lock(mMutex);
			// 再次检查，防止在获取写锁期间其他线程已插入
			auto Iter = mPool.find(InHash);
			if (Iter != mPool.end())
			{
				return Iter->second;
			}
			return AllocName(InName, InHash);
		}
	}

	const char* NamePool::AllocName(const char* InName, uint64_t InHash)
	{
		size_t StrLength = strlen(InName);
		char* Mem = (char*)Memory::SystemMalloc(StrLength + 1)	;
		Memory::MemoryCopy(Mem, InName, StrLength + 1);
		mPool.emplace(InHash, Mem);
		return Mem;
	}

	void NamePool::DellocName(const char* InName)
	{
		if (InName == nullptr)
			return;

		Memory::SystemFree((void*)InName);
	}
}