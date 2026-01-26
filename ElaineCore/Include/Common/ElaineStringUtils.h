#pragma once
#include "ElaineCorePrerequirements.h"
#include <algorithm>

namespace Elaine
{
	class StringUtils
	{
    public:
       static void ToLower(std::string& InString)
       {
            std::transform(InString.begin(), InString.end(), InString.begin(),
                [](unsigned char c)
                {
                    return std::tolower(c);
                });
       }

    public:
        static inline std::string EMPTY = "";
	};
}