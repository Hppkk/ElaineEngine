#pragma once
#include "nlohmann/json_fwd.hpp"
#include "ElaineVersion.h"

namespace Elaine
{
	extern ElaineCoreExport Version CurrentEngineVersion;

#ifndef ENUM_OPERATORS
#define ENUM_OPERATORS(EnumType)                            \
inline EnumType operator|(EnumType a, EnumType b)                       \
{                                                                         \
    using T = std::underlying_type_t<EnumType>;                          \
    return static_cast<EnumType>(static_cast<T>(a) | static_cast<T>(b));\
}                                                                         \
                                                                          \
inline EnumType operator&(EnumType a, EnumType b)                        \
{                                                                         \
    using T = std::underlying_type_t<EnumType>;                          \
    return static_cast<EnumType>(static_cast<T>(a) & static_cast<T>(b));\
}                                                                         \
                                                                          \
inline EnumType operator^(EnumType a, EnumType b)                        \
{                                                                         \
    using T = std::underlying_type_t<EnumType>;                          \
    return static_cast<EnumType>(static_cast<T>(a) ^ static_cast<T>(b));\
}                                                                         \
                                                                          \
inline EnumType operator~(EnumType a)                                    \
{                                                                         \
    using T = std::underlying_type_t<EnumType>;                          \
    return static_cast<EnumType>(~static_cast<T>(a));                   \
}                                                                         \
                                                                          \
inline EnumType& operator|=(EnumType& a, EnumType b)                     \
{                                                                         \
    a = a | b;                                                           \
    return a;                                                           \
}                                                                         \
                                                                          \
inline EnumType& operator&=(EnumType& a, EnumType b)                     \
{                                                                         \
    a = a & b;                                                           \
    return a;                                                           \
}\
inline bool operator==(EnumType a, EnumType b)                          \
{                                                                       \
    using T = std::underlying_type_t<EnumType>;                         \
    return static_cast<T>(a) == static_cast<T>(b);                       \
}                                                                       
#endif
    using JsonCpp = nlohmann::json;
}