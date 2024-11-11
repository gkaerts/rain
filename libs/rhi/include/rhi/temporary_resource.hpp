#pragma once

#include "common/common.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/vector.hpp"
#include "common/handle.hpp"

namespace rn::rhi
{
    RN_HANDLE(GPUAllocation);
    RN_HANDLE(Buffer);

    RN_MEMORY_CATEGORY(RHI);

    class Device;
    enum class GPUAllocationFlags : uint32_t;
    enum class BufferCreationFlags : uint32_t;

    using FnMapMemory = void*(*)(Device*, GPUAllocation, uint64_t, uint64_t);
    using FnUnmapMemory = void(*)(Device*, GPUAllocation);

    constexpr const uint32_t TEMPORARY_ALLOCATOR_PAGE_SIZE = 4 * MEGA;
    constexpr const uint32_t TEMPORARY_RESOURCE_THRESHOLD_SIZE = 3 * MEGA;
    constexpr const uint32_t TEMPORARY_RESOURCE_ALIGNMENT = 512;

    struct TemporaryResource
    {
        Buffer buffer;
        void* cpuPtr;

        uint32_t offsetInBytes;
        uint32_t sizeInBytes;
    };

    class TemporaryResourceAllocator
    {
    public:

        TemporaryResourceAllocator(
            Device* device, 
            uint32_t maxFrameLatency,
            GPUAllocationFlags allocationFlags,
            BufferCreationFlags bufferFlags,
            FnMapMemory mapFn, 
            FnUnmapMemory unmapFn);

        ~TemporaryResourceAllocator();

        TemporaryResourceAllocator(TemporaryResourceAllocator&) = delete;
        TemporaryResourceAllocator(TemporaryResourceAllocator&&) = delete;
        TemporaryResourceAllocator& operator=(TemporaryResourceAllocator&) = delete;
        TemporaryResourceAllocator& operator=(TemporaryResourceAllocator&&) = delete; 

        TemporaryResource AllocateTemporaryResource(uint32_t size, uint32_t alignment);
        void Flush(uint64_t currentFrameIndex);

    private:

        struct Page
        {
            GPUAllocation alloc;
            Buffer buffer;

            void* mappedPtr;
        };

        struct RetiredPage
        {
            uint32_t pageIdx;
            uint64_t frameIdx;
        };

        struct RetiredPageQueue
        {
            Vector<RetiredPage> pages = MakeVector<RetiredPage>(MemoryCategory::RHI);
        };

        Page AllocateNewPage(uint64_t size);
        void DestroyPage(Page& page);
        void RetireCurrentPage();

        Vector<Page> _pages = MakeVector<Page>(MemoryCategory::RHI);
        Vector<Page> _fallbackPages = MakeVector<Page>(MemoryCategory::RHI);

        Vector<uint32_t> _availablePages = MakeVector<uint32_t>(MemoryCategory::RHI);
        RetiredPageQueue* _retiredPageQueues = nullptr;
        
        Device* _parentDevice = nullptr;

        FnMapMemory _mapFn;
        FnUnmapMemory _unmapFn;

        GPUAllocationFlags _allocFlags;
        BufferCreationFlags _creationFlags;

        uint32_t _currentPageIdx = 0;
        uint32_t _offsetInCurrentPage = 0;

        uint64_t _currentFrameIndex = 0;
        uint32_t _maxFrameLatency = 0;
    };
}