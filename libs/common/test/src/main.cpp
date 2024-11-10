#include "gtest/gtest.h"

#include "common/memory/memory.hpp"

struct TestingSetup
{
    TestingSetup()
    {
        rn::InitializeScopedAllocationForThread(16 * rn::MEGA);
    }

    ~TestingSetup()
    {
        rn::TeardownScopedAllocationForThread();
    }
};

int main(int argc, char* argv[])
{
    TestingSetup testingSetup;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}