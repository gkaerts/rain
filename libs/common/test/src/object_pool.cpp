#include <gtest/gtest.h>

#include "common/handle.hpp"
#include "common/memory/memory.hpp"
#include "common/memory/object_pool.hpp"

using namespace rn;

RN_MEMORY_CATEGORY(Test)
RN_DEFINE_HANDLE(DummyHandle, 0x33)

struct HotDummy
{
    uint32_t value;
};

struct ColdDummy
{
    uint32_t value;
};

TEST(ObjectPoolTests, CanCreateHotOnlyObjectPool)
{
    ObjectPool<DummyHandle, HotDummy> pool(::MemoryCategory::Test, 128);
}

TEST(ObjectPoolTests, CanCreateHotColdObjectPool)
{
    ObjectPool<DummyHandle, HotDummy, ColdDummy> pool(::MemoryCategory::Test, 128);
}

TEST(ObjectPoolTests, CanAllocateAndRemoveObjectHotOnly)
{
    ObjectPool<DummyHandle, HotDummy> pool(::MemoryCategory::Test, 128);

    HotDummy dummy =
    {
        .value = 1024
    };

    DummyHandle handle = pool.Store(std::move(dummy));
    EXPECT_TRUE(IsValid(handle));
    EXPECT_EQ(pool.GetHot(handle).value, 1024); 

    pool.Remove(handle);
}

TEST(ObjectPoolTests, CanAllocateAndRemoveObjectHotCold)
{
    ObjectPool<DummyHandle, HotDummy, ColdDummy> pool(::MemoryCategory::Test, 128);

    HotDummy hot =
    {
        .value = 1024
    };

    ColdDummy cold =
    {
        .value = 128
    };

    DummyHandle handle = pool.Store(std::move(hot), std::move(cold));
    EXPECT_TRUE(IsValid(handle));
    EXPECT_EQ(pool.GetHot(handle).value, 1024); 
    EXPECT_EQ(pool.GetCold(handle).value, 128);

    pool.Remove(handle);
}

TEST(ObjectPoolTests, PoolGrowsBeyondInitialCapacity)
{
    ObjectPool<DummyHandle, HotDummy> pool(::MemoryCategory::Test, 4);

    HotDummy dummy =
    {
        .value = 1024
    };

    DummyHandle handles[10];

    for (int i = 0; i < 10; ++i)
    {
        DummyHandle handle = pool.Store(std::move(dummy));
        EXPECT_TRUE(IsValid(handle));

        handles[i] = handle;
    }

    for (int i = 0; i < 10; ++i)
    {
        pool.Remove(handles[i]);
    }
}

TEST(ObjectPoolTests, RemovedHandleReturnsNullptr)
{
    ObjectPool<DummyHandle, HotDummy> pool(::MemoryCategory::Test, 4);
    HotDummy dummy = 
    {
        .value = 1024
    };

    DummyHandle handle = pool.Store(std::move(dummy));
    pool.Remove(handle);
    EXPECT_EQ(pool.GetHotPtr(handle), nullptr);
}

TEST(ObjectPoolTests, HandlesAreAlwaysUnique)
{
    ObjectPool<DummyHandle, HotDummy> pool(::MemoryCategory::Test, 4);
    HotDummy dummy = 
    {
        .value = 1024
    };

    DummyHandle handle = pool.Store(std::move(dummy));
    pool.Remove(handle);

    dummy.value = 1025;
    DummyHandle newHandle = pool.Store(std::move(dummy));

    EXPECT_EQ(pool.GetHotPtr(handle), nullptr);
    EXPECT_NE(pool.GetHotPtr(newHandle), nullptr);
}