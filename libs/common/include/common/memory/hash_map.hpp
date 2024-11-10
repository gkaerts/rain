#pragma once

#include "common/memory/memory.hpp"
#include "ankerl/unordered_dense.h"

namespace rn
{
    template <class Key, 
              class T,
              class Hash = ankerl::unordered_dense::hash<Key>>
    using HashMap = ankerl::unordered_dense::map<Key, T, Hash, std::equal_to<Key>, TrackedAllocatorSTL<std::pair<Key, T>>>;

    template <class Key, 
              class T,
              class Hash = ankerl::unordered_dense::hash<Key>>
    using ScopedHashMap = ankerl::unordered_dense::map<Key, T, Hash, std::equal_to<Key>, ScopedAllocatorSTL<std::pair<Key, T>>>;

    template <class Key, 
              class T,
              class Hash = ankerl::unordered_dense::hash<Key>>
    HashMap<Key, T, Hash> MakeHashMap(MemoryCategoryID cat)
    {
        return HashMap<Key, T, Hash>(TrackedAllocatorSTL<T>(cat));
    }
}