#include "app/application.hpp"
#include "app/render_window.hpp"

#include "common/memory/memory.hpp"
#include "common/log/log.hpp"
#include "common/task/scheduler.hpp"

#include "rhi/device.hpp"
#include "rhi/rhi_d3d12.hpp"

namespace rn::app
{
    RN_DEFINE_MEMORY_CATEGORY(App);

    Application::Application(const ApplicationConfig& config)
    {
        InitializePlatform();
        InitializeScopedAllocationForThread(config.threadScopeBackingSize);
        InitializeLogger(config.loggerSettings);
        InitializeTaskScheduler();

        switch(config.rhiDeviceType)
        {
            case RHIDeviceType::D3D12:
            default:
            {
                rhi::DeviceD3D12Options d3d12Options = 
                {
                    .adapterIndex = 0,
                    .enableDebugLayer = true,
                    .enableGPUValidation = false
                };

                _rhiDevice = rhi::CreateD3D12Device(d3d12Options, rhi::DefaultDeviceMemorySettings());
            }
            break;
        }

        _mainWindow = TrackedNew<RenderWindow>(MemoryCategory::App,
            _rhiDevice,
            config.mainWindowTitle,
            config.mainWindowWidth,
            config.mainWindowHeight,
            rhi::PresentMode::VSync);
    }

    Application::~Application()
    {
        TrackedDelete(_mainWindow);
        rhi::DestroyDevice(_rhiDevice);

        TeardownTaskScheduler();
        TeardownLogger();
        TeardownScopedAllocationForThread();
        TeardownPlatform();
    }
}