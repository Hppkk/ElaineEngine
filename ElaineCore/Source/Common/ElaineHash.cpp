#include "ElainePrecompiledHeader.h"

namespace Elaine
{
	inline std::string getHashValString(const std::string& str)
	{
		size_t hash = std::hash<std::string>{}(str);
		size_t val1 = hash & 0xf;
		size_t val2 = (hash >> 4) & 0xf;
		size_t val3 = (hash >> 8) & 0xf;
		size_t val4 = (hash >> 12) & 0xf;
		size_t val5 = (hash >> 16) & 0xf;
		size_t val6 = (hash >> 20) & 0xf;
		size_t val7 = (hash >> 24) & 0xf;
		size_t val8 = (hash >> 28) & 0xf;

		return std::format("%4x%4x%4x%4x%4x%4x%4x%4x", val1, val2, val3, val4, val5, val6, val7, val8);
	}
}