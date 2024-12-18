#pragma once

#include "common/common.hpp"
#include "common/math/math.hpp"

#if RN_PLATFORM_WINDOWS
struct SDL_Window;
#endif

namespace rn::rhi
{
    class Device;
    class SwapChain;
    enum class PresentMode : uint32_t;
}

namespace rn::app
{
    class RenderWindow
    {
    public:

        RenderWindow(rhi::Device* rhiDevice, const char* title, uint32_t width, uint32_t height, rhi::PresentMode presentMode);
        ~RenderWindow();

        RenderWindow(const RenderWindow&) = delete;
        RenderWindow(RenderWindow&&) = delete;
        RenderWindow& operator=(const RenderWindow&) = delete;
        RenderWindow& operator=(RenderWindow&&) = delete;

        void            OnResize();
        bool            IsVisible() const;

        void*           NativeWindow() const { return _nativeWindow; }
        rhi::SwapChain* RHISwapChain() const { return _rhiSwapChain; }

    #if RN_PLATFORM_WINDOWS
        SDL_Window*     SDLWindow() const { return _sdlWindow; }

    #endif

        uint2           DrawableSize() const;
        float           DPIScale() const;

    private:

        void InitializeWindow(const char* title, uint32_t width, uint32_t height);
        void TeardownWindow();
        void ResizeInternal(uint32_t width, uint32_t height);

        rhi::SwapChain* _rhiSwapChain = nullptr;
        void* _nativeWindow = nullptr;

    #if RN_PLATFORM_WINDOWS
        SDL_Window* _sdlWindow = nullptr;
    #endif
    };
}