#include "common/memory/memory.hpp"
#include <type_traits>
#include <atomic>

#if RN_PLATFORM_WINDOWS
    #define WIN32_LEAN_AND_MEAN
    #include <Windows.h>
#endif

namespace rn
{
    RN_DEFINE_MEMORY_CATEGORY(Default)

    namespace
    {
        constexpr const uint8_t MAX_MEMORY_CATEGORY_COUNT = UINT8_MAX;

        uint8_t& MemoryCategoryCounter()
        {
            static uint8_t categoryCounter = 0;
            return categoryCounter;
        }

        struct MemoryCategoryStats
        {
            std::atomic_size_t numAllocations;
            std::atomic_size_t allocatedSize;

            std::atomic_size_t totalAllocationCount;
            std::atomic_size_t totalFreeCount;
        };

        const char* MEMORY_CATEGORY_NAMES[MAX_MEMORY_CATEGORY_COUNT] = {};
        MemoryCategoryStats MEMORY_CATEGORY_STATS[MAX_MEMORY_CATEGORY_COUNT] = {};
    }

    MemoryCategoryID AllocateMemoryCategoryID(const char* name)
    {
        uint8_t& counter = MemoryCategoryCounter();

        RN_ASSERT(counter < MAX_MEMORY_CATEGORY_COUNT);

        const uint8_t categoryID = counter++;
        MEMORY_CATEGORY_NAMES[categoryID] = name;

        return MemoryCategoryID(categoryID);
    }

    uint8_t NumMemoryCategories()
    {
        return MemoryCategoryCounter();
    }

    namespace
    {
        inline bool IsValidMemoryCategory(MemoryCategoryID cat)
        {
            return uint8_t(cat) < NumMemoryCategories();
        }
    }

    MemoryCategoryInfo MemoryInfoForCategory(MemoryCategoryID cat)
    {
        RN_ASSERT(IsValidMemoryCategory(cat));

        const uint8_t catIdx = uint8_t(cat);
        MemoryCategoryInfo info
        {
            .numAllocations = MEMORY_CATEGORY_STATS[catIdx].numAllocations,
            .allocatedSize = MEMORY_CATEGORY_STATS[catIdx].allocatedSize,
            .totalAllocationCount = MEMORY_CATEGORY_STATS[catIdx].totalAllocationCount,
            .totalFreeCount = MEMORY_CATEGORY_STATS[catIdx].totalFreeCount
        };

        return info;
    }

    const char* MemoryCategoryName(MemoryCategoryID cat)
    {
        RN_ASSERT(IsValidMemoryCategory(cat));
        return MEMORY_CATEGORY_NAMES[uint8_t(cat)];
    }

    void TrackExternalAllocation(MemoryCategoryID cat, size_t size)
    {
        RN_ASSERT(IsValidMemoryCategory(cat));
        MemoryCategoryStats& stats = MEMORY_CATEGORY_STATS[uint8_t(cat)];
        stats.numAllocations++;
        stats.allocatedSize += size;
        stats.totalAllocationCount++;
    }

    void TrackExternalFree(MemoryCategoryID cat, size_t size)
    {
        RN_ASSERT(IsValidMemoryCategory(cat));
        MemoryCategoryStats& stats = MEMORY_CATEGORY_STATS[uint8_t(cat)];
        stats.numAllocations--;
        stats.allocatedSize -= size;
        stats.totalFreeCount++;
    }

    namespace 
    {
        struct AllocationTrackingBlock
        {
            uint32_t category;
            uint32_t alignmentOffset;
            size_t size;
        };
        static_assert(sizeof(AllocationTrackingBlock) == 16);
    }

    void* TrackedAlloc(MemoryCategoryID cat, size_t size, size_t alignment)
    {
        RN_ASSERT(IsValidMemoryCategory(cat));

        if (!IsPowerOfTwo(alignment))
        {
            return nullptr;
        }

        size_t actualSize = sizeof(AllocationTrackingBlock) + size;
        if (alignment > 0)
        {
            actualSize += (alignment - 1);
        }

        uintptr_t uBasePtr = uintptr_t(malloc(actualSize));
        uintptr_t uDataPtr = AlignSize(uBasePtr + sizeof(AllocationTrackingBlock), alignment);
        uintptr_t uTrackingPtr = uDataPtr - sizeof(AllocationTrackingBlock); 

        AllocationTrackingBlock* trackingBlock = reinterpret_cast<AllocationTrackingBlock*>(uTrackingPtr);
        trackingBlock->category = uint32_t(cat);
        trackingBlock->size = size;
        trackingBlock->alignmentOffset = uint32_t(uDataPtr - uBasePtr);

        TrackExternalAllocation(cat, size);

        return reinterpret_cast<void*>(uDataPtr);
    }

