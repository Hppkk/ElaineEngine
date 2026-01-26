#include "ElainePrecompiledHeader.h"
#include "ElaineNamePool.h"
#include "ElaineHash.h"

namespace Elaine
{
	Name Name::EMPTY_NAME = Name("");

	Name::Name(const char* InName)
	{
		if (InName == nullptr || std::strcmp(InName, "") == 0)
			return;

		mHashValue = CalculateHash(InName);
		mStringValue = NamePool::instance().GetName(InName, mHashValue);
	}

	Name::~Name()
	{
		mHashValue = 0;
		mStringValue = nullptr;
	}

	uint64_t Name::CalculateHash(const char* InString)
	{
		return FNV_1A_HASH_64(InString);
	}
}