#pragma once

#include "common/memory/memory.hpp"
#include "ankerl/unordered_dense.h"

namespace rn
{
    template <class Key,
              class Hash = ankerl::unordered_dense::hash<Key>>
    using HashSet = ankerl::unordered_dense::set<Key, Hash, std::equal_to<Key>, TrackedAllocatorSTL<Key>>;

    template <class Key,
              class Hash = ankerl::unordered_dense::hash<Key>>
    using ScopedHashSet = ankerl::unordered_dense::set<Key, Hash, std::equal_to<Key>, ScopedAllocatorSTL<Key>>;

    template <class Key,
              class Hash = ankerl::unordered_dense::hash<Key>>
    HashSet<Key, Hash> MakeHashSet(MemoryCategoryID cat)
    {
        return HashSet<Key, Hash>(TrackedAllocatorSTL<Key>(cat));
    }
}