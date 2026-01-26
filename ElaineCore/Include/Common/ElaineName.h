#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport Name
	{
	public:
		Name() = default;
		Name(const char* InName);
		~Name();
		//Name(Name&& InName);
		//Name(Name& InName);
		//Name& operator=(Name&& InName)
		//{

		//}
		//Name& operator=(Name& InName)
		//{

		//}

		bool operator==(const Name& InOther) const
		{
			return mHashValue == InOther.mHashValue;
		}

		bool operator!=(const Name& InOther) const
		{
			return mHashValue != InOther.mHashValue;
		}

		bool Empty() const
		{
			return mHashValue == 0;
		}

		uint64_t GetHashValue() const
		{
			return mHashValue;
		}

		const char* C_Str() const
		{
			return mStringValue;
		}

		std::string ToString() const
		{
			return std::string(mStringValue);
		}

		bool operator<(const Name& InName) const
		{
			return mHashValue < InName.mHashValue;
		}

		static uint64_t CalculateHash(const char* InString);

	private:

	private:
		const char* mStringValue = nullptr;
		uint64_t mHashValue = 0;
	public:
		static Name EMPTY_NAME;
		friend class NamePool;
	};
}

namespace std
{
	template<>
	struct hash<Elaine::Name>
	{
		size_t operator()(const Elaine::Name& n) const noexcept
		{
			return n.GetHashValue();
		}
	};
}