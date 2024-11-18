#include "rhi/transient_resource.hpp"
#include "rhi/device.hpp"

namespace rn::rhi
{
    namespace
    {
        constexpr size_t TRANSIENT_RESOURCE_PAGE_SIZE = 64 * KILO;
    }

    TransientResourceAllocator::TransientResourceAllocator(MemoryCategoryID cat, Device* device, uint32_t pageCountPerBackingAllocation)
        : _device(device)
        , _memCat(cat)
        , _pageCountPerBackingAllocation(pageCountPerBackingAllocation)
        , _allocations(MakeVector<Allocation>(cat))
    {
       NewBackingAllocation();
    }

    TransientResourceAllocator::~TransientResourceAllocator()
    {
        for (Allocation& allocation : _allocations)
        {
            _device->GPUFree(allocation.allocation);
        }
    }

    void TransientResourceAllocator::NewBackingAllocation()
    {
         _allocations.push_back({
            .allocation = _device->GPUAlloc(_memCat, _pageCountPerBackingAllocation * TRANSIENT_RESOURCE_PAGE_SIZE, GPUAllocationFlags::DeviceOnly),
            .pageCount = _pageCountPerBackingAllocation,
            .freePageRanges = MakeVector<PageRange>(_memCat),
            .usedPageRanges = MakeVector<PageRange>(_memCat)
        });

        _allocations.back().freePageRanges.push_back({
            .startPage = 0,
            .pageCount = _pageCountPerBackingAllocation
        });
    }

    void TransientResourceAllocator::InsertPageRange(Vector<PageRange>& rangeList, const PageRange& newRange)
    {
        auto endIt = rangeList.end();
        auto prevIt = endIt;
        auto nextIt = endIt;
        for (auto it = rangeList.begin(); it != endIt; ++it)
        {
            const PageRange& rangeToTest = *it;
            if (rangeToTest.startPage + rangeToTest.pageCount == newRange.startPage)
            {
                prevIt = it;
            }

            if (newRange.startPage + newRange.pageCount == rangeToTest.startPage)
            {
                nextIt = it;
            }

            if (prevIt != endIt && nextIt != endIt)
            {
                break;
            }
        }

        if (prevIt != endIt && nextIt != endIt)
        {
            // Page range sat right in between two listed page ranges. Merge them all together.
            prevIt->pageCount += newRange.pageCount + nextIt->pageCount;
            rangeList.erase(nextIt);
        }
        else if (prevIt != endIt)
        {
            // Range has a neighbor to the left. Update it
            prevIt->pageCount += newRange.pageCount;
        }
        else if (nextIt != endIt)
        {
            // Range has a neighbor to the right. Update it
            nextIt->startPage = newRange.startPage;
            nextIt->pageCount += newRange.pageCount;
        }
        else
        {
            // No neighbor ranges. Push the range onto the page range list
            rangeList.push_back(newRange);
        }
    }

    bool TransientResourceAllocator::TryAllocatePageRange(Allocation& a, uint32_t pageCount, GPUMemoryRegion& outRegion)
    {
        if (a.freePageRanges.empty())
        {
            return false;
        }
        
        GPUAllocation gpuAlloc = GPUAllocation::Invalid;
        uint32_t startPage = 0;
        for (auto it = a.freePageRanges.begin(); it != a.freePageRanges.end(); ++it)
        {
            PageRange& range = *it;

            if (range.pageCount >= pageCount)
            {
                startPage = range.startPage;
                gpuAlloc = a.allocation;

                range.pageCount -= pageCount;
                range.startPage += pageCount;

                if (!range.pageCount)
                {
                    a.freePageRanges.erase(it);
                }

                InsertPageRange(a.usedPageRanges, { 
                    .startPage = startPage, 
                    .pageCount = pageCount });
                break;
            }
        }

        return gpuAlloc != GPUAllocation::Invalid;
    }

