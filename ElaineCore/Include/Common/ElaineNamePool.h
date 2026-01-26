#pragma once
#include "ElaineCorePrerequirements.h"
#include <shared_mutex>

namespace Elaine
{
	class ElaineCoreExport NamePool
	{
	public:
		~NamePool();
		static NamePool& instance();
		const char* Find(uint64_t InHash);
		const char* GetName(const char* InName, uint64_t InHash);
	private:
		const char* AllocName(const char* InName, uint64_t InHash);
		void DellocName(const char* InName);
	private:
		std::unordered_map<uint64_t, const char*> mPool;
		mutable std::shared_mutex mMutex;
	};
}