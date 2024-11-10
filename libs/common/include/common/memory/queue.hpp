#pragma once

#include "common/memory/memory.hpp"
#include "common/memory/deque.hpp"
#include <queue>

namespace rn
{
    template <typename T>
    using Queue = std::queue<T, Deque<T>>;

    template <typename T>
    using ScopedQueue = std::queue<T, ScopedDeque<T>>;

    template <typename T>
    Queue<T> MakeQueue(MemoryCategoryID cat)
    {
        return Queue<T>(TrackedAllocatorSTL<T>(cat));
    }
}