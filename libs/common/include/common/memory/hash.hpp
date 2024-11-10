#pragma once

#include "common/common.hpp"

namespace rn
{
    constexpr uint64_t LARGE_PRIME = 8368526953680221;
    struct LargeHash
    {
        uint64_t lower = LARGE_PRIME;
        uint64_t upper = LARGE_PRIME;
    };

    inline bool operator==(const LargeHash& lhs, const LargeHash& rhs)
    {
        return lhs.lower == rhs.lower && lhs.upper == rhs.upper;
    }

    inline bool operator!=(const LargeHash& lhs, const LargeHash& rhs)
    {
        return lhs.lower != rhs.lower || lhs.upper != rhs.upper;
    }

    LargeHash HashMemory(const void* ptr, size_t size);

    template <typename T>
    LargeHash HashValue(const T& value)
    {
        return MemoryHash(&value, sizeof(value));
    }
}