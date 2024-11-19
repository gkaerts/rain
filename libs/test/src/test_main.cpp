#include "gtest/gtest.h"
#include "common/memory/memory.hpp"
#include "common/log/log.hpp"
#include "common/task/scheduler.hpp"

struct TestingSetup
{
    TestingSetup()
    {
     
        rn::InitializeScopedAllocationForThread(16 * rn::MEGA);
        rn::InitializeLogger({
            .desktop = {
                .logFilename = "log_test.txt"
            }
        });
        rn::InitializeTaskScheduler();
    }

    ~TestingSetup()
    {
        rn::TeardownTaskScheduler();
        rn::TeardownLogger();
        rn::TeardownScopedAllocationForThread();
    }
};

int main(int argc, char* argv[])
{
    TestingSetup testingSetup;

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}