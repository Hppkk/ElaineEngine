#pragma once

namespace Elaine
{
    template<typename T>
    inline void hash_combine(std::size_t& seed, const T& v)
    {
        seed ^= std::hash<T> {}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
    }

    template<typename T, typename... Ts>
    inline void hash_combine(std::size_t& seed, const T& v, Ts... rest)
    {
        hash_combine(seed, v);
        if constexpr (sizeof...(Ts) > 1)
        {
            hash_combine(seed, rest...);
        }
    }

    inline std::string ElaineCoreExport   getHashValString(const std::string& str);

    inline uint32 HashCombine(uint32 A, uint32 C)
    {
        uint32 B = 0x9e3779b9;
        A += B;

        A -= B; A -= C; A ^= (C >> 13);
        B -= C; B -= A; B ^= (A << 8);
        C -= A; C -= B; C ^= (B >> 13);
        A -= B; A -= C; A ^= (C >> 12);
        B -= C; B -= A; B ^= (A << 16);
        C -= A; C -= B; C ^= (B >> 5);
        A -= B; A -= C; A ^= (C >> 3);
        B -= C; B -= A; B ^= (A << 10);
        C -= A; C -= B; C ^= (B >> 15);

        return C;
    }

    inline void HashCombine(uint64& seed, uint64 value)
    {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    inline uint32_t FNV_1A_HASH_32(const std::string& InString)
    {
        if (InString.empty())
            return 0;

        unsigned int hash = 2166136261U;
        for (char c : InString)
        {
            hash ^= static_cast<unsigned char>(c);
            hash *= 16777619;
        }
        return hash;
    }

    inline uint32_t FNV_1A_HASH_32(const char* InString)
    {
        if (InString == nullptr)
            return 0;

        size_t len = strlen(InString);
        unsigned int hash = 2166136261U;
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= static_cast<unsigned char>(InString[i]);
            hash *= 16777619;
        }
        return hash;
    }

    inline uint64_t FNV_1A_HASH_64(const std::string& InString)
    {
        uint64_t hash = 14695981039346656037ULL;
        for (char c : InString)
        {
            hash ^= static_cast<unsigned char>(c);
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    inline uint64_t FNV_1A_HASH_64(const char* InString)
    {
        if (InString == nullptr)
            return 0;

        size_t len = strlen(InString);
        uint64_t hash = 14695981039346656037ULL;
        for (size_t i = 0; i < len; ++i)
        {
            hash ^= static_cast<unsigned char>(InString[i]);
            hash *= 1099511628211ULL;
        }
        return hash;
    }
}