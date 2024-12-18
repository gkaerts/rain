#pragma once

#include "common/common.hpp"
#include "common/log/log.hpp"

namespace rn::rhi
{
    class Device;
};

namespace rn::app
{
    class RenderWindow;

    enum class RHIDeviceType : uint32_t
    {
        D3D12 = 0,
    };

    using FnPlatformEventListener = void(*)(void* event);

    struct ApplicationConfig
    {
        size_t threadScopeBackingSize;
        LoggerSettings loggerSettings;
        RHIDeviceType rhiDeviceType;

        const char* mainWindowTitle;
        uint32_t mainWindowWidth;
        uint32_t mainWindowHeight;

        FnPlatformEventListener eventListenerHook;

        int argc;
        char** argv;
    };

    class Application
    {
    public:

        Application(const ApplicationConfig& config);
        ~Application();

        bool ProcessEvents();

        RenderWindow*   MainWindow() const { return _mainWindow; }
        rhi::Device*    RHIDevice() const { return _rhiDevice; }

    private:

        void InitializePlatform();
        void TeardownPlatform();

        rhi::Device* _rhiDevice = nullptr;
        RenderWindow* _mainWindow = nullptr;
        FnPlatformEventListener _eventListenerHook = nullptr;
    };

};