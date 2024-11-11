#include "common/memory/index_allocator.hpp"

namespace rn
{
    IndexAllocator::IndexAllocator(MemoryCategoryID cat, size_t capacity)
        : _capacity(int32_t(capacity))
    {
        if (_capacity > 0)
        {
            _nodeList = TrackedNewArray<ResourceIndex>(cat, capacity);

            // Set up the free pointer chain
            for (size_t i = 0; i < capacity; ++i)
            {
                _nodeList[i] = ResourceIndex(i + 1);
            }
        }
    }

    IndexAllocator::~IndexAllocator()
    {
        if (_nodeList)
        {
            TrackedDeleteArray(_nodeList);
        }
    }

    IndexAllocator::IndexAllocator(IndexAllocator&& rhs)
        : _capacity(rhs._capacity)
        , _freePtrAndSentinel(rhs._freePtrAndSentinel.load())
        , _nodeList(rhs._nodeList)
    {
        rhs._capacity = 0;
        rhs._freePtrAndSentinel = 0;
        rhs._nodeList = nullptr;
    }

    IndexAllocator& IndexAllocator::operator=(IndexAllocator&& rhs)
    {
        if (_nodeList)
        {
            TrackedDeleteArray(_nodeList);
        }

        _capacity = rhs._capacity;
        _freePtrAndSentinel = rhs._freePtrAndSentinel.load();
        _nodeList = rhs._nodeList;

        rhs._capacity = 0;
        rhs._freePtrAndSentinel = 0;
        rhs._nodeList = nullptr;

        return *this;
    }

    ResourceIndex IndexAllocator::Allocate()
    {
        ResourceIndex index = ResourceIndex::Invalid;

        while (true)
        {
            uint64_t freePtrAndSentinel = _freePtrAndSentinel.load();
            index = ResourceIndex(freePtrAndSentinel & 0xFFFFFFFF);

            if (int32_t(index) >= _capacity)
            {
                index = ResourceIndex::Invalid;
                break;
            }

            uint64_t sentinel = (freePtrAndSentinel & 0xFFFFFFFF00000000) >> 32;
            ResourceIndex nextFreePtr = _nodeList[uint32_t(index)];

            if (_freePtrAndSentinel.compare_exchange_weak(
                freePtrAndSentinel,
                (((sentinel + 1) & 0xFFFFFFFF00000000) << 32) | (uint64_t(nextFreePtr) & 0xFFFFFFFF)))
            {
                break;
            }
        }

        return index;
    }

    void IndexAllocator::Free(ResourceIndex index)
    {
        if (index == ResourceIndex::Invalid)
        {
            return;
        }

        bool handlePushed = false;
        while (!handlePushed)
        {
            uint64_t freePtrAndSentinel = _freePtrAndSentinel.load();
            ResourceIndex nextFreePtr = ResourceIndex(freePtrAndSentinel & 0xFFFFFFFF);
            uint64_t sentinel = (freePtrAndSentinel & 0xFFFFFFFF00000000) >> 32;

            _nodeList[uint32_t(index)] = nextFreePtr;

            if (_freePtrAndSentinel.compare_exchange_weak(
                freePtrAndSentinel,
                (((sentinel + 1) & 0xFFFFFFFF00000000) << 32) | (uint64_t(index) & 0xFFFFFFFF)))
            {
                handlePushed = true;
            }
        }

        RN_ASSERT(handlePushed);
    }

}