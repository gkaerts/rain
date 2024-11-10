#include <gtest/gtest.h>

#include "common/memory/memory.hpp"


using namespace rn;

RN_DEFINE_MEMORY_CATEGORY(Test)

TEST(MemoryTests, VerifyMemoryCategoryName)
{
    EXPECT_STREQ(MemoryCategoryName(::MemoryCategory::Test), "Test");
}

TEST(MemoryTests, VerifyNumberOfCategoriesAllocated)
{
    EXPECT_GT(NumMemoryCategories(), 0);
}

TEST(MemoryTests, CategoryInfoShouldStartEmpty)
{
    MemoryCategoryInfo info = MemoryInfoForCategory(::MemoryCategory::Test);
    EXPECT_EQ(info.numAllocations, 0);
    EXPECT_EQ(info.allocatedSize, 0);
    EXPECT_EQ(info.totalAllocationCount, 0);
    EXPECT_EQ(info.totalFreeCount, 0);
}

TEST(MemoryTests, AllocationAndFreeTrackingShouldReflectInMemoryInfo)
{
    MemoryCategoryInfo info = MemoryInfoForCategory(::MemoryCategory::Test);
    TrackExternalAllocation(::MemoryCategory::Test, 1024);

    MemoryCategoryInfo infoAfterAlloc = MemoryInfoForCategory(::MemoryCategory::Test);
    EXPECT_EQ(infoAfterAlloc.numAllocations, info.numAllocations + 1);
    EXPECT_EQ(infoAfterAlloc.allocatedSize, info.allocatedSize + 1024);
    EXPECT_EQ(infoAfterAlloc.totalAllocationCount, info.totalAllocationCount + 1);
    EXPECT_EQ(infoAfterAlloc.totalFreeCount, info.totalFreeCount);

    TrackExternalFree(::MemoryCategory::Test, 1024);
    MemoryCategoryInfo infoAfterFree = MemoryInfoForCategory(::MemoryCategory::Test);
    EXPECT_EQ(infoAfterFree.numAllocations, info.numAllocations);
    EXPECT_EQ(infoAfterFree.allocatedSize, info.allocatedSize);
    EXPECT_EQ(infoAfterFree.totalAllocationCount, info.totalAllocationCount + 1);
    EXPECT_EQ(infoAfterFree.totalFreeCount, info.totalFreeCount + 1);
}

TEST(MemoryTests, CanAllocateAndFreeTrackedMemory)
{
    void* ptr = TrackedAlloc(::MemoryCategory::Test, 1024, 16);
    EXPECT_NE(ptr, nullptr);

    TrackedFree(ptr);
}

TEST(MemoryTests, TrackedAllocAndFreeShouldReflectInMemoryInfo)
{
    MemoryCategoryInfo info = MemoryInfoForCategory(::MemoryCategory::Test);

    void* ptr = TrackedAlloc(::MemoryCategory::Test, 1024, 16);

    MemoryCategoryInfo infoAfterAlloc = MemoryInfoForCategory(::MemoryCategory::Test);
    EXPECT_EQ(infoAfterAlloc.numAllocations, info.numAllocations + 1);
    EXPECT_EQ(infoAfterAlloc.allocatedSize, info.allocatedSize + 1024);
    EXPECT_EQ(infoAfterAlloc.totalAllocationCount, info.totalAllocationCount + 1);
    EXPECT_EQ(infoAfterAlloc.totalFreeCount, info.totalFreeCount);

    TrackedFree(ptr);

    MemoryCategoryInfo infoAfterFree = MemoryInfoForCategory(::MemoryCategory::Test);
    EXPECT_EQ(infoAfterFree.numAllocations, info.numAllocations);
    EXPECT_EQ(infoAfterFree.allocatedSize, info.allocatedSize);
    EXPECT_EQ(infoAfterFree.totalAllocationCount, info.totalAllocationCount + 1);
    EXPECT_EQ(infoAfterFree.totalFreeCount, info.totalFreeCount + 1);
}