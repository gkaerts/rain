#pragma once

#include "common/common.hpp"
#include "common/handle.hpp"
#include "common/memory/vector.hpp"
#include "rhi/resource.hpp"

namespace rn::rhi
{
    class Device;

    class TransientResourceAllocator
    {
    public:
        TransientResourceAllocator(MemoryCategoryID cat, Device* device, uint32_t pageCountPerBackingAllocation);
        ~TransientResourceAllocator();

        TransientResourceAllocator(const TransientResourceAllocator&) = delete;
        TransientResourceAllocator& operator=(const TransientResourceAllocator&) = delete;

        TransientResourceAllocator(TransientResourceAllocator&&);
        TransientResourceAllocator& operator=(TransientResourceAllocator&&);

        GPUMemoryRegion AllocateMemoryRegion(uint32_t sizeInBytes);
        void FreeMemoryRegion(const GPUMemoryRegion& region);

    private:

        struct PageRange
        {
            uint32_t startPage;
            uint32_t pageCount;
        };

        struct Allocation
        {
            GPUAllocation allocation;
            uint32_t pageCount;

            Vector<PageRange> freePageRanges;
            Vector<PageRange> usedPageRanges;
        };

        void NewBackingAllocation();
        bool TryAllocatePageRange(Allocation& a, uint32_t pageCount, GPUMemoryRegion& outRegion);
        void FreePageRange(Allocation& a, const PageRange& range);

        void InsertPageRange(Vector<PageRange>& rangeList, const PageRange& newRange);

        Device* _device;
        MemoryCategoryID _memCat;
        Vector<Allocation> _allocations;
        uint32_t _pageCountPerBackingAllocation;
    };
}