    GPUMemoryRegion TransientResourceAllocator::AllocateMemoryRegion(uint32_t sizeInBytes)
    {
        RN_ASSERT(sizeInBytes > 0);
        RN_ASSERT(sizeInBytes <= _pageCountPerBackingAllocation * TRANSIENT_RESOURCE_PAGE_SIZE);

        GPUMemoryRegion outRegion = {};
        uint32_t pageCount = (sizeInBytes + (TRANSIENT_RESOURCE_PAGE_SIZE - 1)) / TRANSIENT_RESOURCE_PAGE_SIZE;
        for (Allocation& a : _allocations)
        {
            if (TryAllocatePageRange(a, pageCount, outRegion))
            {
                break;
            }
        }

        if (outRegion.allocation == GPUAllocation::Invalid)
        {
            NewBackingAllocation();
            RN_ASSERT(TryAllocatePageRange(_allocations.back(), pageCount, outRegion));
        }

        return outRegion;
    }

    void TransientResourceAllocator::FreePageRange(Allocation& a, const PageRange& range)
    {
        RN_ASSERT(range.pageCount > 0);
        RN_ASSERT(range.startPage < a.pageCount);
        RN_ASSERT(range.startPage + range.pageCount < a.pageCount);

        // Find the used page range the given range falls into
        PageRange freedRange{};
        const uint32_t startPage = range.startPage;
        const uint32_t endPage = startPage + range.pageCount;
        for (auto it = a.usedPageRanges.begin(); it != a.usedPageRanges.end(); ++it)
        {
            PageRange& usedRange = *it;
            const uint32_t usedStartPage = usedRange.startPage;
            const uint32_t usedEndPage = usedStartPage + usedRange.pageCount;

            if (startPage >= usedStartPage && startPage < usedEndPage &&
                endPage > usedStartPage && endPage <= usedEndPage)
            {
                // Found it. Chop it up.
                PageRange previousUsedRange = {
                    .startPage = usedStartPage,
                    .pageCount = startPage - usedStartPage
                };

                PageRange nextUsedRange = {
                    .startPage = endPage,
                    .pageCount = usedEndPage - endPage
                };

                if (previousUsedRange.pageCount == 0 && nextUsedRange.pageCount == 0)
                {
                    // Range encompassed the entirety of this used page range. Nuke it from the used page range entirely
                    a.usedPageRanges.erase(it);
                }
                else if (previousUsedRange.pageCount == 0)
                {
                    // Range sat at the start of this used page range. Update the existing used page block.
                    usedRange.startPage += range.pageCount;
                    usedRange.pageCount -= range.pageCount;
                }
                else if (nextUsedRange.pageCount == 0)
                {
                    // Range sat at the end of this used page range. Update the existing used page block
                    usedRange.pageCount -= range.pageCount;
                }
                else
                {
                    // Update the existing page range to reflect the "previous" block, and add a new entry for the "next" block
                    usedRange = previousUsedRange;
                    a.usedPageRanges.push_back(nextUsedRange);
                }

                freedRange = { 
                    .startPage = range.startPage, 
                    .pageCount = range.pageCount 
                };

                break;
            }
        }

        RN_ASSERT(freedRange.pageCount > 0);
        InsertPageRange(a.freePageRanges, freedRange);
    }

    void TransientResourceAllocator::FreeMemoryRegion(const rhi::GPUMemoryRegion& region)
    {
        auto it = std::find_if(_allocations.begin(), _allocations.end(), [&](const Allocation& a) {
            return a.allocation == region.allocation;
        });

        RN_ASSERT(it != _allocations.end());

        FreePageRange(*it, {
            .startPage = uint32_t(region.offsetInAllocation / TRANSIENT_RESOURCE_PAGE_SIZE),
            .pageCount = uint32_t(region.regionSize / TRANSIENT_RESOURCE_PAGE_SIZE)
        });
    }
}