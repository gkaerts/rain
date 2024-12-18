#include "common/config.hpp"

#if RN_PLATFORM_WINDOWS
#include "app/render_window.hpp"
#include "SDL.h"
#include "SDL_syswm.h"

namespace rn::app
{
    namespace
    {
        constexpr const float DEFAULT_DPI = 96.0f;
    }

    void RenderWindow::InitializeWindow(const char* title, uint32_t width, uint32_t height)
    {
        _sdlWindow = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            int(width),
            int(height),
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    #if RN_PLATFORM_WINDOWS
        SDL_SysWMinfo wmInfo{ };
        SDL_GetWindowWMInfo(_sdlWindow, &wmInfo);
        _nativeWindow = wmInfo.info.win.window;
    #endif // RN_PLATFORM_WINDOWS

        SDL_SetWindowData(_sdlWindow, "RN_RENDER_WINDOW", this);
    }

    void RenderWindow::TeardownWindow()
    {
        SDL_DestroyWindow(_sdlWindow);
    }

    bool RenderWindow::IsVisible() const
    {
        return (SDL_GetWindowFlags(_sdlWindow) & SDL_WINDOW_SHOWN) != 0;
    }

    uint2 RenderWindow::DrawableSize() const
    {
        int width = 0, height = 0;
        SDL_GetWindowSizeInPixels(_sdlWindow,
            &width,
            &height);

        return { uint32_t(width), uint32_t(height) };
    }

    float RenderWindow::DPIScale() const
    {
        float hdpi = 0.0f;
        SDL_GetDisplayDPI(SDL_GetWindowDisplayIndex(_sdlWindow), nullptr, &hdpi, nullptr);
        return hdpi / DEFAULT_DPI;
    }
}

#endif // RN_PLATFORM_WINDOWS