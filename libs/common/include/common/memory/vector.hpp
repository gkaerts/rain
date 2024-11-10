#pragma once

#include "common/memory/memory.hpp"
#include <vector>

namespace rn
{
    template <typename T>
    using Vector = std::vector<T, TrackedAllocatorSTL<T>>;

    template <typename T>
    using ScopedVector = std::vector<T, ScopedAllocatorSTL<T>>;

    template <typename T>
    Vector<T> MakeVector(MemoryCategoryID cat)
    {
        return Vector<T>(TrackedAllocatorSTL<T>(cat));
    }

    template <typename T>
    Vector<T> MakeVector(size_t n, MemoryCategoryID cat)
    {
        return Vector<T>(n, TrackedAllocatorSTL<T>(cat));
    }
}