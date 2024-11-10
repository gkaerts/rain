#pragma once

#include "common/common.hpp"

namespace enki
{
    class TaskScheduler;
}

namespace rn
{
    void InitializeTaskScheduler();
    void TeardownTaskScheduler();

    enki::TaskScheduler* TaskScheduler();
}