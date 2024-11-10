#pragma once

#include "common/memory/memory.hpp"
#include <deque>

namespace rn
{
    template <typename T>
    using Deque = std::deque<T, TrackedAllocatorSTL<T>>;

    template <typename T>
    using ScopedDeque = std::deque<T, ScopedAllocatorSTL<T>>;

    template <typename T>
    Deque<T> MakeDeque(MemoryCategoryID cat)
    {
        return Deque<T>(TrackedAllocatorSTL<T>(cat));
    }
}