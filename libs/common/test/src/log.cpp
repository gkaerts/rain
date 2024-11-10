#include <gtest/gtest.h>

#include "common/log/log.hpp"

using namespace rn;

RN_DEFINE_LOG_CATEGORY(Test)

TEST(LogTests, CanInitializeAndTeardownLogger)
{
    LoggerSettings settings{};
    InitializeLogger(settings);
    LogInfo(::LogCategory::Test, "Hello GoogleTest, I am an info log statement!");
    LogWarning(::LogCategory::Test, "Hello GoogleTest, I am a warning log statement!");
    LogError(::LogCategory::Test, "Hello GoogleTest, I am an error log statement!");
    TeardownLogger();
}