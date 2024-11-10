#pragma once

#include "common/common.hpp"

namespace rn
{
    enum class MemoryCategoryID : uint8_t;

    class BumpAllocator
    {
    public:

        BumpAllocator(MemoryCategoryID cat, size_t capacity);
        ~BumpAllocator();

        BumpAllocator(const BumpAllocator&) = delete;
        BumpAllocator& operator=(const BumpAllocator&) = delete;

        BumpAllocator(BumpAllocator&& rhs) = default;
        BumpAllocator& operator=(BumpAllocator&& rhs) = default;

        void* Allocate(size_t size, size_t alignment);

        template <typename T>
        T* AllocatePOD()
        {
            return static_cast<T*>(Allocate(sizeof(T), alignof(T)));
        }

        void Purge();

        void Rewind(size_t size);
        void Reset();

    private:

        char* _virtualPtr = nullptr;
        char* _virtualEnd = nullptr;
        size_t _pageSize = 0;

        char* _physicalCurrent = nullptr;
        char* _physicalEnd = nullptr;

        MemoryCategoryID _category;
    };
}