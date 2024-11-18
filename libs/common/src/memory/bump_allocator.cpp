#include "common/memory/bump_allocator.hpp"
#include "common/memory/memory.hpp"

namespace rn
{
    BumpAllocator::BumpAllocator(MemoryCategoryID cat, size_t capacity)
        : _pageSize(SystemPageSize())
    {
        capacity = AlignSize(capacity, _pageSize);
        _virtualPtr = static_cast<char*>(ReserveVirtualAddressSpace(capacity));
        _virtualEnd = _virtualPtr + capacity;
        _physicalCurrent = _virtualPtr;
        _physicalEnd = _virtualPtr;
        _category = cat;

        TrackExternalAllocation(cat, capacity);
    }

    BumpAllocator::~BumpAllocator()
    {
        uintptr_t capacity = _virtualEnd - _virtualPtr;
        FreeVirtualAddressSpace(_virtualPtr, capacity);
        TrackExternalFree(_category, capacity);
    }

    void* BumpAllocator::Allocate(size_t size, size_t alignment)
    {
        if (!size)
        {
            return nullptr;
        }

        char* physicalCurrent = static_cast<char*>(AlignPtr(_physicalCurrent, alignment));
        if (physicalCurrent + size > _physicalEnd)
        {
            // Out of commited pages. Allocate more.
            uintptr_t requiredSize = (physicalCurrent + size) - _physicalEnd;
            uintptr_t requiredPageCount = (requiredSize + (_pageSize - 1)) / _pageSize;
            uintptr_t roundedSize = requiredPageCount * _pageSize;
            
            RN_ASSERT(_physicalEnd + roundedSize <= _virtualEnd);

            CommitVirtualAddressSpace(_physicalEnd, size_t(roundedSize));
            _physicalEnd = _physicalEnd + roundedSize;
        }

        _physicalCurrent = physicalCurrent + size;
        return _physicalCurrent;
    }

    void BumpAllocator::Purge()
    {
        char* alignedCurrent = static_cast<char*>(AlignPtr(_physicalCurrent, _pageSize));

        uintptr_t unallocatedPhysicalRange = _physicalEnd - alignedCurrent;
        if (unallocatedPhysicalRange > _pageSize)
        {
            uintptr_t decommitSize = (unallocatedPhysicalRange / _pageSize) * _pageSize;
            DecommitVirtualAddressSpace(alignedCurrent, decommitSize);
            _physicalEnd = alignedCurrent + (decommitSize - unallocatedPhysicalRange);
        }
    }

    void BumpAllocator::Rewind(size_t size)
    {
        uintptr_t allocatedRange = _physicalCurrent - _virtualPtr;
        RN_ASSERT(size <= allocatedRange);

        _physicalCurrent = _physicalCurrent - size;
    }

    void BumpAllocator::Reset()
    {
        _physicalCurrent = _virtualPtr;
    }
}