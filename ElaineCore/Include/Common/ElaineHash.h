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
}