    void TrackedFree(void* ptr)
    {
    

        uintptr_t uDataPtr = uintptr_t(ptr);
        uintptr_t uTrackingPtr = uDataPtr - sizeof(AllocationTrackingBlock);
        AllocationTrackingBlock* trackingBlock = reinterpret_cast<AllocationTrackingBlock*>(uTrackingPtr);

        uintptr_t uBasePtr = uDataPtr - trackingBlock->alignmentOffset;
        
        MemoryCategoryID cat = MemoryCategoryID(trackingBlock->category);
        size_t size = trackingBlock->size;

        void* basePtr = reinterpret_cast<void*>(uBasePtr);
        free(basePtr);

        TrackExternalFree(cat, size);
    }

    // Scope allocation
    namespace
    {
        struct ScopedAllocatorBacking
        {
            char* ptr = nullptr;
            ptrdiff_t offset = 0;
            size_t capacity = 0;
        };

        thread_local ScopedAllocatorBacking THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING;
    }

    MemoryScope::MemoryScope()
        : offset(CurrentScopeOffset())
        , lastDeleter(nullptr)
    {};

    MemoryScope::~MemoryScope()
    {
        Deleter* d = lastDeleter;
        while (d != nullptr)
        {
            if (d->fn)
            {
                d->fn(d->dataPtr);
            }

            d = d->prevDeleter;
        }

        lastDeleter = nullptr;
        ResetScope(offset);
    }

    void InitializeScopedAllocationForThread(size_t backingSize)
    {
        RN_ASSERT(THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr == nullptr);

        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr = (char*)TrackedAlloc(MemoryCategory::Default, backingSize, 16);
        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.offset = 0;
        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.capacity = backingSize;
    }

    void TeardownScopedAllocationForThread()
    {
        if (THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr)
        {
            TrackedFree(THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr);
        }

        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING = {};
    }

    ptrdiff_t CurrentScopeOffset()
    {
        return THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.offset;
    }

    void ResetScope(ptrdiff_t offset)
    {
         if (!THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr || 
              size_t(offset) >= THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.capacity)
        {
            return;
        }

        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.offset = offset;
    }

    void* ScopedAlloc(size_t size, size_t alignment)
    {
        if (!THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr)
        {
            return nullptr;
        }

        uintptr_t basePtr = uintptr_t(THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.ptr);
        uintptr_t alignedAddress = AlignSize(basePtr + THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.offset, alignment);
        uintptr_t alignedOffset = alignedAddress - basePtr;

        if (alignedOffset + size >= THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.capacity)
        {
            return nullptr;
        }

        THREAD_LOCAL_SCOPED_ALLOCATOR_BACKING.offset = alignedOffset + size;
        return (void*)alignedAddress;
    }
    
    void ScopedFree(void* ptr)
    {}


    size_t SystemPageSize()
    {
    #if RN_PLATFORM_WINDOWS
        SYSTEM_INFO sysInfo{};
        GetSystemInfo(&sysInfo);

        return sysInfo.dwPageSize;
    #else
        #error SystemPageSize not implemented on this platform
    #endif
    }


    void* ReserveVirtualAddressSpace(size_t size)
    {
    #if RN_PLATFORM_WINDOWS
        return VirtualAlloc(nullptr, size, MEM_RESERVE, PAGE_READWRITE);

    #else
        #error ReserveVirtualAddressSpace not implemented on this platform
    #endif
    }

    void FreeVirtualAddressSpace(void* vptr, size_t size)
    {
    #if RN_PLATFORM_WINDOWS
        VirtualFree(vptr, size, MEM_RELEASE);

    #else
        #error FreeVirtualAddressSpace not implemented on this platform
    #endif
    }

    void CommitVirtualAddressSpace(void* vptr, size_t size)
    {
    #if RN_PLATFORM_WINDOWS
        VirtualAlloc(vptr, size, MEM_COMMIT, PAGE_READWRITE);

    #else
        #error CommitVirtualAddressSpace not implemented on this platform
    #endif
    }

    void DecommitVirtualAddressSpace(void* pptr, size_t size)
    {
    #if RN_PLATFORM_WINDOWS
        VirtualFree(pptr, size, MEM_DECOMMIT);

    #else
        #error DecommitVirtualAddressSpace not implemented on this platform
    #endif
    }

}