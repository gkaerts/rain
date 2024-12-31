#pragma once

#include "common/common.hpp"
#include <type_traits>
#include <memory>

namespace rn
{
    // Memory categories
    enum class MemoryCategoryID : uint8_t {};

    MemoryCategoryID AllocateMemoryCategoryID(const char* nameLiteral);
    uint8_t NumMemoryCategories();

    #define RN_DEFINE_MEMORY_CATEGORY(name) \
        namespace MemoryCategory \
        {   \
            extern const rn::MemoryCategoryID name = rn::AllocateMemoryCategoryID(#name); \
        }

    #define RN_MEMORY_CATEGORY(name) \
        namespace MemoryCategory \
        { \
            extern const rn::MemoryCategoryID name; \
        }

    // Memory category tracking info
    struct MemoryCategoryInfo
    {
        size_t numAllocations;
        size_t allocatedSize;

        size_t totalAllocationCount;
        size_t totalFreeCount;
    };

    MemoryCategoryInfo MemoryInfoForCategory(MemoryCategoryID cat);
    const char* MemoryCategoryName(MemoryCategoryID cat);

    void TrackExternalAllocation(MemoryCategoryID cat, size_t size);
    void TrackExternalFree(MemoryCategoryID cat, size_t size);


    // Tracked memory allocation
    RN_MEMORY_CATEGORY(Default)

    void* TrackedAlloc(MemoryCategoryID cat, size_t size, size_t alignment);
    void TrackedFree(void* ptr);

    template <typename T, typename... Args> T* TrackedNew(MemoryCategoryID cat, Args&&... args);
    template <typename T> T* TrackedNewArray(MemoryCategoryID cat, size_t size);

    template <typename T> void TrackedDelete(T* ptr);
    template <typename T> void TrackedDeleteArray(T* ptr);

    // Thread-local scope allocation
    class MemoryScope
    {
    public:
        using FnDeleter = void(*)(void*);
        struct Deleter
        {
            FnDeleter fn = nullptr;
            Deleter* prevDeleter = nullptr;
            void* dataPtr = nullptr;
        };

        MemoryScope();
        ~MemoryScope();

        void PushDeleter(void* ptr, void(*deleteFn)(void*));

    private:
        uintptr_t offset;
        Deleter* lastDeleter;
        MemoryScope* prevScope;
    };

    void InitializeScopedAllocationForThread(size_t backingSize);
    void TeardownScopedAllocationForThread();

    ptrdiff_t CurrentScopeOffset();
    void ResetScope(ptrdiff_t offset);

    void* ScopedAlloc(size_t size, size_t alignment);
    void ScopedFree(void* ptr);

    template <typename T, typename... Args> T* ScopedNew(MemoryScope& scope, Args&&... args);
    template <typename T> T* ScopedNewArray(MemoryScope& scope, size_t size);
    template <typename T> T* ScopedNewArrayNoInit(MemoryScope& scope, size_t size);

    // STL adapters
    template <typename T, typename = void>
    struct TrackedDeleterSTL;

    template <typename T>
    class TrackedAllocatorSTL : public std::allocator<T>
    {
        public:

            using size_type = size_t;
            using pointer = T*;
            using const_pointer = const T*;

            template <typename U>
            struct rebind
            {
                using other = TrackedAllocatorSTL<U>;
            };

            pointer allocate(size_type n, const void* hint = nullptr);
            void deallocate(pointer p, size_type n);

            TrackedAllocatorSTL() throw() : std::allocator<T>(), category(MemoryCategory::Default) {}

            TrackedAllocatorSTL(MemoryCategoryID cat) throw(): std::allocator<T>(), category(cat) {}
            TrackedAllocatorSTL(const TrackedAllocatorSTL& a) throw() : std::allocator<T>(a), category(a.GetCategory()) {}

            template <class U>
            TrackedAllocatorSTL(const TrackedAllocatorSTL<U>& a) throw() : std::allocator<T>(a), category(a.GetCategory()) {}
            ~TrackedAllocatorSTL() throw() {}

            MemoryCategoryID GetCategory() const { return category; }

        private:
            MemoryCategoryID category;
    };

    template <typename T>
    class ScopedAllocatorSTL : public std::allocator<T>
    {
        public:

        using size_type = size_t;
        using pointer = T*;
        using const_pointer = const T*;

        template <typename U>
        struct rebind
        {
            using other = ScopedAllocatorSTL<U>;
        };

        pointer allocate(size_type n, const void* hint = nullptr);
        void deallocate(pointer p, size_type n);

        ScopedAllocatorSTL() throw() : std::allocator<T>() {}
        ScopedAllocatorSTL(const ScopedAllocatorSTL& a) throw() : std::allocator<T>(a) {}

        template <class U>
        ScopedAllocatorSTL(const ScopedAllocatorSTL<U>& a) throw() : std::allocator<T>(a) {}
        ~ScopedAllocatorSTL() throw() {}
    };

    // Tracked smart pointers
    template <typename T> using TrackedUniquePtr = std::unique_ptr<T, TrackedDeleterSTL<T>>;
    template <typename T> using TrackedSharedPtr = std::shared_ptr<T>;

    template <typename T, typename... Args> std::enable_if_t<!std::is_array_v<T>, TrackedUniquePtr<T>> MakeUniqueTracked(MemoryCategoryID cat, Args&&... args);
    template <typename T> std::enable_if_t<std::is_unbounded_array_v<T>, TrackedUniquePtr<T>> MakeUniqueTracked(MemoryCategoryID cat, size_t size);
    template <typename T> std::enable_if_t<std::is_bounded_array_v<T>, TrackedUniquePtr<T>> MakeUniqueTracked(MemoryCategoryID cat, size_t size) = delete;

    template <typename T, typename... Args> std::enable_if_t<!std::is_array_v<T>, TrackedSharedPtr<T>> MakeSharedTracked(MemoryCategoryID cat, Args&&... args);
    template <typename T> std::enable_if_t<std::is_unbounded_array_v<T>, TrackedSharedPtr<T>> MakeSharedTracked(MemoryCategoryID cat, size_t size);
    template <typename T> std::enable_if_t<std::is_bounded_array_v<T>, TrackedSharedPtr<T>> MakeSharedTracked(MemoryCategoryID cat, size_t size) = delete;


    // Virtual memory management
    size_t SystemPageSize();
    void* ReserveVirtualAddressSpace(size_t size);
    void FreeVirtualAddressSpace(void* vptr, size_t size);
    void CommitVirtualAddressSpace(void* vptr, size_t size);
    void DecommitVirtualAddressSpace(void* pptr, size_t size);
}

#include "common/memory/memory.inl"