#pragma once

#include "common/common.hpp"
#include "common/memory/memory.hpp"
#include <atomic>

namespace rn
{
    enum class ResourceIndex : uint32_t
    {
        Invalid = 0xFFFFFFFF
    };

    class IndexAllocator
    {
    public:

        IndexAllocator(MemoryCategoryID cat, size_t capacity);
        ~IndexAllocator();

        IndexAllocator(const IndexAllocator&) = delete;
        IndexAllocator& operator=(const IndexAllocator&) = delete;

        IndexAllocator(IndexAllocator&&);
        IndexAllocator& operator=(IndexAllocator&&);

        ResourceIndex Allocate();
        void Free(ResourceIndex index);

    private:

        int32_t _capacity = 0;
        std::atomic_uint64_t _freePtrAndSentinel = 0;
        ResourceIndex* _nodeList = nullptr;
    };
}