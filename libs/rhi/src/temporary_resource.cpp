#include "rhi/temporary_resource.hpp"
#include "rhi/device.hpp"

namespace rn::rhi
{
    TemporaryResourceAllocator::TemporaryResourceAllocator(
            Device* device,
            uint32_t maxFrameLatency,
            GPUAllocationFlags allocationFlags,
            BufferCreationFlags bufferFlags,
            FnMapMemory mapFn, 
            FnUnmapMemory unmapFn)
        : _parentDevice(device)
        , _mapFn(mapFn)
        , _unmapFn(unmapFn)
        , _allocFlags(allocationFlags)
        , _creationFlags(bufferFlags)
        , _maxFrameLatency(maxFrameLatency)
    {
        _pages.push_back(AllocateNewPage(TEMPORARY_ALLOCATOR_PAGE_SIZE));
        _retiredPageQueues = TrackedNewArray<RetiredPageQueue>(MemoryCategory::RHI, maxFrameLatency);
    }

    TemporaryResourceAllocator::~TemporaryResourceAllocator()
    {
        Reset();
        TrackedDeleteArray(_retiredPageQueues);
    }

    void TemporaryResourceAllocator::Reset()
    {
        for (Page& p : _pages)
        {
            DestroyPage(p);
        }

        for (Page& p : _fallbackPages)
        {
            DestroyPage(p);
        }

        _pages.clear();
        _fallbackPages.clear();
    }

    TemporaryResource TemporaryResourceAllocator::AllocateTemporaryResource(uint32_t size, uint32_t alignment)
    {
        _offsetInCurrentPage = uint32_t(AlignSize(_offsetInCurrentPage, std::max(TEMPORARY_RESOURCE_ALIGNMENT, alignment)));
        size = uint32_t(AlignSize(size, TEMPORARY_RESOURCE_ALIGNMENT));

        TemporaryResource resource = {};

        if (size > TEMPORARY_RESOURCE_THRESHOLD_SIZE)
        {
            Page fallbackPage = AllocateNewPage(size);
            _fallbackPages.push_back(fallbackPage);

            resource = {
                .buffer = fallbackPage.buffer,
                .cpuPtr = fallbackPage.mappedPtr,
                .offsetInBytes = 0,
                .sizeInBytes = size
            };
        }
        else
        {
            if (_offsetInCurrentPage + size > TEMPORARY_ALLOCATOR_PAGE_SIZE)
            {
                RetireCurrentPage();
            }

            Page& currentPage = _pages[_currentPageIdx];
            resource = {
                .buffer = currentPage.buffer,
                .cpuPtr = static_cast<char*>(currentPage.mappedPtr) + _offsetInCurrentPage,
                .offsetInBytes = _offsetInCurrentPage,
                .sizeInBytes = size
            };

            _offsetInCurrentPage += size;
        }

        return resource;
    }

    void TemporaryResourceAllocator::Flush(uint64_t currentFrameIndex)
    {
        for (Page& p : _fallbackPages)
        {
            DestroyPage(p);
        }
        _fallbackPages.clear();

        RetiredPageQueue& queue = _retiredPageQueues[currentFrameIndex % _maxFrameLatency];
        for (RetiredPage& p : queue.pages)
        {
            _availablePages.push_back(p.pageIdx);
        }

        _currentFrameIndex = currentFrameIndex;
    }

    TemporaryResourceAllocator::Page TemporaryResourceAllocator::AllocateNewPage(uint64_t size)
    {
        GPUAllocation alloc = _parentDevice->GPUAlloc(MemoryCategory::RHI, size, _allocFlags);
        Buffer b = _parentDevice->CreateBuffer({
            .flags = _creationFlags,
            .size = uint32_t(size),
            .name = "Temporary Resource Page"
        },
        {
            .allocation = alloc,
            .offsetInAllocation = 0,
            .regionSize = size
        });

        Page page =
        {
            .alloc = alloc,
            .buffer = b,
            .mappedPtr = _mapFn(_parentDevice, b, 0, size),
            .size = size
        };

        return page;
    }

    void TemporaryResourceAllocator::DestroyPage(Page& page)
    {
        _unmapFn(_parentDevice, page.buffer, 0, page.size);
        _parentDevice->Destroy(page.buffer);
        _parentDevice->GPUFree(page.alloc);
    }

    void TemporaryResourceAllocator::RetireCurrentPage()
    {
        RetiredPage retiredPage = {
            .pageIdx = _currentPageIdx,
            .frameIdx = _currentFrameIndex
        };
        _retiredPageQueues[_currentFrameIndex % _maxFrameLatency].pages.push_back(retiredPage);

        if (!_availablePages.empty())
        {
            _currentPageIdx = _availablePages.back();
            _availablePages.pop_back();
        }
        else
        {
            Page newPage = AllocateNewPage(TEMPORARY_ALLOCATOR_PAGE_SIZE);
            _currentPageIdx = uint32_t(_pages.size());
            _pages.push_back(newPage);
        }

        _offsetInCurrentPage = 0;
    }
}