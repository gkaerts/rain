#include "common/memory/hash.hpp"
#include "murmurhash3/MurmurHash3.h"

namespace rn
{
    LargeHash HashMemory(const void* ptr, size_t size)
    {
        LargeHash newHash;
        const uint32_t seed = uint32_t(LARGE_PRIME);
        MurmurHash3_x64_128(ptr, int(size), seed, &newHash);

        return newHash;
    }
}