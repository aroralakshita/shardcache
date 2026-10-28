#pragma once

#include <cstdint>
#include <string_view>

namespace shardcache {

inline std::uint64_t hash64(std::string_view text) {
    std::uint64_t h = 14695981039346656037ull; 
    for (unsigned char c : text) {
        h ^= c;
        h *= 1099511628211ull;
    }
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ull;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebull;
    h ^= h >> 31;
    return h;
}

}
