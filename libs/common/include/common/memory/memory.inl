#include "common/memory/memory.hpp"
#include <utility>

namespace rn
{
    template <typename T, typename... Args> 
    T* TrackedNew(MemoryCategoryID cat, Args&&... args)
    {
        void* ptr = TrackedAlloc(cat, sizeof(T), alignof(T));
        return new (ptr) T(std::forward<Args>(args)...);
    }

    template <typename T> 
    T* TrackedNewArray(MemoryCategoryID cat, size_t size)
    {
        size_t preambleSizeInBytes = AlignSize(sizeof(size_t), alignof(T));
        size_t sizeInBytes = 
            preambleSizeInBytes + 
            AlignSize(sizeof(T), alignof(T)) * size;

        void* basePtr = TrackedAlloc(cat, sizeInBytes, alignof(T));
        void* dataPtr = reinterpret_cast<void*>(uintptr_t(basePtr) + preambleSizeInBytes);
        void* sizePtr = reinterpret_cast<void*>(uintptr_t(dataPtr) - sizeof(size_t));

        *reinterpret_cast<size_t*>(sizePtr) = size;
        return new (dataPtr) T[size];
    }

    template <typename T> 
    void TrackedDelete(T* ptr)
    {
        ptr->~T();
        TrackedFree(ptr);
    }

    template <typename T> 
    void TrackedDeleteArray(T* ptr)
    {
        uintptr_t dataPtr = uintptr_t(ptr);
        uintptr_t sizePtr = dataPtr - sizeof(size_t);

        size_t size = *reinterpret_cast<size_t*>(sizePtr);
        for (size_t i = 0; i < size; ++i)
        {
            ptr[i].~T();
        }

        uintptr_t basePtr = dataPtr - AlignSize(sizeof(size_t), alignof(T));
        TrackedFree(reinterpret_cast<void*>(basePtr));
    }

    template <typename T, typename... Args> T* ScopedNew(MemoryScope& scope, Args&&... args)
    {
        void* ptr = ScopedAlloc(sizeof(T), alignof(T));
        if (!ptr)
        {
            return nullptr;
        }

        scope.PushDeleter(ptr, [](void* ptr) { ((T*)ptr)->~T(); });
        return new (ptr) T(std::forward<Args>(args)...);
    }

    template <typename T> T* ScopedNewArray(MemoryScope& scope, size_t size)
    {
        return new (ScopedNewArrayNoInit<T>(scope, size)) T[size];
    }

    template <typename T> T* ScopedNewArrayNoInit(MemoryScope& scope, size_t size)
    {
        size_t preambleSizeInBytes = AlignSize(sizeof(size_t), alignof(T));
        size_t sizeInBytes = 
            preambleSizeInBytes + 
            AlignSize(sizeof(T), alignof(T)) * size;

        void* basePtr = ScopedAlloc(sizeInBytes, alignof(T));
        if (!basePtr)
        {
            return nullptr;
        }

        void* dataPtr = reinterpret_cast<void*>(uintptr_t(basePtr) + preambleSizeInBytes);
        void* sizePtr = reinterpret_cast<void*>(uintptr_t(dataPtr) - sizeof(size_t));

        *reinterpret_cast<size_t*>(sizePtr) = size;
        scope.PushDeleter(dataPtr, [](void* ptr) 
        { 
            uintptr_t dataPtr = uintptr_t(ptr);
            uintptr_t sizePtr = dataPtr - sizeof(size_t);

            size_t size = *reinterpret_cast<size_t*>(sizePtr);
            for (size_t i = 0; i < size; ++i)
            {
                static_cast<T*>(ptr)[i].~T();
            } 
        });

        return static_cast<T*>(dataPtr);
    }

    template <typename T>
    struct TrackedDeleterSTL<T, std::enable_if_t<!std::is_array_v<T>>>
    {
        void operator()(T* ptr)
        {
            TrackedDelete(ptr);
        }
    };

    template <typename T>
    struct TrackedDeleterSTL<T, std::enable_if_t<std::is_array_v<T>>>
    {
        void operator()(std::remove_extent_t<T>* ptr)
        {
            TrackedDeleteArray(ptr);
        }
    };

    template <typename T>
    typename TrackedAllocatorSTL<T>::pointer TrackedAllocatorSTL<T>::allocate(size_type n, const void* hint)
    {
        return (T*)TrackedAlloc(category, sizeof(T) * n, alignof(T));
    }

    template <typename T>
    void TrackedAllocatorSTL<T>::deallocate(pointer p, size_type n)
    {
        return TrackedFree(p);
    }

    template <typename T>
    typename ScopedAllocatorSTL<T>::pointer ScopedAllocatorSTL<T>::allocate(size_type n, const void* hint)
    {
        return (T*)ScopedAlloc(sizeof(T) * n, alignof(T));
    }

    template <typename T>
    void ScopedAllocatorSTL<T>::deallocate(pointer p, size_type n)
    {
        return ScopedFree(p);
    }

    template <typename T, typename... Args> std::enable_if_t<!std::is_array_v<T>, TrackedUniquePtr<T>> MakeUniqueTracked(MemoryCategoryID cat, Args&&... args)
    {
        return TrackedUniquePtr<T>(TrackedNew<T>(cat, std::forward<Args>(args)...));
    }

    template <typename T> std::enable_if_t<std::is_unbounded_array_v<T>, TrackedUniquePtr<T>> MakeUniqueTracked(MemoryCategoryID cat, size_t size)
    {
        return TrackedUniquePtr<T>(TrackedNewArray<std::remove_extent_t<T>>(cat, size));
    }

    template <typename T, typename... Args> std::enable_if_t<!std::is_array_v<T>, TrackedSharedPtr<T>> MakeSharedTracked(MemoryCategoryID cat, Args&&... args)
    {
        return TrackedSharedPtr<T>(TrackedNew<T>(cat, std::forward<Args>(args)...), TrackedDeleterSTL<T>());
    }

    template <typename T> std::enable_if_t<std::is_unbounded_array_v<T>, TrackedSharedPtr<T>> MakeSharedTracked(MemoryCategoryID cat, size_t size)
    {
        return TrackedSharedPtr<T>(TrackedNewArray<std::remove_extent_t<T>>(cat, size), TrackedDeleterSTL<T>());
    }
}