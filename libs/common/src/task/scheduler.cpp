#include "TaskScheduler.h"

#include "common/task/scheduler.hpp"
#include "common/memory/memory.hpp"
#include "common/log/log.hpp"

namespace rn
{
    RN_DEFINE_LOG_CATEGORY(Task)
    RN_DEFINE_MEMORY_CATEGORY(Task)

    namespace
    {
        constexpr const size_t THREAD_SCOPE_BACKING_SIZE = 16 * 1024 * 1024;
        enki::TaskScheduler* SCHEDULER = nullptr;
    }

    void InitializeTaskScheduler()
    {
        if (SCHEDULER)
        {
            LogError(LogCategory::Task, "Task scheduler already initialized!");
            return;
        }

        enki::TaskSchedulerConfig config;
        config.customAllocator.alloc = []( size_t align_, size_t size_, void* userData_, const char* file_, int line_) -> void*
        {
            return TrackedAlloc(MemoryCategory::Task, size_, align_);
        };

        config.customAllocator.free = [](void* ptr_, size_t size_, void* userData_, const char* file_, int line_)
        {
            TrackedFree(ptr_);
        };

        // Hijack the profiler callbacks to do thread init and shutdown hooks for scope allocation
        config.profilerCallbacks.threadStart = [](uint32_t threadnum_)
        {
            InitializeScopedAllocationForThread(THREAD_SCOPE_BACKING_SIZE);
        };

        config.profilerCallbacks.threadStop = [](uint32_t threadnum_)
        {
            TeardownScopedAllocationForThread();
        };

        SCHEDULER = TrackedNew<enki::TaskScheduler>(MemoryCategory::Task);
        SCHEDULER->Initialize(config);
    }

    void TeardownTaskScheduler()
    {
        if (!SCHEDULER)
        {
            LogError(LogCategory::Task, "Task scheduler not initialized!");
            return;
        }

        SCHEDULER->WaitforAllAndShutdown();
    }

    enki::TaskScheduler* TaskScheduler()
    {
        return SCHEDULER;
    }
}